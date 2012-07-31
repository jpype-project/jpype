/*****************************************************************************
   Copyright 2004-2008 Steve Menard

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
*****************************************************************************/   
#include <jpype.h>

//AT's on porting:
//  1) the original definition of global static object leads to crashing
//on HP-UX platform. Cause: it is suspected to be an undefined order of initialization of static objects
//
//  2) TODO: in any case, use of static objects may impose problems in multi-threaded environment.
//Therefore, they must be guarded with a mutex.
#ifdef WIN32
	#include "jp_platform_win32.h"
	#define  PLATFORM_ADAPTER Win32PlatformAdapter
#else
	#include "jp_platform_linux.h"
	#define  PLATFORM_ADAPTER LinuxPlatformAdapter  
#endif

JPPlatformAdapter* JPJavaEnv::GetAdapter()
{
	static JPPlatformAdapter* adapter = new PLATFORM_ADAPTER();
	return adapter;
}

#define JAVA_CHECK(msg) \
	if (JPEnv::getJava()->ExceptionCheck()) \
	{ \
		RAISE(JavaException, msg); \
	} \

jint (JNICALL *JPJavaEnv::CreateJVM_Method)(JavaVM **pvm, void **penv, void *args);
jint (JNICALL *JPJavaEnv::GetCreatedJVMs_Method)(JavaVM **pvm, jsize size, jsize* nVms);

JNIEnv* JPJavaEnv::getJNIEnv()
{
	JNIEnv* env;
	GetEnv(&env);
	return env;
}

void JPJavaEnv::load(const string& path)
{
	TRACE_IN("JPJavaEnv::load");
	
	// WIN32
	GetAdapter()->loadLibrary((char*)path.c_str());
	CreateJVM_Method = (jint (JNICALL *)(struct JavaVM_ ** ,void ** ,void *))GetAdapter()->getSymbol("JNI_CreateJavaVM");
	GetCreatedJVMs_Method = (jint (JNICALL *)(struct JavaVM_ ** , jsize, jsize*))GetAdapter()->getSymbol("JNI_GetCreatedJavaVMs");
	// No idea why I can't find this symbol .... no matter, it does not work anyway.
	//JNI_DestroyJavaVM = (jint (__stdcall *)(struct JavaVM_ *))GetAdapter()->getSymbol("DestroyJavaVM");
TRACE_OUT;
}

void JPJavaEnv::shutdown()
{
	jvm = NULL;
	// TODO unload the library
}

/**
	throw a JPypeException if the JVM is not started
*/
void JPJavaEnv::checkInitialized()
{
	if (! JPEnv::isInitialized())
	{
		RAISE( JPypeException, "Java Subsystem not started");
	}
}

JPJavaEnv* JPJavaEnv::GetCreatedJavaVM()  
{
	/*
	TRACE_IN("JPJavaEnv::GetCreatedJavaVM");

	JavaVM* vm = NULL;
	jsize numVms;
	
	jint ret = GetCreatedJVMs_Method(&vm, 1, &numVms);
	if (ret < 0)
	{
		return NULL;
	}
	
	return new JPJavaEnv(vm);
	TRACE_OUT;
	*/
	return NULL;
}

JPJavaEnv* JPJavaEnv::CreateJavaVM(void* arg)
{
	TRACE_IN("JPJavaEnv::CreateJavaVM");

	JavaVM* vm = NULL;
	void* env; 
	

	CreateJVM_Method(&vm, &env, arg);
	if (vm == NULL)
	{
		return NULL;
	}
	
	TRACE1("A");
	return new JPJavaEnv(vm);
	TRACE_OUT;
}

int JPJavaEnv::DestroyJavaVM()
{
	if (jvm != NULL)
	{
		int res = jvm->functions->DestroyJavaVM(jvm);
		if (res == 0)
		{
			jvm = NULL;
		}
	}
	return 0;
}


void JPJavaEnv::DeleteLocalRef(jobject obj)
{
	//TRACE_IN("JPJavaEnv::DeleteLocalRef");
	//TRACE1((long)obj);
	JNIEnv* env = getJNIEnv();
	if (env != NULL)
	{
		env->functions->DeleteLocalRef(env, obj);
	}
	//TRACE_OUT;
}

void JPJavaEnv::DeleteGlobalRef(jobject obj)
{
	//TRACE_IN("JPJavaEnv::DeleteGlobalRef");
	//TRACE1((long)obj);
	JNIEnv* env = getJNIEnv();
	if (env != NULL)
	{
		env->functions->DeleteGlobalRef(env, obj);
	}
	//TRACE_OUT;
}

