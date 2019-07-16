/*****************************************************************************
   Copyright 2019 Karl Einar Nelson

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

JPResource::~JPResource()
{
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
namespace
{

	JPPlatformAdapter* GetAdapter()
	{
		static JPPlatformAdapter* adapter = new PLATFORM_ADAPTER();
		return adapter;
	}
}


#define USE_JNI_VERSION JNI_VERSION_1_4

JPContext::JPContext()
{
	printf("New JVM %p\n", this);
	m_JavaVM = 0;
	_void = 0;
	_boolean = 0;
	_byte = 0;
	_char = 0;
	_short = 0;
	_int = 0;
	_long = 0;
	_float = 0;
	_double = 0;

	_java_lang_Void = 0;
	_java_lang_Boolean = 0;
	_java_lang_Byte = 0;
	_java_lang_Char = 0;
	_java_lang_Short = 0;
	_java_lang_Integer = 0;
	_java_lang_Long = 0;
	_java_lang_Float = 0;
	_java_lang_Double = 0;

	_java_lang_Object = 0;
	_java_lang_Class = 0;
	_java_lang_String = 0;

	m_TypeFactory = 0;
	m_TypeManager = 0;
	m_ClassLoader = 0;
	m_ReferenceQueue = 0;
	m_ProxyFactory = 0;

	m_Object_ToStringID = 0;
	m_ShutdownMethodID = 0;
	m_IsShutdown = false;
	m_IsInitialized = false;
	m_Host = 0;
}

JPContext::~JPContext()
{
}

bool JPContext::isRunning()
{
	if (m_JavaVM == NULL || !m_IsInitialized)
	{
		return false;
	}
	return true;
}

/**
	throw a JPypeException if the JVM is not started
 */
void JPContext::assertJVMRunning(const JPStackInfo& info)
{
	if (m_JavaVM == NULL || !m_IsInitialized)
	{
		printf("THROW\n");
		throw JPypeException(JPError::_runtime_error, "Java Virtual Machine is not running", info);
	}
}

void JPContext::loadEntryPoints(const string& path)
{
	// Load symbols from the shared library
	GetAdapter()->loadLibrary((char*) path.c_str());
	CreateJVM_Method = (jint(JNICALL *)(JavaVM **, void **, void *))GetAdapter()->getSymbol("JNI_CreateJavaVM");
	GetCreatedJVMs_Method = (jint(JNICALL *)(JavaVM **, jsize, jsize*))GetAdapter()->getSymbol("JNI_GetCreatedJavaVMs");
}

