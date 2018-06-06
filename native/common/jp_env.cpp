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
#include <Python.h>
#include <jpype.h>

/*
 * FIXME: use a less coupled way to call PyGILState_Ensure/Release()
 * in JPCleaner::~JPCleaner() if we wan't to target a non Python
 * implementation.
 */

//AT's on porting:
//  1) the original definition of global static object leads to crashing
//on HP-UX platform. Cause: it is suspected to be an undefined order of initialization of static objects
//
//  2) TODO: in any case, use of static objects may impose problems in multi-threaded environment.
//Therefore, they must be guarded with a mutex.
namespace { // impl details
	HostEnvironment* s_Host = NULL;
	JavaVM*          s_JavaVM = NULL;

	jint (JNICALL *CreateJVM_Method)(JavaVM **pvm, void **penv, void *args);
	jint (JNICALL *GetCreatedJVMs_Method)(JavaVM **pvm, jsize size, jsize* nVms);

	bool s_ConvertStringObjects=true;
}

/*****************************************************************************/   
// Platform handles the differences in dealing with shared libraries 
// on windows and unix variants.

#ifdef WIN32
	#include "jp_platform_win32.h"
	#define  PLATFORM_ADAPTER Win32PlatformAdapter
#else
	#include "jp_platform_linux.h"
	#define  PLATFORM_ADAPTER LinuxPlatformAdapter  
#endif


#define USE_JNI_VERSION JNI_VERSION_1_4

namespace {
JPPlatformAdapter* GetAdapter()
{
	static JPPlatformAdapter* adapter = new PLATFORM_ADAPTER();
	return adapter;
}


/*****************************************************************************/   
// Helper classes

/** Call a java jni function with error checking.
 *
 * This version should be used when the java call is a fixed length of time
 * to complete.
 */
class JPCall
{
    	JNIEnv* _env;
	const char* _msg;
public:
	JPCall(JPJavaFrame& frame, const char* msg)
	{
		_env = frame.getEnv();
		_msg = msg;
	}

	~JPCall() throw(JavaException)
	{
		// This is a throw in destructor which is only allowed on an exception safe code pattern
	        if (_env->functions->ExceptionCheck(_env)==JNI_TRUE)
		{
			RAISE(JavaException, _msg);
		}
	}
};

/** Call a java jni function with error checking releasing the host lock.A
 *
 * This version should be used if the call is indeterminate length of time to 
 * complete.  Thus we need to free up the global resource lock.
 */
class JPCallExternal
{
    	JNIEnv* _env;
	void* _save;
	const char* _msg;
public:
	JPCallExternal(JPJavaFrame& frame, const char* msg)
	{
		_env = frame.getEnv();
    		_save = JPEnv::getHost()->gotoExternal();
		_msg = msg;
	}

	~JPCallExternal() throw(JavaException)
	{
    		JPEnv::getHost()->returnExternal(_save);
		// This is a throw in destructor which is only allowed on an exception safe code pattern
	        if (_env->functions->ExceptionCheck(_env)==JNI_TRUE)
		{
			RAISE(JavaException, _msg);
		}
	}

	operator JNIEnv*()
	{
		return _env;
	}

	JNIEnv* operator->()
	{ 
		return _env;
	}
};

} // default namespace 

/*****************************************************************************/   

// JPEnv is currently a holder for JavaEnv and HostEnv, but this
// is not a necessary abstraction.


HostEnvironment* JPEnv::getHost()
{
	return s_Host;
}

bool JPEnv::isInitialized()
{
	return s_JavaVM!=NULL && getHost() != NULL;
}

/**
	throw a JPypeException if the JVM is not started
*/
void JPEnv::assertJVMRunning(const char* function, const char* file, int line)
{
	// FIXME fit function names into raise
	if (! JPEnv::isInitialized())
	{
		throw JPypeException("Java Virtual Machine is not running", function, file, line);
	}
}

void JPEnv::init(HostEnvironment* hostEnv)
{
	s_Host = hostEnv;
	
	JPTypeName::init();  
}

void loadEntryPoints(const string& path)
{
	// Load symbols from the shared library	
	GetAdapter()->loadLibrary((char*)path.c_str());
	CreateJVM_Method = (jint (JNICALL *)(JavaVM ** ,void ** ,void *))GetAdapter()->getSymbol("JNI_CreateJavaVM");
	GetCreatedJVMs_Method = (jint (JNICALL *)(JavaVM ** , jsize, jsize*))GetAdapter()->getSymbol("JNI_GetCreatedJavaVMs");
}


void JPEnv::setConvertStringObjects(bool flag)
{
	s_ConvertStringObjects = flag;
}

bool JPEnv::getConvertStringObjects()
{
	return s_ConvertStringObjects;
}
	
void JPEnv::loadJVM(const string& vmPath, char ignoreUnrecognized, const StringVector& args)
{
	TRACE_IN("JPEnv::loadJVM");

	// Get the entry points in the shared library
	loadEntryPoints(vmPath);

	JavaVMInitArgs jniArgs;
	jniArgs.options = NULL;
	
	// prepare this ...
	jniArgs.version = USE_JNI_VERSION;
	jniArgs.ignoreUnrecognized = ignoreUnrecognized;

	jniArgs.nOptions = (jint)args.size();
	jniArgs.options = (JavaVMOption*)malloc(sizeof(JavaVMOption)*jniArgs.nOptions);
	memset(jniArgs.options, 0, sizeof(JavaVMOption)*jniArgs.nOptions);
	for (int i = 0; i < jniArgs.nOptions; i++)
	{
		jniArgs.options[i].optionString = (char*)args[i].c_str();
	}
	JPEnv::CreateJavaVM((void*)&jniArgs);
	free(jniArgs.options);

	if (s_JavaVM == NULL) 
	{
		TRACE1("Unable to start");
		RAISE(JPypeException, "Unable to start JVM");
	}

	TRACE1("Initialize");
	JPJni::init();
	JPTypeManager::init();
	JPReferenceQueue::init();
	JPProxy::init();
	TRACE_OUT;
}

