#include <Python.h>
#include <jni.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif
#include "jpype.h"
#include "pyjp.h"
#include <list>

PyThreadState *s_ThreadState;
PyThreadState *m_State1;
#ifdef __cplusplus
extern "C" {
#endif

static void fail(JNIEnv *env, const char* msg)
{
	// This is a low frequency path so we don't need efficiency.
	jclass runtimeException = env->FindClass("java/lang/RuntimeException");
	env->ThrowNew(runtimeException, msg);
}

static void convertException(JNIEnv *env, JPypeException& ex)
{
	// This is a low frequency path so we don't need efficiency.
	// We can't use ex.toJava() because this is part of initialization.
	jclass runtimeException = env->FindClass("java/lang/RuntimeException");

	// If it is a Java exception, we can simply throw it
	if (ex.getExceptionType() == JPError::_java_error)
	{
		env->Throw(ex.getThrowable());
		return;
	}

	// No guarantees that the exception will make it back so print it first
	PyObject *err = PyErr_Occurred();
	if (err != NULL)
	{
		PyErr_Print();
		env->ThrowNew(runtimeException, "Exception in Python");
	} else
	{
		env->ThrowNew(runtimeException, ex.getMessage().c_str());
	}
}

PyObject* jpype = nullptr;
PyObject* jpypep = nullptr;


JNIEXPORT void JNICALL Java_org_jpype_bridge_Native_start
(JNIEnv *env, jobject engine)
{
	try
	{
		PyStatus status;
		PyConfig config;
		PyConfig_InitPythonConfig(&config);
		status = PyConfig_SetBytesString(&config, &config.program_name, "jpython");
		if (PyStatus_Exception(status))
		{
			PyConfig_Clear(&config);
			fail(env, "configuration failed");
		}
			
		// Get Python started
		PyImport_AppendInittab("_jpype", &PyInit__jpype);
		status = Py_InitializeFromConfig(&config);
		PyConfig_Clear(&config);
		if (PyStatus_Exception(status))
		{
			fail(env, "Python initialization failed");
		}
	
#if  PY_VERSION_HEX<0x030a0000
		PyEval_InitThreads();
#endif
		s_ThreadState = PyThreadState_Get();
		PySys_SetPath(L".");

		// Import the Python side to create the hooks
		jpype = PyImport_ImportModule("jpype");
		if (jpype == NULL)
		{
			fail(env, "jpype module not found");
			return;
		}
		Py_DECREF(jpype);

		// Next install the hooks into the private side.
		jpypep = PyImport_ImportModule("_jpype");
		if (jpypep == NULL)
		{
			fail(env, "_jpype module not found");
			return;
		}
		PyJPModule_loadResources(jpypep);
		Py_DECREF(jpypep);
		
		// Then attach the private module to the JVM
		JPContext* context = JPContext_global;
		context->attachJVM(env);
		
		// Initialize the resources in the jpype module
		PyObject *obj = PyObject_GetAttrString(jpype, "_core");
		PyObject *obj2 = PyObject_GetAttrString(obj, "initializeResources");
		PyObject *obj3 = PyTuple_New(0);
		PyObject_Call(obj2, obj3, NULL);
		Py_DECREF(obj);
		Py_DECREF(obj2);
		Py_DECREF(obj3);
		
		// Everything is up and ready

		// Next, we need to release the state so we can return to Java.
		m_State1 = PyEval_SaveThread();
	} catch (JPypeException& ex)
	{
		convertException(env, ex);
	}	catch (...)
	{
		fail(env, "C++ exception during start");
	}
}

JNIEXPORT jlong JNICALL Java_org_jpype_bridge_Native_newContext
(JNIEnv *env, jobject, jstring str)
{
	
}


// This section will allow us to look up symbols in shared libraries from within
// Java.  We will then call those symbols using the invoker. 

// FIXME global
static std::list<void*> libraries;

// FIXME this was the full CFFI interface so we could directly call symbols in C.  
// Likely is not necessary.  But I need it for testing now.
/*
 * Class:     python_lang_PyEngine
 * Method:    getSymbol
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_jpype_bridge_Native_getSymbol
(JNIEnv *env, jobject, jstring str)
{
	try
	{
		jboolean copy;
		const char* name = env->GetStringUTFChars(str, &copy);
		for (std::list<void*>::iterator iter = libraries.begin();
				iter != libraries.end(); iter++)
		{
			void* res = NULL;
#ifdef WIN32
			res = (void*) GetProcAddress((HMODULE) *iter, name);
#else
			res = dlsym(*iter, name);
#endif
			if (res != NULL)
				return (jlong) res;
		}
		env->ReleaseStringUTFChars(str, name);
		return 0;
	} catch (JPypeException& ex)
	{
		convertException(env, ex);
	}	catch (...)
	{
		fail(env, "C++ exception during getSymbol");
	}
	return 0;
}

/*
 * Class:     python_lang_PyEngine
 * Method:    addLibrary
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_jpype_bridge_Native_addLibrary
(JNIEnv *env, jobject, jstring str)
{
	try
	{
		jboolean copy;
		const char* name = env->GetStringUTFChars(str, &copy);
		void *library = NULL;
#ifdef WIN32
		library = LoadLibrary(name);
#else
#if defined(_HPUX) && !defined(_IA64)
		jvmLibrary = shl_load(name, BIND_DEFERRED | BIND_VERBOSE, 0L);
#else
		library = dlopen(name, RTLD_LAZY | RTLD_GLOBAL);
#endif // HPUX
#endif
		env->ReleaseStringUTFChars(str, name);

		if (library == NULL)
			fail(env, "Failed to load library");
		else
			libraries.push_back(library);
		return;
	}	catch (...)
	{
		fail(env, "C++ exception in addLibrary");
	}
}

#ifdef __cplusplus
}
#endif
