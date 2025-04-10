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

/* Arguments we need to push in.
 * 
 * A list of module_search_paths so this can be used of limited/embedded deployments.
 * A list of command line arguments so we can execute command line functionality.
 */
JNIEXPORT void JNICALL Java_org_jpype_bridge_Natives_start
(JNIEnv *env, jclass cls, jobjectArray modulePath, jobjectArray args, 
	jstring name, jstring prefix, jstring home, jstring exec_prefix, jstring executable,
	jboolean isolated, jboolean faulthandler, jboolean quiet, jboolean verbose,
	jboolean site_import, jboolean user_site, jboolean bytecode)
{

	PyObject* jpype = nullptr;
	PyObject* jpypep = nullptr;
	JPContext* context;
	PyObject *obj;
	PyObject *obj2;
	PyObject *obj3;
	PyStatus status;
	PyConfig config;

	PyGILState_STATE gstate;
	jboolean isCopy;
	const char *cstr;
	int length;
	std::string str;
	jsize items;
	wchar_t* wide_str;
	jobject v;
	std::list<wchar_t*> resources;

	try
	{
		if (isolated)
		{
			PyConfig_InitIsolatedConfig(&config);
		}
		else
		{
			PyConfig_InitPythonConfig(&config);
			status = PyConfig_Read(&config);
			if (PyStatus_Exception(status))
				goto error_config;
		}

		config.faulthandler = faulthandler;
		config.quiet = quiet;
		config.site_import = site_import;
		config.user_site_directory = user_site;
		config.write_bytecode = bytecode;
		config.verbose = verbose;

		if (name != nullptr)
		{
			cstr = env->GetStringUTFChars(name, &isCopy);
			length = env->GetStringUTFLength(name);
			str = transcribe(cstr, length, JPEncodingJavaUTF8(), JPEncodingUTF8());
			env->ReleaseStringUTFChars(name, cstr);
			wide_str = Py_DecodeLocale(str.c_str(), NULL);
			config.program_name = wide_str;
			resources.push_back(wide_str);
		}

		if (prefix != nullptr)
		{
			cstr = env->GetStringUTFChars(prefix, &isCopy);
			length = env->GetStringUTFLength(prefix);
			str = transcribe(cstr, length, JPEncodingJavaUTF8(), JPEncodingUTF8());
			env->ReleaseStringUTFChars(prefix, cstr);
			wide_str = Py_DecodeLocale(str.c_str(), NULL);
			config.prefix = wide_str;
			resources.push_back(wide_str);
		}

		if (home != nullptr)
		{
			cstr = env->GetStringUTFChars(home, &isCopy);
			length = env->GetStringUTFLength(home);
			str = transcribe(cstr, length, JPEncodingJavaUTF8(), JPEncodingUTF8());
			env->ReleaseStringUTFChars(home, cstr);
			wide_str = Py_DecodeLocale(str.c_str(), NULL);
			config.home = wide_str;
			resources.push_back(wide_str);
		}

		if (exec_prefix != nullptr)
		{
			cstr = env->GetStringUTFChars(exec_prefix, &isCopy);
			length = env->GetStringUTFLength(exec_prefix);
			str = transcribe(cstr, length, JPEncodingJavaUTF8(), JPEncodingUTF8());
			env->ReleaseStringUTFChars(exec_prefix, cstr);
			wide_str = Py_DecodeLocale(str.c_str(), NULL);
			config.exec_prefix = wide_str;
			resources.push_back(wide_str);
		}

		if (executable != nullptr)
		{
			cstr = env->GetStringUTFChars(executable, &isCopy);
			length = env->GetStringUTFLength(executable);
			str = transcribe(cstr, length, JPEncodingJavaUTF8(), JPEncodingUTF8());
			env->ReleaseStringUTFChars(executable, cstr);
			wide_str = Py_DecodeLocale(str.c_str(), NULL);
			config.executable = wide_str;
			resources.push_back(wide_str);
		}

		if (modulePath != nullptr)
		{
			config.module_search_paths_set = 1;
			items = env->GetArrayLength(modulePath);
			for (jsize i = 0; i<items; ++i)
			{
				v = env->GetObjectArrayElement(modulePath, i);
				if (v == nullptr)
					continue;
				cstr = env->GetStringUTFChars((jstring)v, &isCopy);
				length = env->GetStringUTFLength((jstring)v);
				str = transcribe(cstr, length, JPEncodingJavaUTF8(), JPEncodingUTF8());
				env->ReleaseStringUTFChars((jstring)v, cstr);
				wide_str = Py_DecodeLocale(str.c_str(), NULL);
			 	PyWideStringList_Append(&config.module_search_paths, wide_str);
				resources.push_back(wide_str);
			}
		}

		if (args != nullptr)
		{
			config.parse_argv = 1;
			items = env->GetArrayLength(args);
			for (jsize i = 0; i<items; ++i)
			{
				v = env->GetObjectArrayElement(args, i);
				if (v == nullptr)
					continue;
				cstr = env->GetStringUTFChars((jstring)v, &isCopy);
				length = env->GetStringUTFLength((jstring)v);
				str = transcribe(cstr, length, JPEncodingJavaUTF8(), JPEncodingUTF8());
				env->ReleaseStringUTFChars((jstring)v, cstr);
				wide_str = Py_DecodeLocale(str.c_str(), NULL);
			 	PyWideStringList_Append(&config.argv, wide_str);
				resources.push_back(wide_str);
			}
		    if (PyStatus_Exception(status))
				goto error_config;
		}

		// Get Python started
//		PyImport_AppendInittab("_jpype", &PyInit__jpype);
		status = Py_InitializeFromConfig(&config);
		if (PyStatus_Exception(status))
			goto error_config;

		goto success_config;

error_config:
		PyConfig_Clear(&config);
		fail(env, "configuration failed");
		for (std::list<wchar_t*>::iterator iter = resources.begin(); iter!=resources.end(); ++iter)
			PyMem_Free(*iter);
		return;

success_config:
		PyConfig_Clear(&config);
//		for (std::list<wchar_t*>::iterator iter = resources.begin(); iter!=resources.end(); ++iter)
//			PyMem_Free(*iter);
#if  PY_VERSION_HEX<0x030a0000
		PyEval_InitThreads();
#endif

		gstate = PyGILState_Ensure();
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
		JPJavaFrame frame = JPJavaFrame::external(context, env);
		
		// Initialize the resources in the jpype module
		obj = PyObject_GetAttrString(jpype, "_core");
		obj2 = PyObject_GetAttrString(obj, "initializeResources");
		obj3 = PyTuple_New(0);
		PyObject_Call(obj2, obj3, NULL);
		Py_DECREF(obj);
		Py_DECREF(obj2);
		Py_DECREF(obj3);

		// Next, we need to release the state so we can return to Java.
		PyGILState_Release(gstate);
		return;

	} catch (JPypeException& ex)
	{
		convertException(env, ex);
	}	catch (...)
	{
		fail(env, "C++ exception during start");
	}
}

JNIEXPORT void JNICALL Java_org_jpype_bridge_Natives_interactive
(JNIEnv *env, jclass cls)
{
	JPPyCallAcquire callback;
	PyRun_InteractiveLoop(stdin, "<stdin>");
}

JNIEXPORT void JNICALL Java_org_jpype_bridge_Natives_finish
(JNIEnv *env, jclass cls)
{
	JPPyCallAcquire callback;
	Py_Finalize();
	// FIXME it is unclear if we will need to release the thread lock after this command
}



#ifdef __cplusplus
}
#endif