void JPEnv::shutdown()
{
	TRACE_IN("JPEnv::shutdown");
	if (s_JavaVM == NULL)
		RAISE(JPypeException, "Attempt to shutdown without a live JVM");

	// Reference queue has to be be shutdown first as we need to close resources
	JPReferenceQueue::shutdown();

	// Then the type manager has to be shut down so that we can't create any more classes
	JPTypeManager::shutdown();

	// Wait for all non-demon threads to terminate
	// DestroyJVM is rather misnamed.  It is simply a wait call
	// FIXME our reference queue thunk does not appear to have properly set 
	// as daemon so we hang here
	TRACE1("Destroy JVM");
//	s_JavaVM->functions->DestroyJavaVM(s_JavaVM);

	// unload the jvm library
	TRACE1("Unload JVM");
	GetAdapter()->unloadLibrary();
	s_JavaVM = NULL;
	TRACE_OUT;
}


void JPEnv::CreateJavaVM(void* arg)
{
	TRACE_IN("JPEnv::CreateJavaVM");
	s_JavaVM = NULL;
	JNIEnv* env; 
	CreateJVM_Method(&s_JavaVM, (void**)&env, arg);
	TRACE_OUT;
}

void JPEnv::attachJVM(const string& vmPath)
{
	TRACE_IN("JPEnv::attachJVM");

	// Get the entry points in the shared library
	loadEntryPoints(vmPath);

	JPEnv::GetCreatedJavaVM(); 
	
	if (s_JavaVM == NULL) {
		RAISE(JPypeException, "Unable to attach to JVM");
	}

	JPJni::init();
	JPTypeManager::init();
	JPReferenceQueue::init();
	JPProxy::init();

	TRACE_OUT;
}

void JPEnv::GetCreatedJavaVM()  
{
	// TODO: because we're returning NULL, jpype.attach() wont work.
	/*
	TRACE_IN("JPEnv::GetCreatedJavaVM");

	jsize numVms;
	
	jint ret = GetCreatedJVMs_Method(&s_JavaVM, 1, &numVms);
	if (ret < 0)
	{
		return NULL;
	}
	
	TRACE_OUT;
	*/
}

/*****************************************************************************/   
// Thread code

void JPEnv::attachCurrentThread()
{
	JNIEnv* env;
	jint res = s_JavaVM->functions->AttachCurrentThread(s_JavaVM, (void**)&env, NULL);
	if (res!=JNI_OK)
		RAISE(JPypeException, "Unable to attach to thread");
}

void JPEnv::attachCurrentThreadAsDaemon()  
{
	JNIEnv* env;
	jint res = s_JavaVM->functions->AttachCurrentThreadAsDaemon(s_JavaVM, (void**)&env, NULL);
	if (res!=JNI_OK)
		RAISE(JPypeException, "Unable to attach to thread as daemon");
}

bool JPEnv::isThreadAttached()
{
	JNIEnv* env;
	return JNI_OK != s_JavaVM->functions->GetEnv(s_JavaVM, (void**)&env, USE_JNI_VERSION);
}


void JPEnv::detachCurrentThread()
{
	s_JavaVM->functions->DetachCurrentThread(s_JavaVM);
}

/*****************************************************************************/   
// Local frames represent the JNIEnv for memory handling all java
// resources should be created within them.
//
JPJavaFrame::JPJavaFrame(JNIEnv* p_env, int i)
	: env(p_env), attached(false), popped(false)
{
        // Create a memory managment frame to live in	
	env->functions->PushLocalFrame(env, i);
}


JPJavaFrame::JPJavaFrame(int i)
{
	jint res;
	attached = false;
	if (s_JavaVM == NULL)
	{
		int *i=0;
		*i=0;

		RAISE(JPypeException, "JVM is null");
	}

	// Get the enviroment 
	res = s_JavaVM->functions->GetEnv(s_JavaVM, (void**)&env, USE_JNI_VERSION);

	// If we don't have an enviroment then we are in a thread, so we must attach
	if (res == JNI_EDETACHED)
	{
		res = s_JavaVM->functions->AttachCurrentThread(s_JavaVM, (void**)&env, NULL);
		if (res != JNI_OK)
			RAISE(JPypeException, "Unable to attach to local thread");

		attached = true;
	}

	popped = false;

        // Create a memory managment frame to live in	
	env->functions->PushLocalFrame(env, i);
}

jobject JPJavaFrame::keep(jobject obj)
{
	popped=true;
	return env->functions->PopLocalFrame(env, obj);
}

JPJavaFrame::~JPJavaFrame()
{
	// Check if we have already closed the frame.
	if (!popped)
	{
		env->functions->PopLocalFrame(env, NULL);
	}
	
	// Check if we were created from an detached thread
	if (attached)
	{
// This is unfortunately not safe to do as this will lose all local references including those we want to return.
//		s_JavaVM->functions->DetachCurrentThread(s_JavaVM);
	}
}

void JPJavaFrame::DeleteLocalRef(jobject obj)
{
	MTRACE_IN("JPJavaFrame::DeleteLocalRef");
	MTRACE1((long)obj);
	env->functions->DeleteLocalRef(env, obj);
	MTRACE_OUT;
}

void JPJavaFrame::DeleteGlobalRef(jobject obj)
{
	MTRACE_IN("JPJavaFrame::DeleteGlobalRef");
	MTRACE1((long)obj);
	env->functions->DeleteGlobalRef(env, obj);
	MTRACE_OUT;
}

jobject JPJavaFrame::NewLocalRef(jobject a0)
{
	MTRACE_IN("JPJavaFrame::NewLocalRef");
	jobject res = env->functions->NewLocalRef(env, a0);
	MTRACE3((long)a0, "=>", (long)res); //, JPJni::getClassName(a0).getSimpleName());
	return res;
	MTRACE_OUT;
}

jobject JPJavaFrame::NewGlobalRef(jobject a0)
{
	MTRACE_IN("JPJavaFrame::NewGlobalRef");
	jobject res = env->functions->NewGlobalRef(env, a0);
	MTRACE3((long)a0, "=>", (long)res); //, JPJni::getClassName(a0).getSimpleName());
	return res;
	MTRACE_OUT;
}

