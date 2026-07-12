// --- file: common/jp_context.cpp ---
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
#include <mutex>
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

// Resolves the process-wide GlobalPool release binding used by
// tryRelease() below - defined further down, forward-declared here so
// initializeResources() can call it.
static void bindGlobalPoolRelease(JPJavaFrame& frame);
static void bindCoreImmortals(JPJavaFrame& frame);

void JPRef_failed()
{
	JP_RAISE(PyExc_SystemError, "NULL context in JPRef()");
}
JPContext::JPContext(PyJPModuleState *state)
{
	// --- State and Flags ---
	modulestate = state;
	m_Running = false;
	m_ConvertStrings = false;
	m_Embedded = false;

	// --- Core Services & Framework ---
	m_Interpreter = nullptr;
	m_JavaVM = nullptr;
	m_TypeManager = nullptr;
	m_ClassLoader = nullptr;
	m_GC = new JPGarbageCollection(this);
	m_PyExcConvert = nullptr;

	// --- JNI Function Pointers ---
	CreateJVM_Method = nullptr;
	GetCreatedJVMs_Method = nullptr;
	GetDefaultJavaVMInitArgs_Method = nullptr;

	// --- Primitive Types ---
	_void = nullptr;
	_boolean = nullptr;
	_byte = nullptr;
	_char = nullptr;
	_short = nullptr;
	_int = nullptr;
	_long = nullptr;
	_float = nullptr;
	_double = nullptr;

	// --- Boxed Types ---
	_java_lang_Void = nullptr;
	_java_lang_Boolean = nullptr;
	_java_lang_Byte = nullptr;
	_java_lang_Character = nullptr;
	_java_lang_Short = nullptr;
	_java_lang_Integer = nullptr;
	_java_lang_Long = nullptr;
	_java_lang_Float = nullptr;
	_java_lang_Double = nullptr;

	// --- Core Java Classes ---
	_java_lang_Object = nullptr;
	_java_lang_Class = nullptr;
	_java_lang_reflect_Field = nullptr;
	_java_lang_reflect_Method = nullptr;
	_java_lang_Throwable = nullptr;
	_java_lang_String = nullptr;
	_python_lang_PyObject = nullptr;
	m_RuntimeException = nullptr;
	m_Array = nullptr;
	m_PyJavaObjectClass = nullptr;
	m_ReferenceQueue = nullptr;

	// --- Core Java Method IDs ---
	m_Array_NewInstanceID = nullptr;
	m_Buffer_IsReadOnlyID = nullptr;
	m_Buffer_AsReadOnlyID = nullptr;
	m_Class_GetNameID = nullptr;
	m_CompareToID = nullptr;
	m_Object_ToStringID = nullptr;
	m_Object_EqualsID = nullptr;
	m_Object_HashCodeID = nullptr;
	m_Object_GetClassID = nullptr;
	m_String_ToCharArrayID = nullptr;
	m_Throwable_GetCauseID = nullptr;
	m_Throwable_GetMessageID = nullptr;

	// --- Package Bindings & Support ---
	m_JavaContext = nullptr;
	m_ContextClass = nullptr;
	m_Context_ClearInterruptID = nullptr;
	m_Context_GetFunctionalID = nullptr;
	m_Context_IsPackageID = nullptr;
	m_Context_GetPackageID = nullptr;
	m_Package_GetObjectID = nullptr;
	m_Package_GetContentsID = nullptr;
	m_Context_NewWrapperID = nullptr;
	
	m_SupportClass = nullptr;
	m_Support_GetStackFrameID = nullptr;
	m_Support_collectRectangularID = nullptr;
	m_Support_assembleID = nullptr;
	m_Support_OrderID = nullptr;
	m_Support_GetTotalMemoryID = nullptr;
	m_Support_GetFreeMemoryID = nullptr;
	m_Support_GetMaxMemoryID = nullptr;
	m_Support_GetUsedMemoryID = nullptr;
	m_Support_GetHeapMemoryID = nullptr;

	// --- Proxy Management & Reflection ---
	m_JavaProxyFactory = nullptr;
	m_ProxyFactoryClass = nullptr;
	m_ProxyFactory_getProxyTypeID = nullptr;
	m_ProxyTypeClass = nullptr;
	m_ProxyType_newInstanceID = nullptr;
	m_ProxyType_UnwrapPythonExceptionID = nullptr;
	m_ProxyType_GetInstanceID = nullptr;

	m_Reflector = nullptr;
	m_Reflector_CallMethodID = nullptr;
	m_PyJavaObject_wrap = nullptr;

	interruptState = 0;

}

