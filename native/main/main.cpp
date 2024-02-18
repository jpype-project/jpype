#include <cstdio>
#include <string>
#include <Python.h>
#include <iostream>
#include "jni.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

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
jclass get_main(JNIEnv*env, char* jar)
{
    (env)->PushLocalFrame(20);
    jclass clsCls = (env)->FindClass("java/lang/Class");
    jclass urlClassLoaderCls = (env)->FindClass("java/net/URLClassLoader");
    jclass urlCls = (env)->FindClass("java/net/URL");
    if (clsCls == NULL || urlClassLoaderCls == NULL || urlCls == NULL)
    {
        (env)->PopLocalFrame(NULL);
        return NULL;
    }
    jmethodID urlConstructor = (env)->GetMethodID(urlCls, "<init>", "(Ljava/lang/String;)V");
    jmethodID uclConstructor = (env)->GetMethodID(urlClassLoaderCls, "<init>", "([Ljava/net/URL;)V");
    jmethodID forname = (env)->GetStaticMethodID(clsCls, "forName", "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
    if (urlConstructor == NULL || uclConstructor == NULL || forname == NULL)
    {
        (env)->PopLocalFrame(NULL);
        return NULL;
    }
    jobject jstr = (env)->NewStringUTF(jar);
    jobject url = (env)->NewObject(urlCls, urlConstructor, jstr);
    jobjectArray urls = (env)->NewObjectArray(1, urlCls, NULL);
    (env)->SetObjectArrayElement(urls, 0, url);
    jobject classLoader = (env)->NewObject(urlClassLoaderCls, uclConstructor, urls);
    jobject classToLoad = (env)->NewStringUTF("org.jpype.Main");
    jobject result = (env)->CallStaticObjectMethod(clsCls, forname, classToLoad, 1, classLoader);
    return (jclass) (env)->PopLocalFrame(result);
}

jstring get_native(JNIEnv*env)
{
   PyObject* import = PyImport_AddModule("importlib.util");
   PyObject* jpype = PyObject_CallMethod(import, "find_spec", "s", "_jpype");
   PyObject* origin = PyObject_GetAttrString(jpype, "origin");
   jstring result = (env)->NewStringUTF(toString(origin));
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
    PyObject *jpype_private = PyImport_ImportModule("_jpype");
    char *jvmpath = toString(file);
    char *jarpath = toString(path);

    /* Find the entry point for the JVM */
#ifdef _WIN32
    wchar_t *wjvmpath = Py_DecodeLocale(jvmpath, NULL);
    if (wjvmpath == nullptr)
    {
        std::cerr << "JAVA_HOME must be set" << std::endl;
        return -1;
    }
    // load the entry point
    void* libjvm = LoadLibraryW(wjvmpath);
    typedef jint (*JNI_CreateJavaVM_t)(JavaVM**, JNIEnv**, void*);
    JNI_CreateJavaVM_t JNI_CreateJavaVM = (JNI_CreateJavaVM_t)GetProcAddress((HMODULE)libjvm, "JNI_CreateJavaVM");
#else
    if (jvmpath == nullptr)
    {
        std::cerr << "JAVA_HOME must be set" << std::endl;
        return -1;
    }
    void* libjvm = dlopen(jvmpath, RTLD_NOW);
    typedef jint (*JNI_CreateJavaVM_t)(JavaVM**, JNIEnv**, void*);
    JNI_CreateJavaVM_t JNI_CreateJavaVM = (JNI_CreateJavaVM_t)dlsym(libjvm, "JNI_CreateJavaVM");
#endif
    if (JNI_CreateJavaVM == nullptr)
    {
        std::cerr << "Unable to load JNI entrypoint." << std::endl;
        return -1;
    }
 
    /* Next launch Java */
    JavaVM *jvm;       /* denotes a Java VM */
    JNIEnv* env;       /* pointer to native method interface */
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
     * pointer inenv */
    JNI_CreateJavaVM(&jvm, &env, &vm_args);
    free(options);

    /* Find the main entry point for JVM */
    jclass main = get_main(env, jarpath);
    if (main == NULL)
    {
        fprintf(stderr, "Unable to find entry class for org.jpype");
        return -1;
    }
    free(jarpath);
    jmethodID mid = env->GetStaticMethodID(main, "mainX", "([Ljava/lang/String;Ljava/lang/String;)V");
    if (mid == NULL)
    {
        fprintf(stderr, "Unable to find entry point for org.jpype");
        return -1;
    }
 
    /* Set up arguments to main */
    jclass str = env->FindClass("java/lang/String");
    jobjectArray stra = (env)->NewObjectArray(argc, str, NULL);

    /* Copy program name first */
    env->SetObjectArrayElement(stra, 0, (env)->NewStringUTF(argv[0]));

    /* Copy remaining arguments */
    j=1;
    for (; i<argc; ++i)
    {
        env->SetObjectArrayElement(stra, j++, (env)->NewStringUTF(argv[i]));
    }
    jobject native = get_native(env);
 
    /* Call jpype main method */ 
    env->CallStaticVoidMethod(main, mid, stra, native);

    /* Java returns control, so now we wait for all user threads to end. */
    printf("wait for user thread to end.\n");
    jvm->DestroyJavaVM();

    printf("free Python resources.\n");

    if (jpype_imports)
        Py_DECREF(jpype_imports);
    if (jpype_private)
        Py_DECREF(jpype_private);

    /* Java is done so we can free remaining Python resources */
    if (Py_FinalizeEx() < 0) {
        exit(120);
    }

    /* Shutdown the JVM with Python */
    PyMem_RawFree(program);
    return 0;
}