void JPJavaFrame::ReleaseGlobalRef(jobject a0)
{
	// Check if the JVM is already shutdown
	if (s_JavaVM == NULL)
		return;
	JPJavaFrame frame;
	frame.DeleteGlobalRef(a0);
}
	
/*****************************************************************************/   
// Exceptions

bool JPJavaFrame::ExceptionCheck()
{
	return (env->functions->ExceptionCheck(env) ? true : false);
}

void JPJavaFrame::ExceptionDescribe()
{
	env->functions->ExceptionDescribe(env);
}

void JPJavaFrame::ExceptionClear()
{
	env->functions->ExceptionClear(env);
}

jint JPJavaFrame::ThrowNew(jclass clazz, const char* msg)
{
	return env->functions->ThrowNew(env, clazz, msg);
}

jint JPJavaFrame::Throw(jthrowable th)
{
	return env->functions->Throw(env, th);	
}

jthrowable JPJavaFrame::ExceptionOccurred()
{
	jthrowable res;

	res = env->functions->ExceptionOccurred(env);
#ifdef TRACING
	if(res) {
		env->functions->ExceptionDescribe(env);
	}
#endif
	return res;
}

/*****************************************************************************/   

#define JAVA_CHECK(msg)  if (ExceptionCheck()) { RAISE(JavaException, msg); }

jobject JPJavaFrame::NewObjectA(jclass a0, jmethodID a1, jvalue* a2)
{ 
	jobject res;
	void* _save = JPEnv::getHost()->gotoExternal();

	// Allocate the object
	res = env->functions->AllocObject(env, a0);
	JAVA_CHECK("NewObjectA");

	// Initialize the object
	env->functions->CallVoidMethodA(env, res, a1, a2);
	JPEnv::getHost()->returnExternal(_save);
	JAVA_CHECK("NewObjectA");
	return res;
}

jobject JPJavaFrame::NewObject(jclass a0, jmethodID a1)
{ 
	jobject res;
	void* _save = JPEnv::getHost()->gotoExternal();

	// Allocate the object
	res = env->functions->AllocObject(env, a0);
	JAVA_CHECK("NewObject");

	// Initialize the object
	env->functions->CallVoidMethod(env, res, a1);
	JPEnv::getHost()->returnExternal(_save);
	JAVA_CHECK("NewObject");
	return res;
}

jobject JPJavaFrame::NewDirectByteBuffer(void* address, jlong capacity)
{
	JPCall call(*this, "NewDirectByteBuffer");
	return env->functions->NewDirectByteBuffer(env, address, capacity);
}

void* JPJavaFrame::GetPrimitiveArrayCritical(jarray array, jboolean *isCopy)
{
	JPCall call(*this, "GetPrimitiveArrayCritical");
	return env->functions->GetPrimitiveArrayCritical(env, array, isCopy);
}

void JPJavaFrame::ReleasePrimitiveArrayCritical(jarray array, void *carray, jint mode)
{
	JPCall call(*this, "ReleasePrimitiveArrayCritical");
	env->functions->ReleasePrimitiveArrayCritical(env, array, carray, mode);
}

/*****************************************************************************/   

jbyte JPJavaFrame::GetStaticByteField(jclass clazz, jfieldID fid) 
{
	JPCall call(*this, "GetStaticByteField");
	return env->functions->GetStaticByteField(env, clazz, fid);
}

jbyte JPJavaFrame::GetByteField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetByteField");
	return env->functions->GetByteField(env, clazz, fid);
}

void JPJavaFrame::SetStaticByteField(jclass clazz, jfieldID fid, jbyte val)
{
	JPCall call(*this, "SetStaticByteField");
	env->functions->SetStaticByteField(env, clazz, fid, val);
}

void JPJavaFrame::SetByteField(jobject clazz, jfieldID fid, jbyte val)
{
	JPCall call(*this, "SetByteField"); 
	env->functions->SetByteField(env, clazz, fid, val);
}

jbyte JPJavaFrame::CallStaticByteMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallStaticByteMethodA");
	return env->functions->CallStaticByteMethodA(env, clazz, mid, val); 
}

jbyte JPJavaFrame::CallStaticByteMethod(jclass clazz, jmethodID mid)
{
	JPCallExternal call(*this, "CallStaticByteMethodA");
	return env->functions->CallStaticByteMethod(env, clazz, mid); 
}

jbyte JPJavaFrame::CallByteMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallByteMethodA");
	return env->functions->CallByteMethodA(env, obj, mid, val); 
}

jbyte JPJavaFrame::CallByteMethod(jobject obj, jmethodID mid)
{
	JPCallExternal call(*this, "CallByteMethod");
	return env->functions->CallByteMethod(env, obj, mid); 
}

jbyte JPJavaFrame::CallNonvirtualByteMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallNonvirtualByteMethodA");
	return env->functions->CallNonvirtualByteMethodA(env, obj, claz, mid, val); 
}

jbyte JPJavaFrame::CallNonvirtualByteMethod(jobject obj, jclass claz, jmethodID mid)
{
	JPCallExternal call(*this, "CallNonvirtualByteMethod");
	return env->functions->CallNonvirtualByteMethod(env, obj, claz, mid); 
}

jshort JPJavaFrame::GetStaticShortField(jclass clazz, jfieldID fid) 
{
	JPCall call(*this, "GetStaticShortField");
	return env->functions->GetStaticShortField(env, clazz, fid);
}

jshort JPJavaFrame::GetShortField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetShortField");
	return env->functions->GetShortField(env, clazz, fid);
}

void JPJavaFrame::SetStaticShortField(jclass clazz, jfieldID fid, jshort val)
{
	JPCall call(*this, "SetStaticShortField");
	env->functions->SetStaticShortField(env, clazz, fid, val);
}

void JPJavaFrame::SetShortField(jobject clazz, jfieldID fid, jshort val)
{
	JPCall call(*this, "SetShortField"); 
	env->functions->SetShortField(env, clazz, fid, val);
}

jshort JPJavaFrame::CallStaticShortMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallStaticShortMethodA");
	return env->functions->CallStaticShortMethodA(env, clazz, mid, val); 
}

jshort JPJavaFrame::CallStaticShortMethod(jclass clazz, jmethodID mid)
{
	JPCallExternal call(*this, "CallStaticShortMethod");
	return env->functions->CallStaticShortMethod(env, clazz, mid); 
}