JPContext::~JPContext()
{
	delete m_TypeManager;
	delete m_GC;
	delete m_ClassLoader;
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
    // Default fallback to the core, guaranteed-to-exist primitive
    PyObject* jvm_not_running_exc = PyExc_RuntimeError;

    if (context == nullptr)
    {
        // Near-fatal system invariant failure
        throw JPypeException(JPError::_python_exc, jvm_not_running_exc, "Java Context is null", info);
    }

    if (!context->isRunning())
    {
        // Look up the exception class inside the active subinterpreter's module dictionary
        if (context->modulestate != nullptr && context->modulestate->module_dict != nullptr)
        {
            // Note: PyDict_GetItemString returns a borrowed reference
            PyObject* obj = PyDict_GetItemString(context->modulestate->module_dict, "JVMNotRunning");
            if (obj != nullptr)
            {
                jvm_not_running_exc = obj;
            }
        }
        throw JPypeException(JPError::_python_exc, jvm_not_running_exc, "Java Virtual Machine is not running", info);
    }
}

void JPContext::loadEntryPoints(const string& path)
{
	JP_TRACE_IN("JPContext::loadEntryPoints");
	JPPlatformAdapter *platform = JPPlatformAdapter::getAdapter();
	platform->loadLibrary((char*) path.c_str());
	CreateJVM_Method = (jint(JNICALL *)(JavaVM **, void **, void *) )
		platform->getSymbol("JNI_CreateJavaVM");
	GetCreatedJVMs_Method = (jint(JNICALL *)(JavaVM **, jsize, jsize*))
		platform->getSymbol("JNI_GetCreatedJavaVMs");
	GetDefaultJavaVMInitArgs_Method = (jint(JNICALL *)(void *))
		platform->getSymbol("JNI_GetDefaultJavaVMInitArgs");

	if (CreateJVM_Method == nullptr || GetDefaultJavaVMInitArgs_Method == nullptr)
		JP_RAISE(PyExc_RuntimeError, "JVM shared library is missing required JNI symbols");
	JP_TRACE_OUT;
}

// This call is used to establish the main interpreter when started from within Python
JavaVM* _JavaVM = nullptr;
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

	JavaVMInitArgs scout;
	scout.version = 0x00090000; // JNI_VERSION_21 (JDK 21)
	if (GetDefaultJavaVMInitArgs_Method(&scout) != JNI_OK)
		JP_RAISE(PyExc_RuntimeError, "Java version too old. Java 9 or later is required");

	bool isJDK21 = false;
	scout.version = 0x00150000; // JNI_VERSION_21 (JDK 21)
	if (GetDefaultJavaVMInitArgs_Method(&scout) == JNI_OK)
		isJDK21 = true;

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
	jint currentOption = 0;
	for (size_t i = 0; i < args.size(); i++)
	{
		const string& opt = args[i];
		// CHOP: If not JDK 21, specifically kill native-access even if it's there
		if (!isJDK21 && opt.find("--enable-native-access") == 0)
		{
			JP_TRACE("Chopping JDK21 option", opt);
			continue;
		}

		// Pack the valid option
		JP_TRACE("Packing Option", opt);
		strncpy(&block[j], opt.c_str(), opt.size());
		jniArgs->options[currentOption].optionString = (char*) &block[j];
		
		j += PAD(opt.size() + 1);
		currentOption++;
	}

	// Finalize the count based on what actually survived the chop
	jniArgs->nOptions = currentOption;

	// Launch the JVM
	JNIEnv* env = nullptr;
	JP_TRACE("Create JVM");
	try
	{
		CreateJVM_Method(&m_JavaVM, (void**) &env, (void*) jniArgs);
		_JavaVM = m_JavaVM;
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

	jint jni_version = env->GetVersion();
	if (jni_version < 0x00090000)
	{
		JP_RAISE(PyExc_RuntimeError, "Java version too old. Java 9 or later is required");
	}

	{
		JPJavaFrame frame = JPJavaFrame::external(env, this);
		jclass interpreterClass = (jclass) frame.FindClass("org/jpype/MainInterpreter");
		jmethodID instance = frame.GetStaticMethodID(interpreterClass, "getInstance", "()Lorg/jpype/MainInterpreter;");
		m_Interpreter = frame.NewGlobalRef( frame.CallStaticObjectMethodA(interpreterClass, instance, nullptr));
	}

	initializeResources(env, interrupt);
	JP_TRACE_OUT;
}


