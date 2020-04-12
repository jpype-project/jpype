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
#include "jpype.h"
#include "jp_typemanager.h"
#include "jp_boxedtype.h"
#include "jp_stringtype.h"
#include "jp_classloader.h"
#include "jp_voidtype.h"
#include "jp_booleantype.h"
#include "jp_bytetype.h"
#include "jp_chartype.h"
#include "jp_shorttype.h"
#include "jp_inttype.h"
#include "jp_longtype.h"
#include "jp_floattype.h"
#include "jp_doubletype.h"
#include "jp_reference_queue.h"
#include "jp_proxy.h"
#include "jp_platform.h"
#include "jp_gc.h"

JPResource::~JPResource()
{
}


#define USE_JNI_VERSION JNI_VERSION_1_4

void JPRef_failed()
{
	JP_RAISE(PyExc_SystemError, "NULL context in JPRef()");
}

JPContext::JPContext()
{
	m_JavaVM = 0;
	_void = 0;
	_byte = 0;
	_boolean = 0;
	_char = 0;
	_short = 0;
	_int = 0;
	_long = 0;
	_float = 0;
	_double = 0;

	_java_lang_Void = 0;
	_java_lang_Boolean = 0;
	_java_lang_Byte = 0;
	_java_lang_Character = 0;
	_java_lang_Short = 0;
	_java_lang_Integer = 0;
	_java_lang_Long = 0;
	_java_lang_Float = 0;
	_java_lang_Double = 0;

	_java_lang_Object = 0;
	_java_lang_Class = 0;
	_java_lang_String = 0;

	_java_lang_reflect_Method = 0;
	_java_lang_reflect_Field = 0;

	m_TypeFactory = 0;
	m_TypeManager = 0;
	m_ClassLoader = 0;
	m_ReferenceQueue = 0;
	m_ProxyFactory = 0;

	m_Object_ToStringID = 0;
	m_Object_EqualsID = 0;
	m_ShutdownMethodID = 0;
	m_IsShutdown = false;
	m_IsInitialized = false;
	m_GC = new JPGarbageCollection(this);
}

JPContext::~JPContext()
{
	delete m_TypeFactory;
	delete m_TypeManager;
	delete m_ReferenceQueue;
	delete m_ProxyFactory;
	delete m_GC;
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
void assertJVMRunning(JPContext* context, const JPStackInfo& info)
{
	if (context == NULL)
		throw JPypeException(JPError::_python_exc, PyExc_RuntimeError, "Java Context is null", info);

	if (!context->isRunning())
	{
		throw JPypeException(JPError::_python_exc, PyExc_RuntimeError, "Java Virtual Machine is not running", info);
	}
}

void JPContext::loadEntryPoints(const string& path)
{
	JPPlatformAdapter *platform = JPPlatformAdapter::getAdapter();
	// Load symbols from the shared library
	platform->loadLibrary((char*) path.c_str());
	CreateJVM_Method = (jint(JNICALL *)(JavaVM **, void **, void *) )platform->getSymbol("JNI_CreateJavaVM");
	GetCreatedJVMs_Method = (jint(JNICALL *)(JavaVM **, jsize, jsize*))platform->getSymbol("JNI_GetCreatedJavaVMs");
}

void JPContext::startJVM(const string& vmPath, const StringVector& args,
		bool ignoreUnrecognized, bool convertStrings)
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
		JP_RAISE(PyExc_RuntimeError, "Unable to start JVM");
	}

	// Connect our resources to the JVM
	{
		// This is the only frame that we can use until the system
		// is initialized.  Any other frame creation will result in an error.
		JPJavaFrame frame(this, env);

		JPException_init(frame);
		// After the JVM is created but before the context is started, we need
		// to set up all the services that the context will need.
		JP_TRACE("Initialize");

		// We need these first because if anything goes south this is the first
		// thing that will get hit.
		jclass cls = frame.FindClass("java/lang/Object");
		m_Object_ToStringID = frame.GetMethodID(cls, "toString", "()Ljava/lang/String;");
		m_Object_EqualsID = frame.GetMethodID(cls, "equals", "(Ljava/lang/Object;)Z");
		m_Object_HashCodeID = frame.GetMethodID(cls, "hashCode", "()I");
		m_Object_GetClassID = frame.GetMethodID(cls, "getClass", "()Ljava/lang/Class;");

		m_NoSuchMethodError = JPClassRef(frame, (jclass) frame.FindClass("java/lang/NoSuchMethodError"));
		m_RuntimeException = JPClassRef(frame, (jclass) frame.FindClass("java/lang/RuntimeException"));

		cls = frame.FindClass("java/lang/String");
		m_String_ToCharArrayID = frame.GetMethodID(cls, "toCharArray", "()[C");

		jclass clsType = frame.FindClass("java/lang/Class");
		m_Class_GetNameID = frame.GetMethodID(clsType, "getName", "()Ljava/lang/String;");

		// Bootloader needs to go first so we can load classes
		m_ClassLoader = new JPClassLoader(frame);

		JP_TRACE("Install native");
		// Start the rest of the services
		m_TypeFactory = new JPTypeFactory(frame);
		m_TypeManager = new JPTypeManager(frame);
		m_ReferenceQueue = new JPReferenceQueue(frame);
		m_ProxyFactory = new JPProxyFactory(frame);

		// Prepare to launch
		JP_TRACE("Start Context");
		cls = m_ClassLoader->findClass(frame, "org.jpype.JPypeContext");
		jmethodID startMethod = frame.GetStaticMethodID(cls, "createContext",
				"(JLjava/lang/ClassLoader;)Lorg/jpype/JPypeContext;");
		m_ShutdownMethodID = frame.GetMethodID(cls, "shutdown", "()V");

		// Launch
		jvalue val[2];
		val[0].j = (jlong) this;
		val[1].l = m_ClassLoader->getBootLoader();
		m_JavaContext = JPObjectRef(frame, frame.CallStaticObjectMethodA(cls, startMethod, val));

		// Post launch
		JP_TRACE("Connect resources");
		// Hook up the type manager
		jmethodID getTypeManager = frame.GetMethodID(cls, "getTypeManager",
				"()Lorg/jpype/manager/TypeManager;");
		m_TypeManager->m_JavaTypeManager = JPObjectRef(frame,
				frame.CallObjectMethodA(m_JavaContext.get(), getTypeManager, 0));

		// Hook up the reference queue
		jmethodID getReferenceQueue = frame.GetMethodID(cls, "getReferenceQueue",
				"()Lorg/jpype/ref/JPypeReferenceQueue;");
		m_ReferenceQueue->m_ReferenceQueue = JPObjectRef(frame,
				frame.CallObjectMethodA(m_JavaContext.get(), getReferenceQueue, 0));

		// Set up methods after everthing is start so we get better error
		// messages
		cls = m_ClassLoader->findClass(frame, "org.jpype.JPypeContext");
		m_CallMethodID = frame.GetMethodID(cls, "callMethod",
				"(Ljava/lang/reflect/Method;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
		m_Context_collectRectangularID = frame.GetMethodID(cls,
				"collectRectangular",
				"(Ljava/lang/Object;)[Ljava/lang/Object;");

		m_Context_assembleID = frame.GetMethodID(cls,
				"assemble",
				"([ILjava/lang/Object;)Ljava/lang/Object;");

		m_Context_CreateExceptionID = frame.GetMethodID(cls, "createException",
				"(JJ)Ljava/lang/Exception;");
		m_Context_GetExcClassID = frame.GetMethodID(cls, "getExcClass",
				"(Ljava/lang/Throwable;)J");
		m_Context_GetExcValueID = frame.GetMethodID(cls, "getExcValue",
				"(Ljava/lang/Throwable;)J");
		m_Context_OrderID = frame.GetMethodID(cls, "order", "(Ljava/nio/Buffer;)Z");

		cls = frame.FindClass("java/nio/Buffer");
		m_Buffer_IsReadOnlyID = frame.GetMethodID(cls, "isReadOnly",
				"()Z");

		cls = frame.FindClass("java/lang/Comparable");
		m_CompareToID = frame.GetMethodID(cls, "compareTo",
				"(Ljava/lang/Object;)I");

		m_GC->init(frame);

		// Testing code to make sure C++ exceptions are handled.
		// FIXME find a way to call this from instrumentation.
		// throw std::runtime_error("Failed");
		// Everything is started.
	}
	m_IsInitialized = true;
	JP_TRACE_OUT;
}