jshort JPJavaFrame::CallShortMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallShortMethodA");
	return env->functions->CallShortMethodA(env, obj, mid, val); 
}

jshort JPJavaFrame::CallShortMethod(jobject obj, jmethodID mid)
{
	JPCallExternal call(*this, "CallShortMethod");
	return env->functions->CallShortMethod(env, obj, mid); 
}

jshort JPJavaFrame::CallNonvirtualShortMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallNonvirtualShortMethodA");
	return env->functions->CallNonvirtualShortMethodA(env, obj, claz, mid, val); 
}

jshort JPJavaFrame::CallNonvirtualShortMethod(jobject obj, jclass claz, jmethodID mid)
{
	JPCallExternal call(*this, "CallNonvirtualShortMethod");
	return env->functions->CallNonvirtualShortMethod(env, obj, claz, mid); 
}

jint JPJavaFrame::GetStaticIntField(jclass clazz, jfieldID fid) 
{
	JPCall call(*this, "GetStaticIntField");
	return env->functions->GetStaticIntField(env, clazz, fid);
}

jint JPJavaFrame::GetIntField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetIntField");
	return env->functions->GetIntField(env, clazz, fid);
}

void JPJavaFrame::SetStaticIntField(jclass clazz, jfieldID fid, jint val)
{
	JPCall call(*this, "SetStaticIntField");
	env->functions->SetStaticIntField(env, clazz, fid, val);
}

void JPJavaFrame::SetIntField(jobject clazz, jfieldID fid, jint val)
{
	JPCall call(*this, "SetIntField"); 
	env->functions->SetIntField(env, clazz, fid, val);
}

jint JPJavaFrame::CallStaticIntMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallStaticIntMethodA");
	return env->functions->CallStaticIntMethodA(env, clazz, mid, val); 
}

jint JPJavaFrame::CallStaticIntMethod(jclass clazz, jmethodID mid)
{
	JPCallExternal call(*this, "CallStaticIntMethod");
	return env->functions->CallStaticIntMethod(env, clazz, mid); 
}

jint JPJavaFrame::CallIntMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallIntMethodA");
	return env->functions->CallIntMethodA(env, obj, mid, val); 
}

jint JPJavaFrame::CallIntMethod(jobject obj, jmethodID mid)
{
	JPCallExternal call(*this, "CallIntMethod");
	return env->functions->CallIntMethod(env, obj, mid); 
}

jint JPJavaFrame::CallNonvirtualIntMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallNonvirtualIntMethodA");
	return env->functions->CallNonvirtualIntMethodA(env, obj, claz, mid, val); 
}

jint JPJavaFrame::CallNonvirtualIntMethod(jobject obj, jclass claz, jmethodID mid)
{
	JPCallExternal call(*this, "CallNonvirtualIntMethod");
	return env->functions->CallNonvirtualIntMethod(env, obj, claz, mid); 
}

jlong JPJavaFrame::GetStaticLongField(jclass clazz, jfieldID fid) 
{
	JPCall call(*this, "GetStaticLongField");
	return env->functions->GetStaticLongField(env, clazz, fid);
}

jlong JPJavaFrame::GetLongField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetLongField");
	return env->functions->GetLongField(env, clazz, fid);
}

void JPJavaFrame::SetStaticLongField(jclass clazz, jfieldID fid, jlong val)
{
	JPCall call(*this, "SetStaticLongField");
	env->functions->SetStaticLongField(env, clazz, fid, val);
}

void JPJavaFrame::SetLongField(jobject clazz, jfieldID fid, jlong val)
{
	JPCall call(*this, "SetLongField"); 
	env->functions->SetLongField(env, clazz, fid, val);
}

jlong JPJavaFrame::CallStaticLongMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallStaticLongMethodA");
	return env->functions->CallStaticLongMethodA(env, clazz, mid, val); 
}

jlong JPJavaFrame::CallStaticLongMethod(jclass clazz, jmethodID mid)
{
	JPCallExternal call(*this, "CallStaticLongMethod");
	return env->functions->CallStaticLongMethod(env, clazz, mid); 
}

jlong JPJavaFrame::CallLongMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallLongMethodA");
	return env->functions->CallLongMethodA(env, obj, mid, val); 
}

jlong JPJavaFrame::CallLongMethod(jobject obj, jmethodID mid)
{
	JPCallExternal call(*this, "CallLongMethod");
	return env->functions->CallLongMethod(env, obj, mid); 
}

jlong JPJavaFrame::CallNonvirtualLongMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallNonvirtualLongMethodA");
	return env->functions->CallNonvirtualLongMethodA(env, obj, claz, mid, val); 
}

jlong JPJavaFrame::CallNonvirtualLongMethod(jobject obj, jclass claz, jmethodID mid)
{
	JPCallExternal call(*this, "CallNonvirtualLongMethod");
	return env->functions->CallNonvirtualLongMethod(env, obj, claz, mid); 
}

jfloat JPJavaFrame::GetStaticFloatField(jclass clazz, jfieldID fid) 
{
	JPCall call(*this, "GetStaticFloatField");
	return env->functions->GetStaticFloatField(env, clazz, fid);
}

jfloat JPJavaFrame::GetFloatField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetFloatField");
	return env->functions->GetFloatField(env, clazz, fid);
}

void JPJavaFrame::SetStaticFloatField(jclass clazz, jfieldID fid, jfloat val)
{
	JPCall call(*this, "SetStaticFloatField");
	env->functions->SetStaticFloatField(env, clazz, fid, val);
}

void JPJavaFrame::SetFloatField(jobject clazz, jfieldID fid, jfloat val)
{
	JPCall call(*this, "SetFloatField"); 
	env->functions->SetFloatField(env, clazz, fid, val);
}

jfloat JPJavaFrame::CallStaticFloatMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallStaticFloatMethodA");
	return env->functions->CallStaticFloatMethodA(env, clazz, mid, val); 
}

jfloat JPJavaFrame::CallStaticFloatMethod(jclass clazz, jmethodID mid)
{
	JPCallExternal call(*this, "CallStaticFloatMethod");
	return env->functions->CallStaticFloatMethod(env, clazz, mid); 
}

