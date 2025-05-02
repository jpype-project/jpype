/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#include "jpype.h"
#include "pyjp.h"
#include "jp_typemanager.h"
#include "jp_stringtype.h"
#include "jp_classloader.h"
#include "jp_proxy.h"
#include "jp_platform.h"
#include "jp_gc.h"

#ifdef WIN32
#include <Windows.h>
#else
#if defined(_HPUX) && !defined(_IA64)
#include <dl.h>
#else
#include <dlfcn.h>
#endif // HPUX
#include <errno.h>
#endif


JPResource::~JPResource() = default;


#define USE_JNI_VERSION JNI_VERSION_1_4

void JPRef_failed()
{
	JP_RAISE(PyExc_SystemError, "NULL context in JPRef()");
}

JPContext::JPContext()
{
	m_Embedded = false;

	m_GC = new JPGarbageCollection();
}

JPContext::~JPContext()
{
	delete m_TypeManager;
	delete m_GC;
}

bool JPContext::isRunning()
{
	if (m_JavaVM == nullptr || !m_Running)
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
	if (_JVMNotRunning == nullptr)
	{
		_JVMNotRunning = PyObject_GetAttrString(PyJPModule, "JVMNotRunning");
		JP_PY_CHECK();
		Py_INCREF(_JVMNotRunning);
	}

	if (context == nullptr)
	{
		throw JPypeException(JPError::_python_exc, _JVMNotRunning, "Java Context is null", info);
	}

	if (!context->isRunning())
	{
		throw JPypeException(JPError::_python_exc, _JVMNotRunning, "Java Virtual Machine is not running", info);
	}
}

void JPContext::loadEntryPoints(const string& path)
{
	JP_TRACE_IN("JPContext::loadEntryPoints");
	JPPlatformAdapter *platform = JPPlatformAdapter::getAdapter();
	// Load symbols from the shared library
	platform->loadLibrary((char*) path.c_str());
	CreateJVM_Method = (jint(JNICALL *)(JavaVM **, void **, void *) )platform->getSymbol("JNI_CreateJavaVM");
	GetCreatedJVMs_Method = (jint(JNICALL *)(JavaVM **, jsize, jsize*))platform->getSymbol("JNI_GetCreatedJavaVMs");
	JP_TRACE_OUT;
}

void JPContext::startJVM(const string& vmPath, const StringVector& args,
		bool ignoreUnrecognized, bool convertStrings, bool interrupt)
{
	JP_TRACE_IN("JPContext::startJVM");

	JP_TRACE("Convert strings", convertStrings);
	m_ConvertStrings = convertStrings;

	// Get the entry points in the shared library
	try
	{
		JP_TRACE("Load entry points");
		loadEntryPoints(vmPath);
	} catch (JPypeException& ex)
	{
		(void) ex;
		throw;
	}

	// Determine the memory requirements
#define PAD(x) ((x+31)&~31)
	size_t mem = PAD(sizeof(JavaVMInitArgs));
	size_t oblock = mem;
	mem += PAD(sizeof(JavaVMOption)*args.size() + 1);
	size_t sblock = mem;
	for (size_t i = 0; i < args.size(); i++)
	{
		mem += PAD(args[i].size()+1);
	}

	// Pack the arguments
	JP_TRACE("Pack arguments");
	char *block = (char*) malloc(mem);
	JavaVMInitArgs* jniArgs = (JavaVMInitArgs*) block;
	memset(jniArgs, 0, mem);
	jniArgs->options = (JavaVMOption*)(&block[oblock]);

	// prepare this ...
	jniArgs->version = USE_JNI_VERSION;
	jniArgs->ignoreUnrecognized = ignoreUnrecognized;
	JP_TRACE("IgnoreUnrecognized", ignoreUnrecognized);

	jniArgs->nOptions = (jint) args.size();
	JP_TRACE("NumOptions", jniArgs->nOptions);
	size_t j = sblock;
	for (size_t i = 0; i < args.size(); i++)
	{
		JP_TRACE("Option", args[i]);
		strncpy(&block[j], args[i].c_str(), args[i].size());
		jniArgs->options[i].optionString = (char*) &block[j];
		j += PAD(args[i].size()+1);
	}

	// Launch the JVM
	JNIEnv* env = nullptr;
	JP_TRACE("Create JVM");
	try
	{
		CreateJVM_Method(&m_JavaVM, (void**) &env, (void*) jniArgs);
	} catch (...)
	{
		JP_TRACE("Exception in CreateJVM?");
	}
	JP_TRACE("JVM created");
	free(jniArgs);

	if (m_JavaVM == nullptr)
	{
		JP_TRACE("Unable to start");
		JP_RAISE(PyExc_RuntimeError, "Unable to start JVM");
	}

	// Mark running for assert
	m_Running = true;
	initializeResources(env, interrupt);
	JP_TRACE_OUT;
}

