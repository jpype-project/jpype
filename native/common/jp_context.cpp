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

/* 
 * File:   jp_context.cpp
 * Author: Karl Einar Nelson
 * 
 * Created on May 28, 2019, 12:15 AM
 */

#include "jp_context.h"


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
}

JPContext::JPContext(const JPContext& orig)
{
}

JPContext::~JPContext()
{
}

bool JPContext::isInitialized()
{
	return m_JavaVM != NULL;
}

/**
	throw a JPypeException if the JVM is not started
 */
void JPContext::assertJVMRunning(const char* function, const JPStackInfo& info)
{
	// FIXME fit function names into raise
	if (!JPEnv::isInitialized())
	{
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

void JPContext::startJVM(const string& vmPath, char ignoreUnrecognized,
		const StringVector& args)
{
	JP_TRACE_IN("JPEnv::loadJVM");

	// Get the entry points in the shared library
	loadEntryPoints(vmPath);

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
	JPEnv::CreateJavaVM((void*) &jniArgs);
	free(jniArgs.options);

	if (m_JavaVM == NULL)
	{
		JP_TRACE("Unable to start");
		JP_RAISE_RUNTIME_ERROR("Unable to start JVM");
	}

	{
		JPJavaFrame frame(this);
		// After the JVM is created but before the context is started, we need 
		// lo set up all the services that the context will need.
		JP_TRACE("Initialize");

		// We need these first because if anything goes south this is the first
		// thing that will get hit.
		_java_lang_NoSuchMethodError = (jclass) frame.FindClass("java/lang/NoSuchMethodError");
		_java_lang_RuntimeException = (jclass) frame.FindClass("java/lang/RuntimeException");

		// Bootloader needs to go first so we can load classes
		m_ClassLoader = new JPClassLoader(this, false);

		// Start the rest of the services
		m_TypeFactory = new JPTypeFactory(this);
		m_TypeManager = new JPTypeManager(this);
		m_ReferenceQueue = new JPReferenceQueue(this);
		m_ProxyFactory = new JPProxyFactory(this);

		// Launch the Java context
		jclass cls = JPClassLoader::findClass("org.jpype.JPypeContext");
		jmethodID startMethod = frame.GetStaticMethodID(cls, "createContext",
				"(JLjava.lang.ClassLoader;)Lorg.jpype.JPypeContext;");
		m_ContextShutdownMethod = frame.GetMethodID(cls, "shutdown", "()V");

		jvalue val[2];
		val[0].j = (jlong) this;
		val[1].l = 0;
		m_JavaContext = frame.CallStaticObjectMethodA(cls, startMethod, val);

		jmethodID getTypeManager = frame.GetMethodID(cls, "getTypeManager",
				"()Lorg.jpype.manager.TypeManager;");
		m_TypeManager->m_TypeManager = frame.CallObjectMethod(m_JavaContext.get(), getTypeManager);

		// Once we return from start method all Java objects are populated.
		// Thus we can now access the Java classes.

		//	s_ReferenceQueueStopMethod = frame.GetMethodID(cls, "stop", "()V");
		m_Object_ToStringID = frame.GetMethodID(_java_lang_Object->getJavaClass(), "toString", "()Ljava/lang/String;");
		s_String_ToCharArrayID = frame.GetMethodID(s_StringClass, "toCharArray", "()[C");

		s_NoSuchMethodErrorClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/NoSuchMethodError"));
		s_RuntimeExceptionClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/RuntimeException"));

		s_ProxyClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/reflect/Proxy"));
		s_NewProxyInstanceID = frame.GetStaticMethodID(s_ProxyClass, "newProxyInstance",
				"(Ljava/lang/ClassLoader;[Ljava/lang/Class;Ljava/lang/reflect/InvocationHandler;)Ljava/lang/Object;");

		s_ThrowableClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/Throwable"));
		s_Throwable_GetMessageID = frame.GetMethodID(s_ThrowableClass, "getMessage", "()Ljava/lang/String;");
		s_Throwable_PrintStackTraceID = frame.GetMethodID(s_ThrowableClass, "printStackTrace", "(Ljava/io/PrintWriter;)V");

		s_StringWriterClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/io/StringWriter"));
		s_PrintWriterClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/io/PrintWriter"));
		s_StringWriterID = frame.GetMethodID(s_StringWriterClass, "<init>", "()V");
		s_PrintWriterID = frame.GetMethodID(s_PrintWriterClass, "<init>", "(Ljava/io/Writer;)V");
		s_FlushID = frame.GetMethodID(s_PrintWriterClass, "flush", "()V");

		frame.CallVoidMethod(cls, startMethod);
	}


	JP_TRACE_OUT;
}

void JPContext::shutdownJVM()
{
	m_Shutdown = true;

	JP_TRACE_IN("JPEnv::shutdown");
	if (m_JavaVM == NULL)
		JP_RAISE_RUNTIME_ERROR("Attempt to shutdown without a live JVM");

	{
		JPJavaFrame frame(this);
		// Tell Java to shutdown the context
		frame.CallVoidMethod(m_JavaContext, m_ContextShutdownMethod);
	}

	// Wait for all non-demon threads to terminate
	// DestroyJVM is rather misnamed.  It is simply a wait call
	// FIXME our reference queue thunk does not appear to have properly set 
	// as daemon so we hang here
	JP_TRACE("Destroy JVM");
	//	s_JavaVM->functions->DestroyJavaVM(s_JavaVM);

	// unload the jvm library
	JP_TRACE("Unload JVM");
	GetAdapter()->unloadLibrary();
	m_JavaVM = NULL;
	JP_TRACE_OUT;
}

void JPContext::createJVM(void* arg)
{
	JP_TRACE_IN("JPEnv::CreateJavaVM");
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
	jint res = m_JavaVM->functions->GetEnv(m_JavaVM, (void**) &, USE_JNI_VERSION);
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
	jstring str = (jstring) frame.CallObjectMethod(o, m_Object_ToStringID);
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