jfloat JPJavaFrame::CallFloatMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallFloatMethodA");
	return env->functions->CallFloatMethodA(env, obj, mid, val); 
}

jfloat JPJavaFrame::CallFloatMethod(jobject obj, jmethodID mid)
{
	JPCallExternal call(*this, "CallFloatMethod");
	return env->functions->CallFloatMethod(env, obj, mid); 
}

jfloat JPJavaFrame::CallNonvirtualFloatMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallNonvirtualFloatMethodA");
	return env->functions->CallNonvirtualFloatMethodA(env, obj, claz, mid, val); 
}

jfloat JPJavaFrame::CallNonvirtualFloatMethod(jobject obj, jclass claz, jmethodID mid)
{
	JPCallExternal call(*this, "CallNonvirtualFloatMethod");
	return env->functions->CallNonvirtualFloatMethod(env, obj, claz, mid); 
}

jdouble JPJavaFrame::GetStaticDoubleField(jclass clazz, jfieldID fid) 
{
	JPCall call(*this, "GetStaticDoubleField");
	return env->functions->GetStaticDoubleField(env, clazz, fid);
}

jdouble JPJavaFrame::GetDoubleField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetDoubleField");
	return env->functions->GetDoubleField(env, clazz, fid);
}

void JPJavaFrame::SetStaticDoubleField(jclass clazz, jfieldID fid, jdouble val)
{
	JPCall call(*this, "SetStaticDoubleField");
	env->functions->SetStaticDoubleField(env, clazz, fid, val);
}

void JPJavaFrame::SetDoubleField(jobject clazz, jfieldID fid, jdouble val)
{
	JPCall call(*this, "SetDoubleField"); 
	env->functions->SetDoubleField(env, clazz, fid, val);
}

jdouble JPJavaFrame::CallStaticDoubleMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallStaticDoubleMethodA");
	return env->functions->CallStaticDoubleMethodA(env, clazz, mid, val); 
}

jdouble JPJavaFrame::CallStaticDoubleMethod(jclass clazz, jmethodID mid)
{
	JPCallExternal call(*this, "CallStaticDoubleMethod");
	return env->functions->CallStaticDoubleMethod(env, clazz, mid); 
}

jdouble JPJavaFrame::CallDoubleMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallDoubleMethodA");
	return env->functions->CallDoubleMethodA(env, obj, mid, val); 
}

jdouble JPJavaFrame::CallDoubleMethod(jobject obj, jmethodID mid)
{
	JPCallExternal call(*this, "CallDoubleMethod");
	return env->functions->CallDoubleMethod(env, obj, mid); 
}

jdouble JPJavaFrame::CallNonvirtualDoubleMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallNonvirtualDoubleMethodA");
	return env->functions->CallNonvirtualDoubleMethodA(env, obj, claz, mid, val); 
}

jdouble JPJavaFrame::CallNonvirtualDoubleMethod(jobject obj, jclass claz, jmethodID mid)
{
	JPCallExternal call(*this, "CallNonvirtualDoubleMethod");
	return env->functions->CallNonvirtualDoubleMethod(env, obj, claz, mid); 
}

jchar JPJavaFrame::GetStaticCharField(jclass clazz, jfieldID fid) 
{
	JPCall call(*this, "GetStaticCharField");
	return env->functions->GetStaticCharField(env, clazz, fid);
}

jchar JPJavaFrame::GetCharField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetCharField");
	return env->functions->GetCharField(env, clazz, fid);
}
void JPJavaFrame::SetStaticCharField(jclass clazz, jfieldID fid, jchar val)
{
	JPCall call(*this, "SetStaticCharField");
	env->functions->SetStaticCharField(env, clazz, fid, val);
}

void JPJavaFrame::SetCharField(jobject clazz, jfieldID fid, jchar val)
{
	JPCall call(*this, "SetCharField"); 
	env->functions->SetCharField(env, clazz, fid, val);
}

jchar JPJavaFrame::CallStaticCharMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallStaticCharMethodA");
	return env->functions->CallStaticCharMethodA(env, clazz, mid, val); 
}

jchar JPJavaFrame::CallStaticCharMethod(jclass clazz, jmethodID mid)
{
	JPCallExternal call(*this, "CallStaticCharMethod");
	return env->functions->CallStaticCharMethod(env, clazz, mid); 
}

jchar JPJavaFrame::CallCharMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallCharMethodA");
	return env->functions->CallCharMethodA(env, obj, mid, val); 
}

jchar JPJavaFrame::CallCharMethod(jobject obj, jmethodID mid)
{
	JPCallExternal call(*this, "CallCharMethod");
	return env->functions->CallCharMethod(env, obj, mid); 
}

jchar JPJavaFrame::CallNonvirtualCharMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallNonvirtualCharMethodA");
	return env->functions->CallNonvirtualCharMethodA(env, obj, claz, mid, val); 
}

jchar JPJavaFrame::CallNonvirtualCharMethod(jobject obj, jclass claz, jmethodID mid)
{
	JPCallExternal call(*this, "CallNonvirtualCharMethod");
	return env->functions->CallNonvirtualCharMethod(env, obj, claz, mid); 
}

jboolean JPJavaFrame::GetStaticBooleanField(jclass clazz, jfieldID fid) 
{
	JPCall call(*this, "GetStaticBooleanField");
	return env->functions->GetStaticBooleanField(env, clazz, fid);
}

jboolean JPJavaFrame::GetBooleanField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetBooleanField");
	return env->functions->GetBooleanField(env, clazz, fid);
}

void JPJavaFrame::SetStaticBooleanField(jclass clazz, jfieldID fid, jboolean val)
{
	JPCall call(*this, "SetStaticBooleanField");
	env->functions->SetStaticBooleanField(env, clazz, fid, val);
}

void JPJavaFrame::SetBooleanField(jobject clazz, jfieldID fid, jboolean val)
{
	JPCall call(*this, "SetBooleanField"); 
	env->functions->SetBooleanField(env, clazz, fid, val);
}