void JPContext::attachJVM(JNIEnv* env)
{
	env->GetJavaVM(&m_JavaVM);
#ifndef ANDROID
	m_Embedded = true;
#endif
	initializeResources(env, false);
}

std::string getShared() 
{
#ifdef WIN32
	// Windows specific
	char path[MAX_PATH];
	HMODULE hm = NULL;
	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR) &getShared, &hm) != 0 &&
		GetModuleFileName(hm, path, sizeof(path)) != 0)
	{
		// This is needed when there is no-ascii characters in path
		char shortPathBuffer[MAX_PATH];
		long len = GetShortPathName(path, shortPathBuffer, MAX_PATH);
		if (len != 0)
			return std::string(shortPathBuffer);
	}
#else
	// Linux specific
	Dl_info info;
	if (dladdr((void*)getShared, &info))
		return info.dli_fname;
#endif
	// Generic
	JPPyObject import = JPPyObject::use(PyImport_AddModule("importlib.util"));
	JPPyObject jpype = JPPyObject::call(PyObject_CallMethod(import.get(), "find_spec", "s", "_jpype"));
	JPPyObject origin = JPPyObject::call(PyObject_GetAttrString(jpype.get(), "origin"));
	return JPPyString::asStringUTF8(origin.get());
}

void JPContext::initializeResources(JNIEnv* env, bool interrupt)
{
	JPJavaFrame frame = JPJavaFrame::external(env);
	// This is the only frame that we can use until the system
	// is initialized.  Any other frame creation will result in an error.

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

	m_RuntimeException = JPClassRef(frame, (jclass) frame.FindClass("java/lang/RuntimeException"));

	jclass stringClass = frame.FindClass("java/lang/String");
	m_String_ToCharArrayID = frame.GetMethodID(stringClass, "toCharArray", "()[C");

	jclass classClass = frame.FindClass("java/lang/Class");
	m_Class_GetNameID = frame.GetMethodID(classClass, "getName", "()Ljava/lang/String;");

	// Bootloader needs to go first so we can load classes
	m_ClassLoader = new JPClassLoader(frame);

	JP_TRACE("Install native");
	// Start the rest of the services
	m_TypeManager = new JPTypeManager(frame);

	// Prepare to launch
	JP_TRACE("Start Context");
	m_ContextClass = JPClassRef(frame, (jclass) m_ClassLoader->findClass(frame, "org.jpype.JPypeContext"));
	jclass contextClass = m_ContextClass.get();
	m_Context_GetStackFrameID = frame.GetMethodID(contextClass, "getStackTrace",
			"(Ljava/lang/Throwable;Ljava/lang/Throwable;)[Ljava/lang/Object;");

	jmethodID startMethod = frame.GetStaticMethodID(contextClass, "createContext",
			"(JLjava/lang/ClassLoader;Ljava/lang/String;Z)Lorg/jpype/JPypeContext;");

	// Launch
	jvalue val[4];
	val[0].j = (jlong) this;
	val[1].l = m_ClassLoader->getBootLoader();
	val[2].l = nullptr;
	val[3].z = interrupt;

	if (!m_Embedded)
	{
		std::string shared = getShared();
		val[2].l = frame.fromStringUTF8(shared);
	}

	// Required before launch
	m_Context_GetFunctionalID = frame.GetStaticMethodID(contextClass,
			"getFunctional",
			"(Ljava/lang/Class;)Ljava/lang/String;");

	m_JavaContext = JPObjectRef(frame, frame.CallStaticObjectMethodA(contextClass, startMethod, val));

	// Post launch
	JP_TRACE("Connect resources");
	// Hook up the type manager
	jmethodID getTypeManager = frame.GetMethodID(contextClass, "getTypeManager",
			"()Lorg/jpype/manager/TypeManager;");
	m_TypeManager->m_JavaTypeManager = JPObjectRef(frame,
			frame.CallObjectMethodA(m_JavaContext.get(), getTypeManager, nullptr));

	// Set up methods after everything is start so we get better error
	// messages
	jclass reflectorClass = frame.FindClass("org/jpype/JPypeReflector");
	jfieldID reflectorField = frame.GetFieldID(contextClass, "reflector", "Lorg/jpype/JPypeReflector;");
	m_Reflector = JPObjectRef(frame, frame.GetObjectField(m_JavaContext.get(), reflectorField));
	m_CallMethodID = frame.GetMethodID(reflectorClass, "callMethod",
			"(Ljava/lang/reflect/Method;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
	m_Context_collectRectangularID = frame.GetMethodID(contextClass,
			"collectRectangular",
			"(Ljava/lang/Object;)[Ljava/lang/Object;");

	m_Context_assembleID = frame.GetMethodID(contextClass,
			"assemble",
			"([ILjava/lang/Object;)Ljava/lang/Object;");

	m_Context_CreateExceptionID = frame.GetMethodID(contextClass, "createException",
			"(JJ)Ljava/lang/Exception;");
	m_Context_GetExcClassID = frame.GetMethodID(contextClass, "getExcClass",
			"(Ljava/lang/Throwable;)J");
	m_Context_GetExcValueID = frame.GetMethodID(contextClass, "getExcValue",
			"(Ljava/lang/Throwable;)J");
	m_Context_OrderID = frame.GetMethodID(contextClass, "order", "(Ljava/nio/Buffer;)Z");
	m_Context_IsPackageID = frame.GetMethodID(contextClass, "isPackage", "(Ljava/lang/String;)Z");
	m_Context_GetPackageID = frame.GetMethodID(contextClass, "getPackage", "(Ljava/lang/String;)Lorg/jpype/pkg/JPypePackage;");
	m_Context_ClearInterruptID = frame.GetStaticMethodID(contextClass, "clearInterrupt", "(Z)V");

	jclass packageClass = m_ClassLoader->findClass(frame, "org.jpype.pkg.JPypePackage");
	m_Package_GetObjectID = frame.GetMethodID(packageClass, "getObject",
			"(Ljava/lang/String;)Ljava/lang/Object;");
	m_Package_GetContentsID = frame.GetMethodID(packageClass, "getContents",
			"()[Ljava/lang/String;");
	m_Context_NewWrapperID = frame.GetMethodID(contextClass, "newWrapper",
			"(J)V");

	m_Array = JPClassRef(frame, frame.FindClass("java/lang/reflect/Array"));
	m_Array_NewInstanceID = frame.GetStaticMethodID(m_Array.get(), "newInstance",
			"(Ljava/lang/Class;[I)Ljava/lang/Object;");

	jclass bufferClass = frame.FindClass("java/nio/Buffer");
	m_Buffer_IsReadOnlyID = frame.GetMethodID(bufferClass, "isReadOnly",
			"()Z");

	jclass bytebufferClass = frame.FindClass("java/nio/ByteBuffer");
	m_Buffer_AsReadOnlyID = frame.GetMethodID(bytebufferClass, "asReadOnlyBuffer",
			"()Ljava/nio/ByteBuffer;");

	jclass comparableClass = frame.FindClass("java/lang/Comparable");
	m_CompareToID = frame.GetMethodID(comparableClass, "compareTo",
			"(Ljava/lang/Object;)I");

	jclass proxyClass = getClassLoader()->findClass(frame, "org.jpype.proxy.JPypeProxy");
	m_ProxyClass = JPClassRef(frame, proxyClass);
	m_Proxy_NewID = frame.GetStaticMethodID(m_ProxyClass.get(),
			"newProxy",
			"(Lorg/jpype/JPypeContext;JJ[Ljava/lang/Class;)Lorg/jpype/proxy/JPypeProxy;");
	m_Proxy_NewInstanceID = frame.GetMethodID(m_ProxyClass.get(),
			"newInstance",
			"()Ljava/lang/Object;");

	m_GC->init(frame);

	_java_nio_ByteBuffer = this->getTypeManager()->findClassByName("java.nio.ByteBuffer");

	// Testing code to make sure C++ exceptions are handled.
	// FIXME find a way to call this from instrumentation.
	// throw std::runtime_error("Failed");
	// Everything is started.
}

void JPContext::onShutdown()
{
	m_Running = false;
}

void JPContext::shutdownJVM(bool destroyJVM, bool freeJVM)
{
	JP_TRACE_IN("JPContext::shutdown");
	if (m_JavaVM == nullptr)
		JP_RAISE(PyExc_RuntimeError, "Attempt to shutdown without a live JVM");
	//	if (m_Embedded)
	//		JP_RAISE(PyExc_RuntimeError, "Cannot shutdown from embedded Python");

	// Wait for all non-demon threads to terminate
	if (destroyJVM)
	{
		JP_TRACE("Destroy JVM");
		JPPyCallRelease call;
		m_JavaVM->DestroyJavaVM();
	}

	// unload the jvm library
	if (freeJVM)
	{
		JP_TRACE("Unload JVM");
		m_JavaVM = nullptr;
		JPPlatformAdapter::getAdapter()->unloadLibrary();
	}

	JP_TRACE("Delete resources");
	for (auto & m_Resource : m_Resources)
	{
		delete m_Resource;
	}
	m_Resources.clear();

	JP_TRACE_OUT;
}

void JPContext::ReleaseGlobalRef(jobject obj)
{
	JP_TRACE_IN("JPContext::ReleaseGlobalRef", obj);
	// Check if the JVM is already shutdown
	if (m_JavaVM == nullptr)
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
	jint res = m_JavaVM->functions->AttachCurrentThread(m_JavaVM, (void**) &env, nullptr);
	if (res != JNI_OK)
		JP_RAISE(PyExc_RuntimeError, "Unable to attach to thread");
}

void JPContext::attachCurrentThreadAsDaemon()
{
	JNIEnv* env;
	jint res = m_JavaVM->functions->AttachCurrentThreadAsDaemon(m_JavaVM, (void**) &env, nullptr);
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
	JNIEnv* env = nullptr;
	if (m_JavaVM == nullptr)
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
		res = m_JavaVM->AttachCurrentThreadAsDaemon((void**) &env, nullptr);
		if (res != JNI_OK)
		{
			JP_RAISE(PyExc_RuntimeError, "Unable to attach to local thread");
		}
	}
	return env;
}

