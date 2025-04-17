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



JNIEXPORT void JNICALL Java_org_jpype_bridge_Native_start
(JNIEnv *env, jobject engine)
{
printf("native start\n");
	PyObject* jpype = nullptr;
	PyObject* jpypep = nullptr;
	try
	{
		PyStatus status;
		PyConfig config;
		PyConfig_InitPythonConfig(&config);
		status = PyConfig_SetBytesString(&config, &config.program_name, "jpython");
printf("configure\n");
		if (PyStatus_Exception(status))
		{
printf("fail\n");
			PyConfig_Clear(&config);
			fail(env, "configuration failed");
		}
			
		// Get Python started
printf("initialize\n");
		PyImport_AppendInittab("_jpype", &PyInit__jpype);
		status = Py_InitializeFromConfig(&config);
		PyConfig_Clear(&config);
		if (PyStatus_Exception(status))
		{
printf("fail\n");
			fail(env, "Python initialization failed");
		}
	
#if  PY_VERSION_HEX<0x030a0000
		PyEval_InitThreads();
#endif
		PyThreadState *s_ThreadState;
		s_ThreadState = PyThreadState_Get();

		// Import the Python side to create the hooks
		jpype = PyImport_ImportModule("jpype");
printf("jpype = %p\n", jpype);
		if (jpype == NULL)
		{
			fail(env, "jpype module not found");
			return;
		}
		Py_DECREF(jpype);

		// Next install the hooks into the private side.
		jpypep = PyImport_ImportModule("_jpype");
printf("jpypep = %p\n", jpypep);
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
printf("initializeResources = %p\n", obj2);
		PyObject *obj3 = PyTuple_New(0);
		PyObject_Call(obj2, obj3, NULL);
		Py_DECREF(obj);
		Py_DECREF(obj2);
		Py_DECREF(obj3);
		
		// Everything is up and ready
printf("DONE\n");

		// Next, we need to release the state so we can return to Java.
		PyThreadState *m_State1;
		m_State1 = PyEval_SaveThread();
	} catch (JPypeException& ex)
	{
		convertException(env, ex);
	}	catch (...)
	{
		fail(env, "C++ exception during start");
	}
}

#ifdef __cplusplus
}
#endif