jboolean JPJavaFrame::CallStaticBooleanMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallStaticBooleanMethodA");
	return env->functions->CallStaticBooleanMethodA(env, clazz, mid, val); 
}

jboolean JPJavaFrame::CallStaticBooleanMethod(jclass clazz, jmethodID mid)
{
	JPCallExternal call(*this, "CallStaticBooleanMethod");
	return env->functions->CallStaticBooleanMethod(env, clazz, mid); 
}

jboolean JPJavaFrame::CallBooleanMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallBooleanMethodA");
	return env->functions->CallBooleanMethodA(env, obj, mid, val); 
}

jboolean JPJavaFrame::CallBooleanMethod(jobject obj, jmethodID mid)
{
	JPCallExternal call(*this, "CallBooleanMethod");
	return env->functions->CallBooleanMethod(env, obj, mid); 
}

jboolean JPJavaFrame::CallNonvirtualBooleanMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallNonvirtualBooleanMethodA");
	return env->functions->CallNonvirtualBooleanMethodA(env, obj, claz, mid, val); 
}

jboolean JPJavaFrame::CallNonvirtualBooleanMethod(jobject obj, jclass claz, jmethodID mid)
{
	JPCallExternal call(*this, "CallNonvirtualBooleanMethod");
	return env->functions->CallNonvirtualBooleanMethod(env, obj, claz, mid); 
}

jobject JPJavaFrame::GetStaticObjectField(jclass clazz, jfieldID fid) 
{
	JPCall call(*this, "GetStaticObjectField");
	return env->functions->GetStaticObjectField(env, clazz, fid);
}

jobject JPJavaFrame::GetObjectField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetObjectField");
	return env->functions->GetObjectField(env, clazz, fid);
}
void JPJavaFrame::SetStaticObjectField(jclass clazz, jfieldID fid, jobject val)
{
	JPCall call(*this, "SetStaticObjectField");
	env->functions->SetStaticObjectField(env, clazz, fid, val);
}

void JPJavaFrame::SetObjectField(jobject clazz, jfieldID fid, jobject val)
{
	JPCall call(*this, "SetObjectField"); 
	env->functions->SetObjectField(env, clazz, fid, val);
}

jobject JPJavaFrame::CallStaticObjectMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallStaticObjectMethodA");
	return env->functions->CallStaticObjectMethodA(env, clazz, mid, val); 
}

jobject JPJavaFrame::CallStaticObjectMethod(jclass clazz, jmethodID mid)
{
	JPCallExternal call(*this, "CallStaticObjectMethod");
	return env->functions->CallStaticObjectMethod(env, clazz, mid); 
}

jobject JPJavaFrame::CallObjectMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallObjectMethodA");
	return env->functions->CallObjectMethodA(env, obj, mid, val); 
}

jobject JPJavaFrame::CallObjectMethod(jobject obj, jmethodID mid)
{
	JPCallExternal call(*this, "CallObjectMethod");
	return env->functions->CallObjectMethod(env, obj, mid); 
}

jobject JPJavaFrame::CallNonvirtualObjectMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCallExternal call(*this, "CallNonvirtualObjectMethodA");
	return env->functions->CallNonvirtualObjectMethodA(env, obj, claz, mid, val); 
}

jobject JPJavaFrame::CallNonvirtualObjectMethod(jobject obj, jclass claz, jmethodID mid)
{
	JPCallExternal call(*this, "CallNonvirtualObjectMethod");
	return env->functions->CallNonvirtualObjectMethod(env, obj, claz, mid); 
}

jbyteArray JPJavaFrame::NewByteArray(jint len)
{
	JPCall call(*this, "NewByteArray");
	return env->functions->NewByteArray(env, len);
}

void JPJavaFrame::SetByteArrayRegion(jbyteArray array, int start, int len, jbyte* vals)
{
	JPCall call(*this, "SetByteArrayRegion");
	env->functions->SetByteArrayRegion(env, array, start, len, vals);
}

void JPJavaFrame::GetByteArrayRegion(jbyteArray array, int start, int len, jbyte* vals)
{
	JPCall call(*this, "GetByteArrayRegion");
	env->functions->GetByteArrayRegion(env, array, start, len, vals);
}

jbyte* JPJavaFrame::GetByteArrayElements(jbyteArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetByteArrayElements");
	return env->functions->GetByteArrayElements(env, array, isCopy);
}

void JPJavaFrame::ReleaseByteArrayElements(jbyteArray array, jbyte* v, jint mode)
{
	JPCall call(*this, "ReleaseByteArrayElements");
	env->functions->ReleaseByteArrayElements(env, array, v, mode);
}

jshortArray JPJavaFrame::NewShortArray(jint len)
{
	JPCall call(*this, "NewShortArray");
	return env->functions->NewShortArray(env, len);
}

void JPJavaFrame::SetShortArrayRegion(jshortArray array, int start, int len, jshort* vals)
{
	JPCall call(*this, "SetShortArrayRegion");
	env->functions->SetShortArrayRegion(env, array, start, len, vals);
}

void JPJavaFrame::GetShortArrayRegion(jshortArray array, int start, int len, jshort* vals)
{
	JPCall call(*this, "GetShortArrayRegion");
	env->functions->GetShortArrayRegion(env, array, start, len, vals);
}

jshort* JPJavaFrame::GetShortArrayElements(jshortArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetShortArrayElements");
	return env->functions->GetShortArrayElements(env, array, isCopy);
}

void JPJavaFrame::ReleaseShortArrayElements(jshortArray array, jshort* v, jint mode)
{
	JPCall call(*this, "ReleaseShortArrayElements");
	env->functions->ReleaseShortArrayElements(env, array, v, mode);
}

jintArray JPJavaFrame::NewIntArray(jint len)
{
	JPCall call(*this, "NewIntArray");
	return env->functions->NewIntArray(env, len);
}

void JPJavaFrame::SetIntArrayRegion(jintArray array, int start, int len, jint* vals)
{
	JPCall call(*this, "SetIntArrayRegion");
	env->functions->SetIntArrayRegion(env, array, start, len, vals);
}

void JPJavaFrame::GetIntArrayRegion(jintArray array, int start, int len, jint* vals)
{
	JPCall call(*this, "GetIntArrayRegion");
	env->functions->GetIntArrayRegion(env, array, start, len, vals);
}

