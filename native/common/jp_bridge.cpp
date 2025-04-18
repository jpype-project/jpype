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
#include <link.h>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	JNIEnv *env;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK)
        return -1;
    printf("Load python library\n");
    return JNI_VERSION_1_6;
}

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
		env->ThrowNew(runtimeException, "Internal error");
	}
}

JNIEXPORT void JNICALL Java_org_jpype_bridge_Natives2_start
(JNIEnv *env, jclass cls)
{
	printf("  natives2 start\n");
	PyObject* jpype = nullptr;
	PyObject* jpypep = nullptr;
	JPContext* context;
	PyObject *obj;
	PyObject *obj2;
	PyObject *obj3;
	PyStatus status;
	PyConfig config;
	PyGILState_STATE gstate;

	try
	{
		printf("  configure\n");
		PyConfig_InitPythonConfig(&config);

		status = PyConfig_Read(&config);
		if (PyStatus_Exception(status))
			goto error_config;

		if (PyStatus_Exception(status))
			goto error_config;
	
		status = PyConfig_SetBytesString(&config, &config.program_name, "jpython");
		if (PyStatus_Exception(status))
			goto error_config;
			
		// Get Python started
		printf("  initialize\n");
		PyImport_AppendInittab("_jpype", &PyInit__jpype);
		status = Py_InitializeFromConfig(&config);
		if (PyStatus_Exception(status))
			goto error_config;
	
		PyConfig_Clear(&config);
#if  PY_VERSION_HEX<0x030a0000
		PyEval_InitThreads();
#endif

		printf("lock\n");
		//gstate = PyGILState_Ensure();
		printf("go\n");
		jpype = PyImport_ImportModule("jpype");
		jpypep = PyImport_ImportModule("_jpype");
		if (jpypep == NULL)
		{
			fail(env, "_jpype module not found");
			return;
		}
		
		// Import the Python side to create the hooks
		if (jpype == NULL)
		{
			fail(env, "jpype module not found");
			return;
		}

		PyJPModule_loadResources(jpypep);

		Py_DECREF(jpype);
		Py_DECREF(jpypep);

		// Then attach the private module to the JVM
		context = JPContext_global;
		context->attachJVM(env);
		
		// Initialize the resources in the jpype module
		obj = PyObject_GetAttrString(jpype, "_core");
		obj2 = PyObject_GetAttrString(obj, "initializeResources");
		obj3 = PyTuple_New(0);
		PyObject_Call(obj2, obj3, NULL);
		Py_DECREF(obj);
		Py_DECREF(obj2);
		Py_DECREF(obj3);

		// Next, we need to release the state so we can return to Java.
		//PyGILState_Release(gstate);
		return;

error_config:
		PyConfig_Clear(&config);
		fail(env, "configuration failed");
		return;


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