void JPContext::shutdownJVM()
{
	m_IsShutdown = true;

	JP_TRACE_IN("JPContext::shutdown");
	if (m_JavaVM == NULL)
		JP_RAISE(PyExc_RuntimeError, "Attempt to shutdown without a live JVM");

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
	// Our reference queue thunk does not appear to have properly set
	// as daemon so we hang here
	JP_TRACE("Destroy JVM");
	//	s_JavaVM->functions->DestroyJavaVM(s_JavaVM);

	// unload the jvm library
	JP_TRACE("Unload JVM");
	m_JavaVM = NULL;
	JPPlatformAdapter::getAdapter()->unloadLibrary();
	JP_TRACE_OUT;
}

void JPContext::ReleaseGlobalRef(jobject obj)
{
	JP_TRACE_IN("JPContext::ReleaseGlobalRef", obj);
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
	JP_TRACE_OUT;
}

/*****************************************************************************/
// Thread code

void JPContext::attachCurrentThread()
{
	JNIEnv* env;
	jint res = m_JavaVM->functions->AttachCurrentThread(m_JavaVM, (void**) &env, NULL);
	if (res != JNI_OK)
		JP_RAISE(PyExc_RuntimeError, "Unable to attach to thread");
}

void JPContext::attachCurrentThreadAsDaemon()
{
	JNIEnv* env;
	jint res = m_JavaVM->functions->AttachCurrentThreadAsDaemon(m_JavaVM, (void**) &env, NULL);
	if (res != JNI_OK)
		JP_RAISE(PyExc_RuntimeError, "Unable to attach to thread as daemon");
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

JNIEnv* JPContext::getEnv()
{
	JNIEnv* env = NULL;
	if (m_JavaVM == NULL)
	{
		JP_RAISE(PyExc_RuntimeError, "JVM is null");
	}

	// Get the environment
	jint res = m_JavaVM->functions->GetEnv(m_JavaVM, (void**) &env, USE_JNI_VERSION);

	// If we don't have an environment then we are in a thread, so we must attach
	if (res == JNI_EDETACHED)
	{
		res = m_JavaVM->functions->AttachCurrentThread(m_JavaVM, (void**) &env, NULL);
		if (res != JNI_OK)
			JP_RAISE(PyExc_RuntimeError, "Unable to attach to local thread");
	}
	return env;
}
