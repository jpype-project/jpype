#include <stdio.h>
#include <Python.h>
#include "jni.h"

char* toString(PyObject* obj)
{
    if (obj==NULL)
        return NULL;
    PyObject *bytes = PyUnicode_AsASCIIString(obj);
    if (bytes == NULL)
        return NULL;
    char* out = strdup( PyBytes_AsString(bytes));
    Py_DECREF(bytes);
    return out;
}

/* Use a URLLoader to load org.jpype.   We are not going to be on the main classloader or
 we would be forced to merge user class path with our classpath. */
jclass get_main(JNIEnv* env, char* jar)
{
    (*env)->PushLocalFrame(env, 20);
    jclass clsCls = (*env)->FindClass(env, "java/lang/Class");
    jclass urlClassLoaderCls = (*env)->FindClass(env, "java/net/URLClassLoader");
    jclass urlCls = (*env)->FindClass(env, "java/net/URL");
    if (clsCls == NULL || urlClassLoaderCls == NULL || urlCls == NULL)
    {
        (*env)->PopLocalFrame(env, NULL);
        return NULL;
    }
    jmethodID urlConstructor = (*env)->GetMethodID(env, urlCls, "<init>", "(Ljava/lang/String;)V");
    jmethodID uclConstructor = (*env)->GetMethodID(env, urlClassLoaderCls, "<init>", "([Ljava/net/URL;)V");
    jmethodID forname = (*env)->GetStaticMethodID(env, clsCls, "forName", "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
    if (urlConstructor == NULL || uclConstructor == NULL || forname == NULL)
    {
        (*env)->PopLocalFrame(env, NULL);
        return NULL;
    }
    jobject jstr = (*env)->NewStringUTF(env, jar);
    jobject url = (*env)->NewObject(env, urlCls, urlConstructor, jstr);
    jobjectArray urls = (*env)->NewObjectArray(env, 1, urlCls, NULL);
    (*env)->SetObjectArrayElement(env, urls, 0, url);
    jobject classLoader = (*env)->NewObject(env, urlClassLoaderCls, uclConstructor, urls);
    jobject classToLoad = (*env)->NewStringUTF(env, "org.jpype.Main");
    jobject result = (*env)->CallStaticObjectMethod(env, clsCls, forname, classToLoad, 1, classLoader);
    return (jclass) (*env)->PopLocalFrame(env, result);
}

jstring get_native(JNIEnv* env)
{
   PyObject* import = PyImport_AddModule("importlib.util");
   PyObject* jpype = PyObject_CallMethod(import, "find_spec", "s", "_jpype");
   PyObject* origin = PyObject_GetAttrString(jpype, "origin");
   jstring result = (*env)->NewStringUTF(env, toString(origin));
   Py_DECREF(jpype);
   Py_DECREF(origin);
   return result;
}

int main(int argc, char** argv)
{

    wchar_t *program = Py_DecodeLocale(argv[0], NULL);
    if (program == NULL) {
        fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
        exit(1);
    }
    Py_SetProgramName(program);  /* optional but recommended */
    Py_Initialize();
    PyObject *jpype = PyImport_ImportModule("jpype._bootstrap");
    PyObject *jpype_imports = PyImport_ImportModule("jpype.imports");
    PyObject *path = PyObject_GetAttrString(jpype, "_JPypeJarPath");
    PyObject *file = PyObject_GetAttrString(jpype, "_JPypeJVMPath");
    PyObject *jpypePrivate = PyImport_ImportModule("_jpype");
    char *jvmpath = toString(file);
    char *jarpath = toString(path);

    /** FIXME load from the specified JVM */
    printf("%s\n", jvmpath);

    // Next launch Java
    JavaVM *jvm;       /* denotes a Java VM */
    JNIEnv *env;       /* pointer to native method interface */
    JavaVMInitArgs vm_args; /* JDK/JRE 10 VM initialization arguments */

    /* we need to parse the command lines looking for anything that is supposed to go to Java and pass it here */

    /* Count up the arguments for the JVM */
    int j_args = 0;
    for (int i=1; i<argc; ++i)
    {
        if (argv[i][0] != '-')
            break;
        if (strlen(argv[i])<2)
            continue;

        /* -X* and -D* go to the JVM.  -- ends interpretation */
        if (argv[i][1] == 'X' || argv[i][1] == 'D')
        {
            j_args++;
            continue;
        }
        if (argv[i][1] == '-')
        {
            i++;
        }
        break;
    }

    /* Collect together argments intended for the JVM */
    JavaVMOption* options = (JavaVMOption*) malloc(j_args * sizeof(JavaVMOption));
    int i;
    int j = 0;
    for (i=1; i<argc; ++i)
    {
        if (argv[i][0] != '-')
            break;
        if (strlen(argv[i])<2)
            continue;
        if (argv[i][1] == 'X' || argv[i][1] == 'D')
        {
            options[j++].optionString = argv[i];
            continue;
        }
        if (argv[i][1] == '-')
        {
            i++;
        }
        break;
    }
    vm_args.version = JNI_VERSION_10;
    vm_args.nOptions = j_args;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = 0;

    /* load and initialize a Java VM, return a JNI interface
     * pointer in env */
    JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
    free(options);

    /* Find the main entry point for JVM */
    jclass main = get_main(env, jarpath);
    if (main == NULL)
    {
        fprintf(stderr, "Unable to find entry class for org.jpype");
        return -1;
    }
    free(jarpath);
    jmethodID mid = (*env)->GetStaticMethodID(env, main, "mainX", "([Ljava/lang/String;Ljava/lang/String;)V");
    if (mid == NULL)
    {
        fprintf(stderr, "Unable to find entry point for org.jpype");
        return -1;
    }
 
    /* Set up arguments to main */
    jclass str = (*env)->FindClass(env, "java/lang/String");
    jobject stra = (*env)->NewObjectArray(env, argc, str, NULL);

    /* Copy program name first */
    (*env)->SetObjectArrayElement(env, stra, 0, (*env)->NewStringUTF(env, argv[0]));
    j=1;
    /* Copy remaining arguments */
    for (; i<argc; ++i)
    {
        (*env)->SetObjectArrayElement(env, stra, j++, (*env)->NewStringUTF(env, argv[i]));
    }
    jobject native = get_native(env);
 
    /* Call jpype main method */ 
    printf("Transfer control to Java\n");
    (*env)->CallStaticVoidMethod(env, main, mid, stra, native);

    /* Java returns control, so now we wait for all user threads to end. */
    printf("wait for user thread to end.\n");
    (*jvm)->DestroyJavaVM(jvm);

    /* Java is done so we can free remaining Python resources */
    if (Py_FinalizeEx() < 0) {
        exit(120);
    }

    /* Shutdown the JVM with Python */
    PyMem_RawFree(program);
    return 0;
}