jobject JPJavaEnv::NewLocalRef(jobject a0)
{
	//TRACE_IN("JPJavaEnv::NewLocalRef");
	//TRACE1((long)a0);
	jobject res;
	JNIEnv* env = getJNIEnv();
	res = env->functions->NewLocalRef(env, a0);
	//TRACE1((long)res); //, JPJni::getClassName(a0).getSimpleName());
	return res;
	//TRACE_OUT;
}

jobject JPJavaEnv::NewGlobalRef(jobject a0)
{
	//TRACE_IN("JPJavaEnv::NewGlobalRef");
	//TRACE1((long)a0);
	jobject res;
	JNIEnv* env = getJNIEnv();
	res = env->functions->NewGlobalRef(env, a0);
	//TRACE1((long)res); //, JPJni::getClassName(a0).getSimpleName());
	return res;
	//TRACE_OUT;
}



bool JPJavaEnv::ExceptionCheck()
{
	JNIEnv* env = getJNIEnv();
	if (env != NULL)
	{
		return (env->functions->ExceptionCheck(env) ? true : false);
	}
	return false;
}

void JPJavaEnv::ExceptionDescribe()
{
	JNIEnv* env = getJNIEnv();
	env->functions->ExceptionDescribe(env);
}

void JPJavaEnv::ExceptionClear()
{
	JNIEnv* env = getJNIEnv();
	if (env != NULL)
	{
		env->functions->ExceptionClear(env);
	}
}

jint JPJavaEnv::AttachCurrentThread()
{
	// TODO find a way to get A JVM once JPJavaEnv is not a singleton anymore ...
	JNIEnv* env;
	jint res = jvm->functions->AttachCurrentThread(jvm, (void**)&env, NULL);
	JAVA_CHECK("AttachCurrentThread");
	return res;
}

jint JPJavaEnv::AttachCurrentThreadAsDaemon()
{
	// TODO find a way to get A JVM once JPJavaEnv is not a singleton anymore ...
	JNIEnv* env;
	jint res = jvm->functions->AttachCurrentThreadAsDaemon(jvm, (void**)&env, NULL);
	JAVA_CHECK("AttachCurrentThreadAsDaemon");
	return res;
}

bool JPJavaEnv::isThreadAttached()
{
	return JPEnv::getJava()->getJNIEnv() != NULL;
}


jint JPJavaEnv::DetachCurrentThread()
{
	return jvm->functions->DetachCurrentThread(jvm);
	
}

jint JPJavaEnv::GetEnv(JNIEnv** env)
{
	if (jvm == NULL)
	{
		*env = NULL;
		return JNI_EDETACHED;
	}

	// This function must not be put in GOTO_EXTERNAL/RETURN_EXTERNAL because it is called from WITHIN such a block already
	jint res;
	res = jvm->functions->GetEnv(jvm, (void**)env, JNI_VERSION_1_2);
	return res;
}

jint JPJavaEnv::ThrowNew(jclass clazz, const char* msg)
{
	JNIEnv* env = getJNIEnv();
	return env->functions->ThrowNew(env, clazz, msg);
}

jint JPJavaEnv::Throw(jthrowable th)
{
	JNIEnv* env = getJNIEnv();
	return env->functions->Throw(env, th);	
}

jthrowable JPJavaEnv::ExceptionOccurred()
{   
	jthrowable res;
    JNIEnv* env = getJNIEnv();

	res = env->functions->ExceptionOccurred(env);
 
    return res;
}

jobject JPJavaEnv::NewDirectByteBuffer(void* address, jlong capacity)
{
	TRACE_IN("JPJavaEnv::NewDirectByteBuffer");
	JNIEnv* env = getJNIEnv(); 
    jobject res = env->functions->NewDirectByteBuffer(env, address, capacity);
    JAVA_CHECK("NewDirectByteBuffer");
	TRACE1((long)res);
    return res;	
	TRACE_OUT;
}

jobject JPJavaEnv::NewObjectA(jclass a0, jmethodID a1, jvalue* a2)
{     jobject res;

    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->AllocObject(env, a0);
	JAVA_CHECK("NewObjectA");

	env->functions->CallVoidMethodA(env, res, a1, a2);

	if (ExceptionCheck())
	{
		DeleteLocalRef(res);
	}

    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("NewObjectA");
    return res;

}

jobject JPJavaEnv::NewObject(jclass a0, jmethodID a1)
{     jobject res;
    JNIEnv* env = getJNIEnv();
    void* _save = JPEnv::getHost()->gotoExternal();

	res = env->functions->AllocObject(env, a0);
	JAVA_CHECK("NewObject");

	env->functions->CallVoidMethod(env, res, a1);

	if (ExceptionCheck())
	{
		DeleteLocalRef(res);
	}

    JPEnv::getHost()->returnExternal(_save);
    JAVA_CHECK("NewObject");
    return res;
}