jint* JPJavaFrame::GetIntArrayElements(jintArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetIntArrayElements");
	return env->functions->GetIntArrayElements(env, array, isCopy);
}

void JPJavaFrame::ReleaseIntArrayElements(jintArray array, jint* v, jint mode)
{
	JPCall call(*this, "ReleaseIntArrayElements");
	env->functions->ReleaseIntArrayElements(env, array, v, mode);
}

jlongArray JPJavaFrame::NewLongArray(jint len)
{
	JPCall call(*this, "NewLongArray");
	return env->functions->NewLongArray(env, len);
}

void JPJavaFrame::SetLongArrayRegion(jlongArray array, int start, int len, jlong* vals)
{
	JPCall call(*this, "SetLongArrayRegion");
	env->functions->SetLongArrayRegion(env, array, start, len, vals);
}

void JPJavaFrame::GetLongArrayRegion(jlongArray array, int start, int len, jlong* vals)
{
	JPCall call(*this, "GetLongArrayRegion");
	env->functions->GetLongArrayRegion(env, array, start, len, vals);
}

jlong* JPJavaFrame::GetLongArrayElements(jlongArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetLongArrayElements");
	return env->functions->GetLongArrayElements(env, array, isCopy);
}

void JPJavaFrame::ReleaseLongArrayElements(jlongArray array, jlong* v, jint mode)
{
	JPCall call(*this, "ReleaseLongArrayElements");
	env->functions->ReleaseLongArrayElements(env, array, v, mode);
}

jfloatArray JPJavaFrame::NewFloatArray(jint len)
{
	JPCall call(*this, "NewFloatArray");
	return env->functions->NewFloatArray(env, len);
}

void JPJavaFrame::SetFloatArrayRegion(jfloatArray array, int start, int len, jfloat* vals)
{
	JPCall call(*this, "SetFloatArrayRegion");
	env->functions->SetFloatArrayRegion(env, array, start, len, vals);
}

void JPJavaFrame::GetFloatArrayRegion(jfloatArray array, int start, int len, jfloat* vals)
{
	JPCall call(*this, "GetFloatArrayRegion");
	env->functions->GetFloatArrayRegion(env, array, start, len, vals);
}

jfloat* JPJavaFrame::GetFloatArrayElements(jfloatArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetFloatArrayElements");
	return env->functions->GetFloatArrayElements(env, array, isCopy);
}

void JPJavaFrame::ReleaseFloatArrayElements(jfloatArray array, jfloat* v, jint mode)
{
	JPCall call(*this, "ReleaseFloatArrayElements");
	env->functions->ReleaseFloatArrayElements(env, array, v, mode);
}

jdoubleArray JPJavaFrame::NewDoubleArray(jint len)
{
	JPCall call(*this, "NewDoubleArray");
	return env->functions->NewDoubleArray(env, len);
}

void JPJavaFrame::SetDoubleArrayRegion(jdoubleArray array, int start, int len, jdouble* vals)
{
	JPCall call(*this, "SetDoubleArrayRegion");
	env->functions->SetDoubleArrayRegion(env, array, start, len, vals);
}

void JPJavaFrame::GetDoubleArrayRegion(jdoubleArray array, int start, int len, jdouble* vals)
{
	JPCall call(*this, "GetDoubleArrayRegion");
	env->functions->GetDoubleArrayRegion(env, array, start, len, vals);
}

jdouble* JPJavaFrame::GetDoubleArrayElements(jdoubleArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetDoubleArrayElements");
	return env->functions->GetDoubleArrayElements(env, array, isCopy);
}

void JPJavaFrame::ReleaseDoubleArrayElements(jdoubleArray array, jdouble* v, jint mode)
{
	JPCall call(*this, "ReleaseDoubleArrayElements");
	env->functions->ReleaseDoubleArrayElements(env, array, v, mode);
}

jcharArray JPJavaFrame::NewCharArray(jint len)
{
	JPCall call(*this, "NewCharArray");
	return env->functions->NewCharArray(env, len);
}

void JPJavaFrame::SetCharArrayRegion(jcharArray array, int start, int len, jchar* vals)
{
	JPCall call(*this, "SetCharArrayRegion");
	env->functions->SetCharArrayRegion(env, array, start, len, vals);
}

void JPJavaFrame::GetCharArrayRegion(jcharArray array, int start, int len, jchar* vals)
{
	JPCall call(*this, "GetCharArrayRegion");
	env->functions->GetCharArrayRegion(env, array, start, len, vals);
}

jchar* JPJavaFrame::GetCharArrayElements(jcharArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetCharArrayElements");
	return env->functions->GetCharArrayElements(env, array, isCopy);
}

void JPJavaFrame::ReleaseCharArrayElements(jcharArray array, jchar* v, jint mode)
{
	JPCall call(*this, "ReleaseCharArrayElements");
	env->functions->ReleaseCharArrayElements(env, array, v, mode);
}

jbooleanArray JPJavaFrame::NewBooleanArray(jint len)
{
	JPCall call(*this, "NewBooleanArray");
	return env->functions->NewBooleanArray(env, len);
}

void JPJavaFrame::SetBooleanArrayRegion(jbooleanArray array, int start, int len, jboolean* vals)
{
	JPCall call(*this, "SetBooleanArrayRegion");
	env->functions->SetBooleanArrayRegion(env, array, start, len, vals);
}

void JPJavaFrame::GetBooleanArrayRegion(jbooleanArray array, int start, int len, jboolean* vals)
{
	JPCall call(*this, "GetBooleanArrayRegion");
	env->functions->GetBooleanArrayRegion(env, array, start, len, vals);
}

jboolean* JPJavaFrame::GetBooleanArrayElements(jbooleanArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetBooleanArrayElements");
	return env->functions->GetBooleanArrayElements(env, array, isCopy);
}

void JPJavaFrame::ReleaseBooleanArrayElements(jbooleanArray array, jboolean* v, jint mode)
{
	JPCall call(*this, "ReleaseBooleanArrayElements");
	env->functions->ReleaseBooleanArrayElements(env, array, v, mode);
}

