#include <Windows.h>
#include <string>
//#include <jni.h>
#include "../../../native/jni_include/jni.h"

HINSTANCE jvmLibrary;
jint(JNICALL * CreateJVM_Method)(JavaVM **pvm, void **penv, void *args);
#define USE_JNI_VERSION JNI_VERSION_1_4
JavaVM *jvm;

std::string formatMessage(DWORD msgCode)
{
	LPVOID lpMsgBuf;
	DWORD rc = ::FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			msgCode,
			0,
			(LPTSTR) & lpMsgBuf,
			0, NULL );
	if (rc == 0)
	{
		printf("format message returned 0 (%d)", ::GetLastError());
		exit(-1);
	}

	std::string res((LPTSTR) lpMsgBuf);
	::LocalFree(lpMsgBuf);
	return res;
}

void loadLibrary(const char* path)
{
	jvmLibrary = ::LoadLibrary(path);
	if (jvmLibrary == NULL)
	{
		printf("Failed to load JVM from %s\n", path);
		printf("LastError %d\n", GetLastError());
		printf("Message: %s\n", formatMessage(GetLastError()).c_str());
		exit(-1);
	}
}

void unloadLibrary()
{
	if (::FreeLibrary(jvmLibrary) == 0)
	{
		printf("  Failed to unload JVM\n");
		printf("  LastError %d\n", GetLastError());
		printf("  Message: %s\n", formatMessage(GetLastError()).c_str());
		exit(-1);
	}
}

void* getSymbol(const char* name)
{
	void* res = (void*) ::GetProcAddress(jvmLibrary, name);
	if (res == NULL)
	{
		printf("  Failed to get Symbol %s\n", name);
		printf("  LastError %d\n", GetLastError());
		printf("  Message: %s\n", formatMessage(GetLastError()).c_str());
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
		printf("Usage:  %s %%JAVA_HOME%%/bin/server/jvm.dll [JVM_ARGS]", argv[0]);
		exit(-1);
	}

	printf("Check paths\n");
	char directory[1024];
	int sz = ::GetSystemDirectoryA(directory, 1024);
	if (sz == 0)
	{
		printf("GetSystemDirectory failed\n");
		exit(-1);
	}
	printf("  SystemDirectory: %s\n", std::string(directory, sz).c_str());
	sz = ::GetWindowsDirectoryA(directory, 1024);
	if (sz == 0)
	{
		printf("GetWindowsDirectory failed\n");
		exit(-1);
	}
	printf("  WindowsDirectory: %s\n", std::string(directory, sz).c_str());

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
