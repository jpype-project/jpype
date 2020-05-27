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

	m_Object_ToStringID = 0;
	m_Object_EqualsID = 0;
	m_Running = false;

	// Java Functions
	m_Object_ToStringID = NULL;
	m_Object_EqualsID = NULL;
	m_Object_HashCodeID = NULL;
	m_CallMethodID = NULL;
	m_Class_GetNameID = NULL;
	m_Context_collectRectangularID = NULL;
	m_Context_assembleID = NULL;
	m_String_ToCharArrayID = NULL;
	m_Context_CreateExceptionID = NULL;
	m_Context_GetExcClassID = NULL;
	m_Context_GetExcValueID = NULL;
	m_CompareToID = NULL;
	m_Buffer_IsReadOnlyID = NULL;
	m_Context_OrderID = NULL;
	m_Object_GetClassID = NULL;
	m_Throwable_GetCauseID = NULL;
	m_BooleanValueID = NULL;
	m_ByteValueID = NULL;
	m_CharValueID = NULL;
	m_ShortValueID = NULL;
	m_IntValueID = NULL;
	m_LongValueID = NULL;
	m_FloatValueID = NULL;
	m_DoubleValueID = NULL;
	m_Context_GetStackFrameID = NULL;

	m_GC = new JPGarbageCollection(this);
}

JPContext::~JPContext()
{
	delete m_TypeFactory;
	delete m_TypeManager;
	delete m_ReferenceQueue;
	delete m_GC;
}

