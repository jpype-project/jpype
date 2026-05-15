// --- file: common/jp_bridge.cpp ---
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
#include <iostream>
#include <cwchar>
#include <cstdlib>

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

static wchar_t* toWideString(JNIEnv* env, jstring value)
{
	if (value == nullptr)
		return nullptr;

	jboolean isCopy;
	const char* cstr = env->GetStringUTFChars(value, &isCopy);
	if (cstr == nullptr)
		return nullptr;

	int length = env->GetStringUTFLength(value);
	std::string str = transcribe(cstr, length, JPEncodingJavaUTF8(), JPEncodingUTF8());
	env->ReleaseStringUTFChars(value, cstr);
	wchar_t* wide_str = Py_DecodeLocale(str.c_str(), NULL);
	return wide_str;
}

static PyStatus appendStringArray(
	JNIEnv* env,
	jobjectArray array,
	PyWideStringList* target)
{
	PyStatus status = PyStatus_Ok();

	if (array == nullptr)
		return status;

	jsize items = env->GetArrayLength(array);
	for (jsize i = 0; i < items; ++i)
	{
		jstring value = (jstring) env->GetObjectArrayElement(array, i);
		if (value == nullptr)
			continue;

		wchar_t* wide_str = toWideString(env, value);
		if (wide_str == nullptr)
			return PyStatus_Error("failed to convert Java string");

		status = PyWideStringList_Append(target, wide_str);
		if (PyStatus_Exception(status))
			return status;
	}

	return status;
}

static bool assignWideString(
	JNIEnv* env,
	jstring value,
	wchar_t*& target)
{
	if (value == nullptr)
		return true;

	wchar_t* wide_str = toWideString(env, value);
	if (wide_str == nullptr)
		return false;

	target = wide_str;
	return true;
}

static void dumpWide(const char* name, const wchar_t* value)
{
	if (value == NULL)
	{
		fprintf(stderr, "%s=<null>\n", name);
		return;
	}

	// Determine buffer size needed
	size_t size = wcstombs(NULL, value, 0);
	if (size == (size_t)-1) 
	{
		fprintf(stderr, "%s=<encoding error>\n", name);
		return;
	}

	char* buffer = (char*)malloc(size + 1);
	wcstombs(buffer, value, size + 1);
	fprintf(stderr, "%s=%s\n", name, buffer);
	free(buffer);
}

static void dumpWideList(const char* name, const PyWideStringList* list)
{
	fprintf(stderr, "%s.length=%zd\n", name, (size_t)list->length);
	for (Py_ssize_t i = 0; i < list->length; ++i)
	{
		char label[256];
		snprintf(label, sizeof(label), "%s[%zd]", name, (size_t)i);
		dumpWide(label, list->items[i]);
	}
}

void print_module_path(const char* name, PyObject* module) {
	if (module != NULL) {
		// Access the __file__ attribute
		PyObject* file_path = PyObject_GetAttrString(module, "__file__");
		if (file_path != NULL) {
			// In Python 3, __file__ is a Unicode object
			const char* path = PyUnicode_AsUTF8(file_path);
			printf("Module [%s] loaded from: %s\n", name, path);
			Py_DECREF(file_path);
		} else {
			printf("Module [%s] has no __file__ attribute (it might be built-in).\n", name);
		}
	}
}


static void dumpPyConfig(const PyConfig* config)
{
	fprintf(stderr, "PyConfig dump begin\n");

	dumpWide("  program_name", config->program_name);
	dumpWide("  prefix", config->prefix);
	dumpWide("  home", config->home);
	dumpWide("  exec_prefix", config->exec_prefix);
	dumpWide("  executable", config->executable);
#if PY_VERSION_HEX >= 0x030B0000
	dumpWide("  base_prefix", config->base_prefix);
	dumpWide("  base_exec_prefix", config->base_exec_prefix);
#endif

	fprintf(stderr, "  isolated=%d\n", config->isolated);
	fprintf(stderr, "  use_environment=%d\n", config->use_environment);
	fprintf(stderr, "  site_import=%d\n", config->site_import);
	fprintf(stderr, "  user_site_directory=%d\n", config->user_site_directory);
	fprintf(stderr, "  write_bytecode=%d\n", config->write_bytecode);
	fprintf(stderr, "  verbose=%d\n", config->verbose);
	fprintf(stderr, "  quiet=%d\n", config->quiet);
	fprintf(stderr, "  faulthandler=%d\n", config->faulthandler);
	fprintf(stderr, "  parse_argv=%d\n", config->parse_argv);
	fprintf(stderr, "  module_search_paths_set=%d\n", config->module_search_paths_set);

	dumpWideList("  argv", &config->argv);
	dumpWideList("  module_search_paths", &config->module_search_paths);

	fprintf(stderr, "PyConfig dump end\n");
}


