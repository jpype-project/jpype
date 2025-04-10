/*
This is a fake module which is installed with the _jpype module to hold the prelaunch hooks.
*/
#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif
#include <jni.h>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif

/* Stock System.loadLibrary() does not work for us because they load the
   shared library with local flags.  We need a load which supports shared
   used with all Python modules.
*/
JNIEXPORT jlong JNICALL Java_org_jpype_bridge_BootstrapLoader_loadLibrary
(JNIEnv *env, jclass clazz, jstring lib)
{
    const char *cstr = env->GetStringUTFChars(lib, nullptr);
	void *handle;
    handle = dlopen(cstr, RTLD_GLOBAL | RTLD_LAZY); 
	return (jlong) handle;
}

#ifdef __cplusplus
}
#endif