bool JPContext::isRunning()
{
	if (m_JavaVM == NULL || !m_Running)
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
		JPJavaFrame frame = JPJavaFrame::external(this, env);

		jclass throwableClass = (jclass) frame.FindClass("java/lang/Throwable");
		m_Throwable_GetCauseID = frame.GetMethodID(throwableClass, "getCause", "()Ljava/lang/Throwable;");
		m_Throwable_GetMessageID = frame.GetMethodID(throwableClass, "getMessage", "()Ljava/lang/String;");

		// After the JVM is created but before the context is started, we need
		// to set up all the services that the context will need.
		JP_TRACE("Initialize");

		// We need these first because if anything goes south this is the first
		// thing that will get hit.
		jclass objectClass = frame.FindClass("java/lang/Object");
		m_Object_ToStringID = frame.GetMethodID(objectClass, "toString", "()Ljava/lang/String;");
		m_Object_EqualsID = frame.GetMethodID(objectClass, "equals", "(Ljava/lang/Object;)Z");
		m_Object_HashCodeID = frame.GetMethodID(objectClass, "hashCode", "()I");
		m_Object_GetClassID = frame.GetMethodID(objectClass, "getClass", "()Ljava/lang/Class;");

		m_NoSuchMethodError = JPClassRef(frame, (jclass) frame.FindClass("java/lang/NoSuchMethodError"));
		m_RuntimeException = JPClassRef(frame, (jclass) frame.FindClass("java/lang/RuntimeException"));

		jclass stringClass = frame.FindClass("java/lang/String");
		m_String_ToCharArrayID = frame.GetMethodID(stringClass, "toCharArray", "()[C");

		jclass classClass = frame.FindClass("java/lang/Class");
		m_Class_GetNameID = frame.GetMethodID(classClass, "getName", "()Ljava/lang/String;");

		// Bootloader needs to go first so we can load classes
		m_ClassLoader = new JPClassLoader(frame);

		JP_TRACE("Install native");
		// Start the rest of the services
		m_TypeFactory = new JPTypeFactory(frame);
		m_TypeManager = new JPTypeManager(frame);
		m_ReferenceQueue = new JPReferenceQueue(frame);

		// Prepare to launch
		JP_TRACE("Start Context");
		jclass contextClass = m_ClassLoader->findClass(frame, "org.jpype.JPypeContext");
		m_Context_GetStackFrameID = frame.GetMethodID(contextClass, "getStackTrace",
				"(Ljava/lang/Throwable;Ljava/lang/Throwable;)[Ljava/lang/Object;");

		jmethodID startMethod = frame.GetStaticMethodID(contextClass, "createContext",
				"(JLjava/lang/ClassLoader;)Lorg/jpype/JPypeContext;");

		JNINativeMethod method[1];
		method[0].name = (char*) "onShutdown";
		method[0].signature = (char*) "(J)V";
		method[0].fnPtr = (void*) JPContext::onShutdown;
		frame.GetMethodID(contextClass, "<init>", "()V");
		frame.RegisterNatives(contextClass, method, 1);

		// Launch
		jvalue val[2];
		val[0].j = (jlong) this;
		val[1].l = m_ClassLoader->getBootLoader();
		m_JavaContext = JPObjectRef(frame, frame.CallStaticObjectMethodA(contextClass, startMethod, val));

		// Post launch
		JP_TRACE("Connect resources");
		// Hook up the type manager
		jmethodID getTypeManager = frame.GetMethodID(contextClass, "getTypeManager",
				"()Lorg/jpype/manager/TypeManager;");
		m_TypeManager->m_JavaTypeManager = JPObjectRef(frame,
				frame.CallObjectMethodA(m_JavaContext.get(), getTypeManager, 0));

		// Hook up the reference queue
		jmethodID getReferenceQueue = frame.GetMethodID(contextClass, "getReferenceQueue",
				"()Lorg/jpype/ref/JPypeReferenceQueue;");
		m_ReferenceQueue->m_ReferenceQueue = JPObjectRef(frame,
				frame.CallObjectMethodA(m_JavaContext.get(), getReferenceQueue, 0));

		// Set up methods after everything is start so we get better error
		// messages
		m_CallMethodID = frame.GetMethodID(contextClass, "callMethod",
				"(Ljava/lang/reflect/Method;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
		m_Context_collectRectangularID = frame.GetMethodID(contextClass,
				"collectRectangular",
				"(Ljava/lang/Object;)[Ljava/lang/Object;");

		m_Context_assembleID = frame.GetMethodID(contextClass,
				"assemble",
				"([ILjava/lang/Object;)Ljava/lang/Object;");

		m_Context_GetFunctionalID = frame.GetMethodID(contextClass,
				"getFunctional",
				"(Ljava/lang/Class;)Ljava/lang/String;");

		m_Context_CreateExceptionID = frame.GetMethodID(contextClass, "createException",
				"(JJ)Ljava/lang/Exception;");
		m_Context_GetExcClassID = frame.GetMethodID(contextClass, "getExcClass",
				"(Ljava/lang/Throwable;)J");
		m_Context_GetExcValueID = frame.GetMethodID(contextClass, "getExcValue",
				"(Ljava/lang/Throwable;)J");
		m_Context_OrderID = frame.GetMethodID(contextClass, "order", "(Ljava/nio/Buffer;)Z");
		m_Context_IsPackageID = frame.GetMethodID(contextClass, "isPackage", "(Ljava/lang/String;)Z");
		m_Context_GetPackageID = frame.GetMethodID(contextClass, "getPackage", "(Ljava/lang/String;)Lorg/jpype/pkg/JPypePackage;");

		jclass packageClass = m_ClassLoader->findClass(frame, "org.jpype.pkg.JPypePackage");
		m_Package_GetObjectID = frame.GetMethodID(packageClass, "getObject",
				"(Ljava/lang/String;)Ljava/lang/Object;");
		m_Package_GetContentsID = frame.GetMethodID(packageClass, "getContents",
				"()[Ljava/lang/String;");

		m_Array = JPClassRef(frame, frame.FindClass("java/lang/reflect/Array"));
		m_Array_NewInstanceID = frame.GetStaticMethodID(m_Array.get(), "newInstance",
				"(Ljava/lang/Class;[I)Ljava/lang/Object;");

		jclass bufferClass = frame.FindClass("java/nio/Buffer");
		m_Buffer_IsReadOnlyID = frame.GetMethodID(bufferClass, "isReadOnly",
				"()Z");

		jclass comparableClass = frame.FindClass("java/lang/Comparable");
		m_CompareToID = frame.GetMethodID(comparableClass, "compareTo",
				"(Ljava/lang/Object;)I");

		jclass proxyClass = getClassLoader()->findClass(frame, "org.jpype.proxy.JPypeProxy");

		method[0].name = (char*) "hostInvoke";
		method[0].signature = (char*) "(JLjava/lang/String;JJ[J[Ljava/lang/Object;)Ljava/lang/Object;";
		method[0].fnPtr = (void*) &JPProxy::hostInvoke;
		frame.GetMethodID(proxyClass, "<init>", "()V");
		frame.RegisterNatives(proxyClass, method, 1);

		m_ProxyClass = JPClassRef(frame, proxyClass);
		m_Proxy_NewID = frame.GetStaticMethodID(m_ProxyClass.get(),
				"newProxy",
				"(Lorg/jpype/JPypeContext;JJ[Ljava/lang/Class;)Lorg/jpype/proxy/JPypeProxy;");
		m_Proxy_NewInstanceID = frame.GetMethodID(m_ProxyClass.get(),
				"newInstance",
				"()Ljava/lang/Object;");

		m_GC->init(frame);

		// Testing code to make sure C++ exceptions are handled.
		// FIXME find a way to call this from instrumentation.
		// throw std::runtime_error("Failed");
		// Everything is started.
	}
	m_Running = true;
	JP_TRACE_OUT;
}

JNIEXPORT void JNICALL JPContext::onShutdown(JNIEnv *env, jobject obj, jlong contextPtr)
{
	((JPContext*) contextPtr)->m_Running = false;
}

void JPContext::shutdownJVM()
{
	JP_TRACE_IN("JPContext::shutdown");
	if (m_JavaVM == NULL)
		JP_RAISE(PyExc_RuntimeError, "Attempt to shutdown without a live JVM");

	// Wait for all non-demon threads to terminate
	JP_TRACE("Destroy JVM");
	{
		JPPyCallRelease call;
		m_JavaVM->DestroyJavaVM();
	}

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
		// We will attach as daemon so that the newly attached thread does
		// not deadlock the shutdown.  The user can convert later if they want.
		res = m_JavaVM->AttachCurrentThreadAsDaemon((void**) &env, NULL);
		if (res != JNI_OK)
			JP_RAISE(PyExc_RuntimeError, "Unable to attach to local thread");
	}
	return env;
}
