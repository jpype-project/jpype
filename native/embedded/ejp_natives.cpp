#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#include <link.h>
#endif
#include <jni.h>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	JNIEnv *env;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK)
        return -1;
    printf("Load natives library\n");
    return JNI_VERSION_1_6;
}


JNIEXPORT jlong JNICALL Java_org_jpype_bridge_Natives_loadLibrary
(JNIEnv *env, jclass clazz, jstring lib)
{
    const char *cstr = env->GetStringUTFChars(lib, nullptr);
	void *handle;
    handle = dlopen(cstr, RTLD_GLOBAL | RTLD_LAZY); 
	return (jlong) handle;
}


JNIEXPORT void JNICALL Java_org_jpype_bridge_Natives_dumpLibraries
(JNIEnv *env, jclass clazz)
{
#ifdef WIN32
#else
	using UnknownStruct = struct unknown_struct {
	   void*  pointers[3];
	   struct unknown_struct* ptr;
	};
	using LinkMap = struct link_map;
	void* handle = dlopen(NULL, RTLD_NOW);
	UnknownStruct* p = reinterpret_cast<UnknownStruct*>(handle)->ptr;
	LinkMap* map = reinterpret_cast<LinkMap*>(p->ptr);
	while (map) {
	  std::cout << map->l_name << std::endl;
	  map = map->l_next;
	}
#endif
}

#ifdef __cplusplus
}
#endif