void JPContext::startJVM(const string& vmPath, const StringVector& args,
			 char ignoreUnrecognized, char convertStrings)
{
	JP_TRACE_IN("JPContext::startJVM");

	m_ConvertStrings = convertStrings;

	// Get the entry points in the shared library
	loadEntryPoints(vmPath);

	// Pack the arguments
	JavaVMInitArgs jniArgs;
	jniArgs.options = NULL;

	// prepare this ...
	jniArgs.version = USE_JNI_VERSION;
	jniArgs.ignoreUnrecognized = ignoreUnrecognized;

	jniArgs.nOptions = (jint) args.size();
	jniArgs.options = (JavaVMOption*) malloc(sizeof (JavaVMOption) * jniArgs.nOptions);
	memset(jniArgs.options, 0, sizeof (JavaVMOption) * jniArgs.nOptions);
	for (int i = 0; i < jniArgs.nOptions; i++)
	{
		jniArgs.options[i].optionString = (char*) args[i].c_str();
	}

	// Launch the JVM
	JNIEnv* env;
	CreateJVM_Method(&m_JavaVM, (void**) &env, (void*) &jniArgs);
	free(jniArgs.options);

	if (m_JavaVM == NULL)
	{
		JP_TRACE("Unable to start");
		JP_RAISE_RUNTIME_ERROR("Unable to start JVM");
	}

	// Connect our resources to the JVM
	{
		JPJavaFrame frame(this);

		// After the JVM is created but before the context is started, we need
		// lo set up all the services that the context will need.
		JP_TRACE("Initialize");

		// We need these first because if anything goes south this is the first
		// thing that will get hit.
		jclass object = frame.FindClass("java/lang/Object");
		m_Object_ToStringID = frame.GetMethodID(object, "toString", "()Ljava/lang/String;");
		_java_lang_NoSuchMethodError = JPClassRef(this, (jclass) frame.FindClass("java/lang/NoSuchMethodError"));
		_java_lang_RuntimeException = JPClassRef(this, (jclass) frame.FindClass("java/lang/RuntimeException"));

		// Bootloader needs to go first so we can load classes
		m_ClassLoader = new JPClassLoader(this);

		JP_TRACE("Install native");
		// Start the rest of the services
		m_TypeFactory = new JPTypeFactory(this);
		m_TypeManager = new JPTypeManager(this);
		m_ReferenceQueue = new JPReferenceQueue(this);
		m_ProxyFactory = new JPProxyFactory(this);

		// Launch the Java context
		JP_TRACE("Start Context");
		jclass cls = m_ClassLoader->findClass("org.jpype.JPypeContext");
		jmethodID startMethod = frame.GetStaticMethodID(cls, "createContext",
								"(JLjava/lang/ClassLoader;)Lorg/jpype/JPypeContext;");
		m_ShutdownMethodID = frame.GetMethodID(cls, "shutdown", "()V");
	    m_CallMethodID = frame.GetMethodID(cls, "callMethod",
			"(Ljava/lang/reflect/Method;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
		
		jvalue val[2];
		val[0].j = (jlong) this;
		val[1].l = m_ClassLoader->getBootLoader();
		m_JavaContext = JPObjectRef(this, frame.CallStaticObjectMethodA(cls, startMethod, val));

		// Post launch
		JP_TRACE("Connect resources");
		// Hook up the type manager
		jmethodID getTypeManager = frame.GetMethodID(cls, "getTypeManager",
							"()Lorg/jpype/manager/TypeManager;");
		m_TypeManager->m_JavaTypeManager = JPObjectRef(this,
							frame.CallObjectMethodA(m_JavaContext.get(), getTypeManager, 0));

		// Hook up the reference queue
		jmethodID getReferenceQueue = frame.GetMethodID(cls, "getReferenceQueue",
								"()Lorg/jpype/ref/JPypeReferenceQueue;");
		m_ReferenceQueue->m_ReferenceQueue = JPObjectRef(this,
								frame.CallObjectMethodA(m_JavaContext.get(), getReferenceQueue, 0));

	}
	m_IsInitialized = true;
	JP_TRACE_OUT;
}

void JPContext::shutdownJVM()
{
	m_IsShutdown = true;

	JP_TRACE_IN("JPContext::shutdown");
	if (m_JavaVM == NULL)
		JP_RAISE_RUNTIME_ERROR("Attempt to shutdown without a live JVM");

	{
		JPJavaFrame frame(this);
		JP_TRACE("Shutdown services");
		JP_TRACE(m_JavaContext.get());
		JP_TRACE(m_ShutdownMethodID);

		// Tell Java to shutdown the context
		{
			JPPyCallRelease release;
			if (m_JavaContext.get() != 0)
				frame.CallVoidMethodA(m_JavaContext.get(), m_ShutdownMethodID, 0);
		}
	}

	// Wait for all non-demon threads to terminate
	// DestroyJVM is rather misnamed.  It is simply a wait call
	// FIXME our reference queue thunk does not appear to have properly set
	// as daemon so we hang here
	JP_TRACE("Destroy JVM");
	//	s_JavaVM->functions->DestroyJavaVM(s_JavaVM);

	// unload the jvm library
	JP_TRACE("Unload JVM");
	m_JavaVM = NULL;
	GetAdapter()->unloadLibrary();
	JP_TRACE_OUT;
}

void JPContext::createJVM(void* arg)
{
	JP_TRACE_IN("JPContext::CreateJavaVM");
	m_JavaVM = NULL;
	JNIEnv* env;
	CreateJVM_Method(&m_JavaVM, (void**) &env, arg);
	JP_TRACE_OUT;
}

void JPContext::ReleaseGlobalRef(jobject obj)
{
	// Check if the JVM is already shutdown
	if (m_JavaVM == NULL)
		return;

	// Get the environment and release the resource if we can.
	// Do not attach the thread if called from an unattached thread it is
	// likely a shutdown anyway.
	JNIEnv* env;
	jint res = m_JavaVM->functions->GetEnv(m_JavaVM, (void**) &env, USE_JNI_VERSION);
	if (res != JNI_EDETACHED)
		env->functions->DeleteGlobalRef(env, obj);
}

/*****************************************************************************/
// Thread code

void JPContext::attachCurrentThread()
{
	JNIEnv* env;
	jint res = m_JavaVM->functions->AttachCurrentThread(m_JavaVM, (void**) &env, NULL);
	if (res != JNI_OK)
		JP_RAISE_RUNTIME_ERROR("Unable to attach to thread");
}

void JPContext::attachCurrentThreadAsDaemon()
{
	JNIEnv* env;
	jint res = m_JavaVM->functions->AttachCurrentThreadAsDaemon(m_JavaVM, (void**) &env, NULL);
	if (res != JNI_OK)
		JP_RAISE_RUNTIME_ERROR("Unable to attach to thread as daemon");
}

bool JPContext::isThreadAttached()
{
	JNIEnv* env;
	return JNI_OK == m_JavaVM->functions->GetEnv(m_JavaVM, (void**) &env, USE_JNI_VERSION);
}

void JPContext::detachCurrentThread()
{
	m_JavaVM->functions->DetachCurrentThread(m_JavaVM);
}

// Java functions

class JPStringAccessor
{
	JPJavaFrame& frame_;
	jboolean isCopy;

public:
	const char* cstr;
	int length;
	jstring jstr_;

	JPStringAccessor(JPJavaFrame& frame, jstring jstr)
	: frame_(frame), jstr_(jstr)
	{
		cstr = frame_.GetStringUTFChars(jstr, &isCopy);
		length = frame_.GetStringUTFLength(jstr);
	}

	~JPStringAccessor()
	{
		frame_.ReleaseStringUTFChars(jstr_, cstr);
	}
};

string JPContext::toString(jobject o)
{
	JPJavaFrame frame(this);
	jstring str = (jstring) frame.CallObjectMethodA(o, m_Object_ToStringID, 0);
	JPStringAccessor contents(frame, str);
	return transcribe(contents.cstr, contents.length, JPEncodingJavaUTF8(), JPEncodingUTF8());
}

string JPContext::toStringUTF8(jstring str)
{
	JPJavaFrame frame(this);
	JPStringAccessor contents(frame, str);
	return transcribe(contents.cstr, contents.length, JPEncodingJavaUTF8(), JPEncodingUTF8());
}

jstring JPContext::fromStringUTF8(const string& str)
{
	JPJavaFrame frame(this);
	string mstr = transcribe(str.c_str(), str.size(), JPEncodingUTF8(), JPEncodingJavaUTF8());
	return (jstring) frame.keep(frame.NewStringUTF(mstr.c_str()));
}

jobject JPContext::callMethod(jobject method, jobject obj, jobject args)
{
	JP_TRACE_IN("JPContext::callMethod");
	if (m_CallMethodID == 0)
		return NULL;
	JPJavaFrame frame;
	jvalue v[3];
	v[0].l = method;
	v[1].l = obj;
	v[2].l = args;
	return frame.keep(frame.CallObjectMethodA(m_JavaContext.get(), m_CallMethodID, v));
	JP_TRACE_OUT;
}