static bool appendModulePathsToSysPath(JNIEnv* env, jobjectArray modulePath)
{
	if (modulePath == nullptr)
		return true;

	PyObject* sys = PyImport_ImportModule("sys");
	if (sys == nullptr)
		return false;

	PyObject* path = PyObject_GetAttrString(sys, "path");
	Py_DECREF(sys);
	if (path == nullptr || !PyList_Check(path))
	{
		Py_XDECREF(path);
		return false;
	}

	jsize count = env->GetArrayLength(modulePath);
	for (jsize i = 0; i < count; ++i)
	{
		jstring jpath = (jstring) env->GetObjectArrayElement(modulePath, i);
		if (jpath == nullptr) continue;

		// Use your existing helper to get a real wchar_t*
		wchar_t* widePath = toWideString(env, jpath);
		if (widePath == nullptr)
		{
			env->DeleteLocalRef(jpath);
			Py_DECREF(path);
			return false;
		}

		// Now Python is getting the 32-bit wchar_t it expects on Linux
		PyObject* pyPath = PyUnicode_FromWideChar(widePath, -1);
		
		// Py_DecodeLocale uses PyMem_RawMalloc, so free it properly
		PyMem_RawFree(widePath); 
		env->DeleteLocalRef(jpath);

		if (pyPath == nullptr || PyList_Append(path, pyPath) < 0)
		{
			Py_XDECREF(pyPath);
			Py_DECREF(path);
			return false;
		}
		Py_DECREF(pyPath);
	}
	Py_DECREF(path);
	return true;
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

	try
	{
		// 1. Basic Init
		if (isolated)
			PyConfig_InitIsolatedConfig(&config);
		else
			PyConfig_InitPythonConfig(&config);

		// 2. Set Booleans
		config.faulthandler = faulthandler;
		config.quiet = quiet;
		config.site_import = site_import;
		config.user_site_directory = user_site;
		config.write_bytecode = bytecode;
		config.verbose = verbose;

		// 3. Set Path Inputs (Critical to do BEFORE PyConfig_Read)
		if (!assignWideString(env, name, config.program_name)) goto error_config;
		if (!assignWideString(env, home, config.home)) goto error_config;
		if (!assignWideString(env, executable, config.executable)) goto error_config;
		// Note: prefix and exec_prefix are usually calculated by Python 
		// based on 'home', but setting them manually here is fine.

		// 4. THE READ: This calculates the default sys.path
		status = PyConfig_Read(&config);
		if (PyStatus_Exception(status)) goto error_config;

		// 5. THE APPEND: Add Java paths to the calculated system paths
		if (modulePath != nullptr)
		{
			// We must set this to 1 to tell Python "I have modified the paths, 
			// don't try to recalculate them during initialization."
			config.module_search_paths_set = 1;
			
			status = appendStringArray(env, modulePath, &config.module_search_paths);
			if (PyStatus_Exception(status)) goto error_config;
		}

		// 6. Set Argv
		if (args != nullptr)
		{
			config.parse_argv = 1;
			status = appendStringArray(env, args, &config.argv);
			if (PyStatus_Exception(status)) goto error_config;
		}

		// 7. Launch
		//dumpPyConfig(&config);
		status = Py_InitializeFromConfig(&config);
		if (PyStatus_Exception(status))
		{
			fprintf(stderr, "Py_InitializeFromConfig failed\n");
			fprintf(stderr, "  func: %s\n", status.func ? status.func : "<null>");
			fprintf(stderr, "  err_msg: %s\n", status.err_msg ? status.err_msg : "<null>");
			fprintf(stderr, "  exitcode: %d\n", status.exitcode);
			printf("Init failed\n");
			goto error_config;
		}

		goto success_config;

error_config:
		PyConfig_Clear(&config);
		fail(env, "configuration failed");
		return;

success_config:

		PyConfig_Clear(&config);
#if  PY_VERSION_HEX<0x030a0000
		PyEval_InitThreads();
#endif
		gstate = PyGILState_Ensure();

		if (!appendModulePathsToSysPath(env, modulePath))
		{
			fail(env, "failed to append module paths to sys.path");
			return;
		}

		jpype = PyImport_ImportModule("jpype");
		jpypep = PyImport_ImportModule("_jpype");
		if (jpypep == NULL)
		{
			printf("missing _jpype\n");
			fflush(stdout);
			fail(env, "_jpype module not found");
			return;
		}
		
		// Import the Python side to create the hooks
		if (jpype == NULL)
		{
			printf("missing jpype\n");
			fflush(stdout);
			fail(env, "jpype module not found");
			return;
		}

		// Usage in your code:
		//print_module_path("jpype", jpype);
		//print_module_path("_jpype", jpypep);

		PyJPModule_loadResources(jpypep);

		// Then attach the private module to the JVM
		context = JPContext_global;
		context->attachJVM(env);

		JPJavaFrame frame = JPJavaFrame::external(env);
		
		// Initialize the resources in the jpype module
		obj = PyObject_GetAttrString(jpype, "_core");
		obj2 = PyObject_GetAttrString(obj, "initializeResources");
		obj3 = PyTuple_New(0);
		PyObject* out = PyObject_Call(obj2, obj3, NULL);
		if (out == NULL) {
			// This will print the full Python traceback to your console
			PyErr_Print(); 
		} else {
			Py_DECREF(out); // Don't forget to decref the result on success!
		}
		Py_DECREF(obj);
		Py_DECREF(obj2);
		Py_DECREF(obj3);

		Py_DECREF(jpype);
		Py_DECREF(jpypep);

		// Next, we need to release the state so we can return to Java.
		PyGILState_Release(gstate);
		fflush(stdout);
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
	try
	{
		JPPyCallAcquire callback;
		PyRun_InteractiveLoop(stdin, "<stdin>");
	} catch (JPypeException& ex)
	{
		convertException(env, ex);
	}	catch (...)
	{
		fail(env, "C++ exception during interactive");
	}
}

JNIEXPORT void JNICALL Java_org_jpype_bridge_Natives_finish
(JNIEnv *env, jclass cls)
{
	try
	{
		printf("A1\n");
		PyGILState_STATE gstate = PyGILState_Ensure();
		JPContext_global->detachJVM();
		printf("A2\n");
		Py_Finalize();
		printf("A3\n");
	} catch (JPypeException& ex)
	{
		convertException(env, ex);
	}	catch (...)
	{
		fail(env, "C++ exception during finish");
	}
}

#ifdef __cplusplus
}
#endif