void JPContext::attachJVM(JNIEnv* env)
{
	if (m_Interpreter == nullptr)
	{
		printf("Interpreter not set\n");
	}
	env->GetJavaVM(&m_JavaVM);
	_JavaVM = m_JavaVM;
#ifndef ANDROID
	m_Embedded = true;
#endif
	initializeResources(env, false);
}

void JPContext::detachJVM()
{
	ReleaseGlobalRef(m_Interpreter);
	if (m_ClassLoader != nullptr)
		m_ClassLoader->release(this);

	// Real per-context globals (m_RuntimeException/m_Array are process-wide
	// immortals resolved via bindCoreImmortals() - not released here, see
	// plan/Globals.md Category B).
	ReleaseGlobalRef(m_ContextClass);
	ReleaseGlobalRef(m_JavaContext);
	if (m_TypeManager != nullptr)
		ReleaseGlobalRef(m_TypeManager->m_JavaTypeManager);
	ReleaseGlobalRef(m_Reflector);
	ReleaseGlobalRef(m_SupportClass);
	ReleaseGlobalRef(m_ProxyFactoryClass);
	ReleaseGlobalRef(m_JavaProxyFactory);
	ReleaseGlobalRef(m_ProxyTypeClass);
	ReleaseGlobalRef(m_PyJavaObjectClass);
	ReleaseGlobalRef(m_ReferenceQueue);
	m_ReferenceQueue = nullptr;

	m_JavaVM = nullptr;
	m_Running = false;
	_JavaVM = nullptr;

	JP_TRACE("Delete resources");
	for (auto & m_Resource : m_Resources)
	{
		delete m_Resource;
	}
	m_Resources.clear();
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

// java.lang.RuntimeException / java.lang.reflect.Array are also process-wide
// immortals (same object regardless of context) - resolved once here rather
// than re-globalized on every JPContext::initializeResources() call.
static jclass s_RuntimeExceptionClass = nullptr;
static jclass s_ArrayClass = nullptr;
static jmethodID s_ArrayNewInstanceID = nullptr;

static void bindCoreImmortals(JPJavaFrame& frame)
{
	static std::once_flag lookupOnce;
	std::call_once(lookupOnce, [&frame]()
	{
		s_RuntimeExceptionClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/RuntimeException"));
		s_ArrayClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/reflect/Array"));
		s_ArrayNewInstanceID = frame.GetStaticMethodID(s_ArrayClass, "newInstance",
				"(Ljava/lang/Class;[I)Ljava/lang/Object;");
	});
}

void JPContext::initializeResources(JNIEnv* env, bool interrupt)
{
	JPPyCallRelease release;
	JPJavaFrame frame = JPJavaFrame::external(env, this);
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

	// True process-wide globals, resolved once here (see tryRelease/
	// bindGlobalPoolRelease/bindCoreImmortals below) rather than
	// re-resolved per context.
	bindGlobalPoolRelease(frame);
	bindCoreImmortals(frame);
	m_RuntimeException = s_RuntimeExceptionClass;

	jclass stringClass = frame.FindClass("java/lang/String");
	m_String_ToCharArrayID = frame.GetMethodID(stringClass, "toCharArray", "()[C");

	jclass classClass = frame.FindClass("java/lang/Class");
	m_Class_GetNameID = frame.GetMethodID(classClass, "getName", "()Ljava/lang/String;");
	
	// Bootloader needs to go first so we can load classes
	m_ClassLoader = new JPClassLoader(frame);

	JP_TRACE("Install native");
	// Start the rest of the services
	m_TypeManager = new JPTypeManager(frame);

	// ========================================================================
	// Initialize the Modernized NativeContext Core Hub
	// ========================================================================
	JP_TRACE("Start Context");
	m_ContextClass = (jclass) frame.NewGlobalRef((jclass) m_ClassLoader->findClass(frame, "org.jpype.internal.NativeContext"));
	jclass contextClass = m_ContextClass;

	// Needed for TypeFactory and other early users of the context during initialization
	m_Context_GetFunctionalID = frame.GetMethodID(contextClass,
			"getFunctional",
			"(Ljava/lang/Class;)J");


	// Construction is split from initialization: `create` only builds the
	// Java object graph (including its GlobalPool). `start` runs
	// typeManager.init() and friends, which call back into native and may
	// invoke storeGlobal - so it must not run until AFTER m_JavaContext and
	// m_Context_StoreGlobalID/RetrieveGlobalID are resolved below, or those
	// early callbacks would storeGlobal() through a still-null instance.
	jmethodID createMethod = frame.GetStaticMethodID(contextClass, "create",
			"(JLorg/jpype/internal/DynamicClassLoader;Ljava/lang/String;)Lorg/jpype/internal/NativeContext;");

	// Prepare arguments for context creation
	jvalue val[3];
	val[0].j = (jlong) this;
	val[1].l = m_ClassLoader->getBootLoader();
	val[2].l = nullptr;

	if (!m_Embedded)
	{
		std::string shared = getShared();
		val[2].l = frame.fromStringUTF8(shared);
	}

	// Instantiate the core context hub (construction only - no initialize())
	m_JavaContext = frame.NewGlobalRef(frame.CallStaticObjectMethodA(contextClass, createMethod, val));

	// NativeContext instance/static methods
	m_Context_IsPackageID = frame.GetMethodID(contextClass, "isPackage", "(Ljava/lang/String;)Z");
	m_Context_GetPackageID = frame.GetMethodID(contextClass, "getPackage", "(Ljava/lang/String;)Lorg/jpype/pkg/Package;");
	m_Context_ClearInterruptID = frame.GetMethodID(contextClass, "clearInterrupt", "(Z)V");
	m_Context_NewWrapperID = frame.GetMethodID(contextClass, "newWrapper", "(J)V");
	m_Context_StoreGlobalID = frame.GetMethodID(contextClass, "storeGlobal", "(Ljava/lang/Object;)J");
	m_Context_RetrieveGlobalID = frame.GetMethodID(contextClass, "retrieveGlobal", "(J)Ljava/lang/Object;");

	// Now safe to run initialize()/scanExistingJars() - any storeGlobal()
	// callbacks they trigger (e.g. primitive type construction) can now
	// resolve against a live m_JavaContext.
	jmethodID startInstanceMethod = frame.GetMethodID(contextClass, "start", "(Z)V");
	jvalue startArgs[1];
	startArgs[0].z = interrupt;
	frame.CallVoidMethodA(m_JavaContext, startInstanceMethod, startArgs);

	// Post launch bindings
	JP_TRACE("Connect resources");

	// Hook up the type manager
	jmethodID getTypeManager = frame.GetMethodID(contextClass, "getTypeManager",
			"()Lorg/jpype/manager/TypeManager;");
	m_TypeManager->m_JavaTypeManager = frame.NewGlobalRef(
			frame.CallObjectMethodA(m_JavaContext, getTypeManager, nullptr));

	// Connect the reflector from the new NativeContext field
	jclass reflectorClass = frame.FindClass("org/jpype/Reflector");
	jfieldID reflectorField = frame.GetFieldID(contextClass, "reflector", "Lorg/jpype/Reflector;");
	m_Reflector = frame.NewGlobalRef(frame.GetObjectField(m_JavaContext, reflectorField));
	m_Reflector_CallMethodID = frame.GetMethodID(reflectorClass, "callMethod",
			"(Ljava/lang/reflect/Method;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");

	// Package bindings
	jclass packageClass = m_ClassLoader->findClass(frame, "org.jpype.pkg.Package");
	m_Package_GetObjectID = frame.GetMethodID(packageClass, "getObject",
			"(Ljava/lang/String;)Ljava/lang/Object;");
	m_Package_GetContentsID = frame.GetMethodID(packageClass, "getContents",
			"()[Ljava/lang/String;");

	// ========================================================================
	// Resolve Support_2.java Utilities Class and Static Method Handles
	// ========================================================================
	jclass supportLocal = (jclass) m_ClassLoader->findClass(frame, "org.jpype.internal.Support");
	m_SupportClass = (jclass) frame.NewGlobalRef(supportLocal);
	jclass supportClass = m_SupportClass;

	// --- Diagnostic Hooks & Exception Handling ---
	m_Support_GetStackFrameID = frame.GetStaticMethodID(supportClass, "getStackTrace",
			"(Ljava/lang/Throwable;Ljava/lang/Throwable;)[Ljava/lang/Object;");

	// --- Primitive Memory View & Multidimensional Array Processing ---
	m_Support_collectRectangularID = frame.GetStaticMethodID(supportClass, "collectRectangular",
			"(Ljava/lang/Object;)[Ljava/lang/Object;");

	m_Support_assembleID = frame.GetStaticMethodID(supportClass, "assemble",
			"([ILjava/lang/Object;)Ljava/lang/Object;");

	// --- Endianness / Byte Ordering ---
	m_Support_OrderID = frame.GetStaticMethodID(supportClass, "order", 
			"(Ljava/nio/Buffer;)Z");

	// --- Memory Profiling and Diagnostics ---
	m_Support_GetTotalMemoryID = frame.GetStaticMethodID(supportClass, "getTotalMemory", "()J");
	m_Support_GetFreeMemoryID = frame.GetStaticMethodID(supportClass, "getFreeMemory", "()J");
	m_Support_GetMaxMemoryID = frame.GetStaticMethodID(supportClass, "getMaxMemory", "()J");
	m_Support_GetUsedMemoryID = frame.GetStaticMethodID(supportClass, "getUsedMemory", "()J");
	m_Support_GetHeapMemoryID = frame.GetStaticMethodID(supportClass, "getHeapMemory", "()J");

	// ========================================================================
	// Resolve ProxyFactory (via NativeContext_2.java Explicit Getter)
	// ========================================================================
	JP_TRACE("Connect Proxy Factory");

	// 1. Look up the modernized ProxyFactory class
	jclass factoryClass = getClassLoader()->findClass(frame, "org.jpype.proxy.ProxyFactory");
	m_ProxyFactoryClass = (jclass) frame.NewGlobalRef(factoryClass);

	// 2. Look up the explicit getter method on NativeContext and safely pull the instance
	jmethodID getProxyFactoryMethod = frame.GetMethodID(contextClass, "getProxyFactory", 
		"()Lorg/jpype/proxy/ProxyFactory;");
	m_JavaProxyFactory = frame.NewGlobalRef(frame.CallObjectMethodA(m_JavaContext, getProxyFactoryMethod, nullptr));

	// 3. Bind getProxyType as an instance method on ProxyFactory
	m_ProxyFactory_getProxyTypeID = frame.GetMethodID(factoryClass, "getProxyType",
		"(J[Ljava/lang/Class;)Lorg/jpype/proxy/ProxyType;");

	// 4. Resolve the simplified ProxyType class
	jclass proxyTypeClass = getClassLoader()->findClass(frame, "org.jpype.proxy.ProxyType");
	m_ProxyTypeClass = (jclass) frame.NewGlobalRef(proxyTypeClass);

	// 5. Look up ProxyType instance and utility methods
	m_ProxyType_newInstanceID = frame.GetMethodID(proxyTypeClass, "newInstance", "(J)Ljava/lang/Object;");
	
	m_ProxyType_UnwrapPythonExceptionID = frame.GetStaticMethodID(
		proxyTypeClass, 
		"unwrapPythonException", 
		"(Ljava/lang/Throwable;)J"
	);
	m_ProxyType_GetInstanceID = frame.GetStaticMethodID(proxyTypeClass, "getInstance", "(Ljava/lang/Object;)J");

	// ========================================================================
	// Core Standard Library Array and Buffer Support
	// ========================================================================
	m_Array = s_ArrayClass;
	m_Array_NewInstanceID = s_ArrayNewInstanceID;
	jclass bufferClass = frame.FindClass("java/nio/Buffer");
	m_Buffer_IsReadOnlyID = frame.GetMethodID(bufferClass, "isReadOnly",
			"()Z");
	jclass bytebufferClass = frame.FindClass("java/nio/ByteBuffer");
	m_Buffer_AsReadOnlyID = frame.GetMethodID(bytebufferClass, "asReadOnlyBuffer",
			"()Ljava/nio/ByteBuffer;");
	jclass comparableClass = frame.FindClass("java/lang/Comparable");
	m_CompareToID = frame.GetMethodID(comparableClass, "compareTo",
			"(Ljava/lang/Object;)I");

	// ========================================================================
	// JNI Wrapper Hooks
	// ========================================================================
	jclass wrapperClass = getClassLoader()->findClass(frame, "python.lang.PyJavaObject");
	m_PyJavaObjectClass = (jclass) frame.NewGlobalRef(wrapperClass);
	m_PyJavaObject_wrap = frame.GetStaticMethodID(
		wrapperClass,
		"wrap",
		"(Ljava/lang/Object;)Ljava/lang/Object;");

	// FIXME this depends on resources that are not initialized until jpype module is initialized, so we need to delay this until then.  We should probably move the GC initialization to the module init code instead of the context init code.
	//m_GC->init(frame);

	//_java_nio_ByteBuffer = (jclass) frame.NewGlobalRef(frame.findClassByName("java.nio.ByteBuffer"));

	// Testing code to make sure C++ exceptions are handled.
	// FIXME find a way to call this from instrumentation.
	// throw std::runtime_error("Failed");
	// Everything is started.
	m_Running = true;
}

void JPContext::onShutdown()
{
	m_Running = false;
}

void JPContext::shutdownJVM(bool destroyJVM, bool freeJVM)
{
	JP_TRACE_IN("JPContext::shutdown");
	if (m_Embedded)
		JP_RAISE(PyExc_RuntimeError, "Attempt to shutdown embedded JVM");
	if (m_JavaVM == nullptr)
		JP_RAISE(PyExc_RuntimeError, "Attempt to shutdown without a live JVM");

	// Wait for all non-demon threads to terminate
	if (destroyJVM)
	{
		JP_TRACE("Destroy JVM");
		JPPyCallRelease call;
		m_JavaVM->DestroyJavaVM();
	}

	// unload the jvm library
	m_JavaVM = nullptr;
	m_Running = false;
	_JavaVM = nullptr;
	if (freeJVM)
	{
		JP_TRACE("Unload JVM");
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
		assertJVMRunning(this, JP_STACKINFO());
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
			assertJVMRunning(this, JP_STACKINFO());
			JP_RAISE(PyExc_RuntimeError, "Unable to attach to local thread");
		}
	}
	return env;
}