int JPJavaFrame::MonitorEnter(jobject a0)
{
	JPCallExternal call(*this, "MonitorEnter");
	return env->functions->MonitorEnter(env, a0);
}

int JPJavaFrame::MonitorExit(jobject a0)
{
	JPCallExternal call(*this, "MonitorExit");
	return env->functions->MonitorExit(env, a0);
}

jmethodID JPJavaFrame::FromReflectedMethod(jobject a0)
{
	JPCallExternal call(*this, "FromReflectedMethod");
	return env->functions->FromReflectedMethod(env, a0);
}

jfieldID JPJavaFrame::FromReflectedField(jobject a0)
{
	JPCallExternal call(*this, "FromReflectedField");
	return env->functions->FromReflectedField(env, a0);
}

jclass JPJavaFrame::FindClass(const char* a0)
{
	JPCallExternal call(*this, "FindClass");
	return env->functions->FindClass(env, a0);
}

jboolean JPJavaFrame::IsInstanceOf(jobject a0, jclass a1)
{
	JPCallExternal call(*this, "IsInstanceOf");
	return env->functions->IsInstanceOf(env, a0, a1);
}

jobjectArray JPJavaFrame::NewObjectArray(int a0, jclass a1, jobject a2)
{
	JPCallExternal call(*this, "NewObjectArray");
	return env->functions->NewObjectArray(env, a0, a1, a2);
}

void JPJavaFrame::SetObjectArrayElement(jobjectArray a0, int a1, jobject a2)
{ 
	JPCallExternal call(*this, "SetObjectArrayElement");
	env->functions->SetObjectArrayElement(env, a0, a1, a2);
}

void JPJavaFrame::CallStaticVoidMethodA(jclass a0, jmethodID a1, jvalue* a2)
{ 
	JPCallExternal call(*this, "CallStaticVoidMethodA");
	env->functions->CallStaticVoidMethodA(env, a0, a1, a2);
}

void JPJavaFrame::CallVoidMethodA(jobject a0, jmethodID a1, jvalue* a2)
{ 
	JPCallExternal call(*this, "CallVoidMethodA");
	env->functions->CallVoidMethodA(env, a0, a1, a2);
}

void JPJavaFrame::CallVoidMethod(jobject a0, jmethodID a1)
{ 
	JPCallExternal call(*this, "CallVoidMethod");
	env->functions->CallVoidMethod(env, a0, a1);
}

jboolean JPJavaFrame::IsAssignableFrom(jclass a0, jclass a1)
{
	JPCallExternal call(*this, "IsAssignableFrom");
	return env->functions->IsAssignableFrom(env, a0, a1);
}

jstring JPJavaFrame::NewString(const jchar* a0, int a1)
{
	JPCallExternal call(*this, "NewString");
	return env->functions->NewString(env, a0, a1);
}

jclass JPJavaFrame::GetSuperclass(jclass a0)
{
	JPCallExternal call(*this, "GetSuperclass");
	return env->functions->GetSuperclass(env, a0);
}

const char* JPJavaFrame::GetStringUTFChars(jstring a0, jboolean* a1)
{
	JPCallExternal call(*this, "GetStringUTFChars");
	return env->functions->GetStringUTFChars(env, a0, a1);
}

void JPJavaFrame::ReleaseStringUTFChars(jstring a0, const char* a1)
{ 
	JPCallExternal call(*this, "ReleaseStringUTFChars");
	env->functions->ReleaseStringUTFChars(env, a0, a1);
}

jsize JPJavaFrame::GetArrayLength(jarray a0)
{
	JPCallExternal call(*this, "GetArrayLength");
	return env->functions->GetArrayLength(env, a0);
}

jobject JPJavaFrame::GetObjectArrayElement(jobjectArray a0, int a1)
{
	JPCallExternal call(*this, "GetObjectArrayElement");
	return env->functions->GetObjectArrayElement(env, a0, a1);
}

jclass JPJavaFrame::GetObjectClass(jobject a0)
{
	JPCallExternal call(*this, "GetObjectClass");
	return env->functions->GetObjectClass(env, a0);
}

jmethodID JPJavaFrame::GetMethodID(jclass a0, const char* a1, const char* a2)
{
	JPCallExternal call(*this, "GetMethodID");
	return env->functions->GetMethodID(env, a0, a1, a2);
}

jmethodID JPJavaFrame::GetStaticMethodID(jclass a0, const char* a1, const char* a2)
{
	JPCallExternal call(*this, "GetStaticMethodID");
	return env->functions->GetStaticMethodID(env, a0, a1, a2);
}

jfieldID JPJavaFrame::GetFieldID(jclass a0, const char* a1, const char* a2)
{
	JPCallExternal call(*this, "GetFieldID");
	return env->functions->GetFieldID(env, a0, a1, a2);
}

jfieldID JPJavaFrame::GetStaticFieldID(jclass a0, const char* a1,  const char* a2)
{
	JPCallExternal call(*this, "GetStaticFieldID");
	return env->functions->GetStaticFieldID(env, a0, a1, a2);
}

const jchar* JPJavaFrame::GetStringChars(jstring a0, jboolean* a1)
{
	JPCallExternal call(*this, "GetStringChars");
	return env->functions->GetStringChars(env, a0, a1);
}

void JPJavaFrame::ReleaseStringChars(jstring a0, const jchar* a1)
{
	JPCallExternal call(*this, "ReleaseStringChars");
	env->functions->ReleaseStringChars(env, a0, a1);
}

jsize JPJavaFrame::GetStringLength(jstring a0)
{
	JPCallExternal call(*this, "GetStringLength");
	return env->functions->GetStringLength(env, a0);
}

jclass JPJavaFrame::DefineClass(const char* a0, jobject a1, const jbyte* a2, jsize a3)
{     
	JPCallExternal call(*this, "DefineClass");
	return env->functions->DefineClass(env, a0, a1, a2, a3);
}

jint JPJavaFrame::RegisterNatives(jclass a0, const JNINativeMethod* a1, jint a2)
{   
	JPCallExternal call(*this, "RegisterNatives");
	return env->functions->RegisterNatives(env, a0, a1, a2);
}