extern "C" JNIEXPORT void JNICALL Java_org_jpype_JPypeContext_onShutdown
(JNIEnv *env, jobject obj, jlong contextPtr)
{
	((JPContext*) contextPtr)->onShutdown();
}

/**********************************************************************
 * Interrupts are complex.   Both Java and Python want to handle the 
 * interrupt, but only one can be in control.  Java starts later and 
 * installs its handler over Python as a chain.  If Java handles it then
 * the JVM will terminate which leaves Python with a bunch of bad
 * references which tends to lead to segfaults.  So we need to disable
 * the Java one by routing it back to Python.  But if we do so then 
 * Java wont respect Ctrl+C.  So we need to handle the interrupt, convert
 * it to a wait interrupt so that Java can break at the next I/O and 
 * then trip Python signal handler so the Python gets the interrupt.
 *
 * But this leads to a few race conditions.
 *
 * If the control is in Java then it will get the interrupt next time 
 * it hits Python code when the returned object is checked resulting
 * InterruptedException.  Now we have two exceptions on the stack,
 * the one from Java and the one from Python.  We check to see if 
 * Python has a pending interrupt and eat the Java one.
 *
 * If the control is in Java and it hits an I/O call.  This generates
 * InterruptedException which again transfers control to Python where
 * the Exception is resolved.
 *
 * If the control is in Python when the interrupt occurs, then
 * we have a bogus Java interrupt sitting on the main thread that the next
 * Java call will trip over.  So we need to call clearInterrupt(false).
 * This checks clears the interrupt in C++ and in Java.
 *
 */

static int interruptState = 0;
extern "C" JNIEXPORT void JNICALL Java_org_jpype_JPypeSignal_interruptPy
(JNIEnv *env, jclass cls, jint signal)
{
	interruptState = 1;
#if PY_MINOR_VERSION<10
	PyErr_SetInterrupt();
#else
	PyErr_SetInterruptEx((int) signal);
#endif
}

extern "C" JNIEXPORT void JNICALL Java_org_jpype_JPypeSignal_acknowledgePy
(JNIEnv *env, jclass cls)
{
	interruptState = 0;
}

int hasInterrupt()
{
	return interruptState != 0;
}