// True process-wide globals (like m_ClassClass/_SystemClass elsewhere),
// resolved once at the first interpreter's bootstrap - see
// bindGlobalPoolRelease() below - not looked up lazily on every
// tryRelease() call. org.jpype.ref.GlobalPool loads via the shared
// boot/system classloader, the same one every interpreter's core support
// classes come from, so one resolution is valid for the life of the
// process.
static jclass s_GlobalPoolClass = nullptr;
static jmethodID s_TryReleaseID = nullptr;

// Called once, from the first interpreter's initializeResources(), to
// populate the globals above.
static void bindGlobalPoolRelease(JPJavaFrame& frame)
{
	static std::once_flag lookupOnce;
	std::call_once(lookupOnce, [&frame]()
	{
		jclass local = frame.FindClass("org/jpype/ref/GlobalPool");
		s_GlobalPoolClass = (jclass) frame.NewGlobalRef(local);
		s_TryReleaseID = frame.GetStaticMethodID(s_GlobalPoolClass, "tryRelease", "(J)V");
	});
}

// Releases a jref (see jpype.h) - called from destructors (JPClass,
// JPMethod, JPField, JPArray, JPMonitor, JPBuffer) with no JPJavaFrame in
// hand. Routes to the static Java-side org.jpype.ref.GlobalPool.tryRelease,
// which decodes ref's prefix to find its owning pool and safely no-ops if
// that interpreter has already torn down - see GlobalPool.java.
void tryRelease(jref ref)
{
	if (ref.value == 0 || _JavaVM == nullptr || s_TryReleaseID == nullptr)
		return;
	JNIEnv* env = nullptr;
	// If we get detached here it usually means the JVM is not active as any thread
	// that can reach this point was once we attached.   I will need to assess that
	// assumption later to make sure we don't leak.
	jint res = _JavaVM->functions->GetEnv(_JavaVM, (void**) &env, USE_JNI_VERSION);
	if (res == JNI_EDETACHED)
		return;
	env->CallStaticVoidMethod(s_GlobalPoolClass, s_TryReleaseID, (jlong) ref.value);
}

extern "C" JNIEXPORT void JNICALL Java_org_jpype_JPypeContext_onShutdown
(JNIEnv *env, jobject obj, jlong ctx)
{
	JPContext* context = (JPContext*) ctx;
	context->onShutdown();
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

extern "C" JNIEXPORT void JNICALL Java_org_jpype_internal_Signal_interruptPy
(JNIEnv *env, jclass cls, jlong ctx, jint signal)
{
	JPContext* context = (JPContext*) ctx;
    context->interruptState = 1;
	JPPyCallAcquire callback(context->modulestate);
    // 2. Safely trip the engine evaluation loop interrupt flag
#if PY_VERSION_HEX < 0x030A0000
    PyErr_SetInterrupt();
#else
    PyErr_SetInterruptEx((int) signal);
#endif
}

extern "C" JNIEXPORT void JNICALL Java_org_jpype_internal_Signal_acknowledgePy
(JNIEnv *env, jclass cls, jlong ctx)
{
	JPContext* context = (JPContext*) ctx;
	context->interruptState = 0;
}

