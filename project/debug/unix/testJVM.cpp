#if defined(_HPUX) && !defined(_IA64)
#include <dl.h>
#else
#include <dlfcn.h>
#endif // HPUX
#include <errno.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <sstream>
//#include <jni.h>
#include "../../../native/jni_include/jni.h"

void* jvmLibrary;
jint(JNICALL * CreateJVM_Method)(JavaVM **pvm, void **penv, void *args);
#define USE_JNI_VERSION JNI_VERSION_1_4
JavaVM *jvm;

void loadLibrary(const char* path)
{
#if defined(_HPUX) && !defined(_IA64)
	jvmLibrary = shl_load(path, BIND_DEFERRED | BIND_VERBOSE, 0L);
#else
	jvmLibrary = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
#endif // HPUX
	if (jvmLibrary == NULL)
	{
		printf("Failed to load JVM from %s\n", path);
		printf("Errno: %d\n", errno);
		if (errno == ENOEXEC)
		{
			printf("diagnostics: %s\n", dlerror());
		}
		exit(-1);
	}
}

void unloadLibrary()
{
	int r = dlclose(jvmLibrary);
	if (r != 0) // error
	{
		printf("  Failed to unload JVM\n");
		printf("Errno: %d\n", errno);
		printf("Dlerror: %s\n", dlerror());
		exit(-1);
	}
}

void* getSymbol(const char* name)
{
	void* res = dlsym(jvmLibrary, name);
	if (res == NULL)
	{
		printf("  Failed to get Symbol %s\n", name);
		printf("errno %d\n", errno);
		exit(-1);
	}
	return res;
}

void loadEntryPoints()
{
	CreateJVM_Method = (jint(JNICALL *)(JavaVM **, void **, void *) )getSymbol("JNI_CreateJavaVM");
	printf("  Entry point found %p\n", CreateJVM_Method);
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Usage:  %s $JAVA_HOME/lib/server/libjvm.so [JVM_ARGS]\n", argv[0]);
		exit(-1);
	}

	printf("Load library\n");
	loadLibrary(argv[1]);

	printf("Load entry points\n");
	loadEntryPoints();

	// Pack the arguments
	printf("Pack JVM arguments\n");
	JavaVMInitArgs jniArgs;
	jniArgs.options = NULL;
	jniArgs.version = USE_JNI_VERSION;
	jniArgs.ignoreUnrecognized = false;
	jniArgs.nOptions = (jint) argc - 2;
	printf("  Num options: %d\n", jniArgs.nOptions);
	jniArgs.options = (JavaVMOption*) malloc(sizeof (JavaVMOption) * jniArgs.nOptions);
	memset(jniArgs.options, 0, sizeof (JavaVMOption) * jniArgs.nOptions);
	for (int i = 0; i < jniArgs.nOptions; i++)
	{
		printf("  Option %s\n", argv[i + 2]);
		jniArgs.options[i].optionString = (char*) argv[i + 2];
	}

	// Launch the JVM
	printf("Create JVM\n");
	JNIEnv* env;
	jint rc = CreateJVM_Method(&jvm, (void**) &env, (void*) &jniArgs);
	free(jniArgs.options);
	if (rc != JNI_OK)
	{
		printf("  Create JVM failed. (%d)\n", rc);
		exit(-1);
	}

	// Create some resources
	printf("Create Java resources\n");
	jclass cls = env->FindClass("java/lang/Object");
	jmethodID mth = env->GetMethodID(cls, "toString", "()Ljava/lang/String;");
	mth = env->GetMethodID(cls, "equals", "(Ljava/lang/Object;)Z");
	mth = env->GetMethodID(cls, "hashCode", "()I");
	mth = env->GetMethodID(cls, "getClass", "()Ljava/lang/Class;");


	// Destroy the JVM
	printf("Destroy JVM\n");
	jvm->DestroyJavaVM();

	printf("Unload library\n");
	unloadLibrary();
	printf("Success\n");
}
