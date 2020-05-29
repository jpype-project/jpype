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

#define USE_JNI_VERSION JNI_VERSION_1_4
JavaVM *jvm;

int main(int argc, char** argv)
{
	if (argc < 1)
	{
		printf("Usage:  %s [JVM_ARGS]\n", argv[0]);
		exit(-1);
	}

	// Pack the arguments
	printf("Pack JVM arguments\n");
	JavaVMInitArgs jniArgs;
	jniArgs.options = NULL;
	jniArgs.version = USE_JNI_VERSION;
	jniArgs.ignoreUnrecognized = false;
	jniArgs.nOptions = (jint) argc - 1;
	printf("  Num options: %d\n", jniArgs.nOptions);
	jniArgs.options = (JavaVMOption*) malloc(sizeof (JavaVMOption) * jniArgs.nOptions);
	memset(jniArgs.options, 0, sizeof (JavaVMOption) * jniArgs.nOptions);
	for (int i = 0; i < jniArgs.nOptions; i++)
	{
		printf("  Option %s\n", argv[i + 1]);
		jniArgs.options[i].optionString = (char*) argv[i + 1];
	}

	// Launch the JVM
	printf("Create JVM\n");
	JNIEnv* env;
	jint rc = JNI_CreateJavaVM(&jvm, (JNIEnv**) &env, (void*) &jniArgs);
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

	printf("Success\n");
}
