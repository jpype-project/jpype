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
#include "jp_gc.h"
#include <list>
#include <iostream>
#include <sstream>
#include <cwchar>
#include <cstdlib>
#include <climits>

#ifdef __cplusplus
extern "C" {
#endif

// gcvisitproc callback matching CPython's functional signature
//   rc=1 is keep going
static int jpype_eject_visitor(PyObject* obj, void* arg)
{
	(void) arg;
	if (obj == NULL)
		return 1;

	// PyJPValue_finalize already handles every current per-instance-slot
	// shape (a full inline JPValue on a _JClass type object, a plain
	// jvalue on an ordinary wrapped instance, or nothing at all for a
	// JValueFn-reconstructing family) and is a safe no-op on an object
	// with no Java-side state -- calling it directly here, ahead of the
	// interpreter's own GC/dealloc sweep, disconnects the Java-side
	// global ref before Py_EndInterpreter tears down the heap, so this
	// object's normal tp_finalize (if CPython still reaches it) becomes
	// a safe no-op instead of touching a torn-down context.
	PyJPValue_finalize(obj);
	return 1; // Standard Py_VISIT success signal to keep going
}

static void fail(JNIEnv *env, const char* msg)
{
	// This is a low frequency path so we don't need efficiency.
	jclass runtimeException = env->FindClass("java/lang/RuntimeException");
	env->ThrowNew(runtimeException, msg);
}

static void convertException(JNIEnv *env, JPBaseError& ex)
{
	// This is a low frequency path so we don't need efficiency.
	// We can't use ex.toJava(context) unconditionally because this is part
	// of initialization -- there is no JPContext to pass yet this early
	// (this JNI entry point is the one establishing a JPContext for this
	// interpreter in the first place, not consuming an already-attached
	// one).
	jclass runtimeException = env->FindClass("java/lang/RuntimeException");

	// A Java-originated exception carries its own live jthrowable and
	// never needs a context to rethrow it.
	auto* javaError = dynamic_cast<JPJavaError*>(&ex);
	if (javaError != nullptr)
	{
		env->Throw(javaError->getThrowable());
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

// Resolves both paths to a canonical, symlink-free absolute form before
// comparing, so a bind mount, a relative path, or a symlinked venv doesn't
// produce a false-positive mismatch. Falls back to a literal compare if
// either path can't be resolved (e.g. doesn't exist) -- that's still a
// meaningful signal (they clearly aren't "the same file" if one can't even
// be stat'd), not a reason to skip the check.
static bool sameFile(const char* a, const char* b)
{
#ifdef WIN32
	char ra[MAX_PATH];
	char rb[MAX_PATH];
	char* pa = _fullpath(ra, a, MAX_PATH);
	char* pb = _fullpath(rb, b, MAX_PATH);
	if (pa == nullptr || pb == nullptr)
		return std::string(a) == std::string(b);
	return _stricmp(pa, pb) == 0;
#else
	char ra[PATH_MAX];
	char rb[PATH_MAX];
	char* pa = realpath(a, ra);
	char* pb = realpath(b, rb);
	if (pa == nullptr || pb == nullptr)
		return std::string(a) == std::string(b);
	return std::string(pa) == std::string(pb);
#endif
}

JPContext* launch(JNIEnv* env, jobject interpreter, const char* expectedJpypeLib)
{
	JPContext* context;

	// We need the interpreter copy of these resources
	JPPyObject jpype = JPPyObject::accept(PyImport_ImportModule("jpype"));
	JPPyObject jpypep = JPPyObject::accept(PyImport_ImportModule("_jpype"));

	if (!jpypep.isValid())
	{
		printf("missing _jpype\n");
		fflush(stdout);
		fail(env, "_jpype module not found");
		return nullptr;
	}

	// Import the Python side to create the hooks
	if (!jpype.isValid())
	{
		printf("missing jpype\n");
		fflush(stdout);
		fail(env, "jpype module not found");
		return nullptr;
	}

	// Consistency guard: Java's installNatives() already dlopen'd a
	// specific _jpype.so via System.load() -- the exact file the detective
	// probe resolved (expectedJpypeLib, passed down from
	// Launcher.getJpypeLibrary()). If Python's own `import _jpype` just
	// above landed on a *different* file (e.g. a stray dev-tree copy
	// shadowing the real install on PYTHONPATH/sys.path), this process now
	// has two independently-loaded copies of the same native code, and
	// every pointer-identity check in the C++ layer (starting with
	// PyJPClass_isWrapperMeta's tp_dealloc comparison) silently breaks --
	// surfacing many frames later as a baffling "Missing Java slot" crash
	// on the very first class constructed, with no hint of the real cause.
	// Catch it here instead, immediately and by name. See
	// plan/MissingJavaSlotBootstrapBug.md for the incident this guards
	// against.
	if (expectedJpypeLib != nullptr)
	{
		JPPyObject fileAttr = JPPyObject::accept(PyObject_GetAttrString(jpypep.get(), "__file__"));
		const char* actualPath = fileAttr.isValid() ? PyUnicode_AsUTF8(fileAttr.get()) : nullptr;
		if (actualPath != nullptr && !sameFile(actualPath, expectedJpypeLib))
		{
			std::stringstream ss;
			ss << "_jpype loaded from two different files in the same process: "
				<< "Java's System.load() loaded '" << expectedJpypeLib << "', but "
				<< "Python's own `import _jpype` resolved to '" << actualPath << "'. "
				<< "This leaves two independent copies of the native module loaded "
				<< "at once and corrupts interpreter state. This almost always means "
				<< "a stray _jpype.so/_jpyne.so is shadowing the real install on "
				<< "PYTHONPATH/sys.path -- remove it.";
			fail(env, ss.str().c_str());
			return nullptr;
		}
	}

	// The interpreter specific context will contain our fresh JPContext
	PyJPModuleState* st = reinterpret_cast<PyJPModuleState*>(PyModule_GetState(jpypep.get()));

	// Then attach the private module to the JVM
	context = st->context;

	// JContext needs a back reference to the starting interpreter
	context->m_Interpreter = env->NewGlobalRef(interpreter);

	// It needs to be told Java is already running
	context->attachJVM(env);
	JPJavaFrame frame = JPJavaFrame::external(env, context);

	// CRITICAL: Load resources from _jpype module BEFORE calling initializeResources
	// At this point, jpype._core has created empty placeholder dicts: _jpype._concrete, _protocol, _methods
	// This loads references to those (empty) dicts into the C module state
	PyJPModule_loadResources(jpypep.get(), st);

	// Now that PyJPModule_loadResources() has populated st->gc_callbacks and
	// st->collect, JPGarbageCollection::init can safely register its Python
	// gc hook. This runs once per interpreter (launch() is the single entry
	// point for both the main interpreter and every subinterpreter), so main
	// and each subinterpreter each get their own correctly-hooked instance.
	context->m_GC->init(frame);

	// Initialize the resources in the jpype module
	// This calls _jbridge.initialize() which POPULATES the _concrete, _protocol, _methods dicts
	// Since C module holds references to the same dict objects, it sees the populated entries
	JPPyObject obj = JPPyObject::call(PyObject_GetAttrString(jpype.get(), "_core"));
	JPPyObject obj2 = JPPyObject::call(PyObject_GetAttrString(obj.get(), "initializeResources"));
	JPPyObject obj3 = JPPyObject::call(PyTuple_New(0));
	JPPyObject out = JPPyObject::call(PyObject_Call(obj2.get(), obj3.get(), NULL));

	return context;
}

/* Arguments we need to push in.
 * 
 * A list of module_search_paths so this can be used of limited/embedded deployments.
 * A list of command line arguments so we can execute command line functionality.
 */
JNIEXPORT jobject JNICALL Java_org_jpype_internal_NativeLauncherControl_startMain
(JNIEnv *env, jclass cls, jobjectArray modulePath, jobjectArray args,
	jstring name, jstring home, jstring executable,
	jboolean isolated, jboolean faulthandler, jboolean quiet, jboolean verbose,
	jboolean site_import, jboolean user_site, jboolean bytecode, jobject interpreter,
	jstring expectedJpypeLibrary)
{
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
		// prefix/exec_prefix are not accepted as parameters here - Python
		// calculates them itself from 'home', and there was never a caller
		// that set them (config.prefix/config.exec_prefix were dead fields).

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
		return nullptr;

success_config:

		PyConfig_Clear(&config);
#if  PY_VERSION_HEX<0x030a0000
		PyEval_InitThreads();
#endif
		gstate = PyGILState_Ensure();

		// Module paths are already in sys.path via config.module_search_paths
		// (step 5 above, before Py_InitializeFromConfig) - no separate
		// post-init append here. There used to be one; it duplicated every
		// entry in sys.path, since Py_InitializeFromConfig already applies
		// config.module_search_paths to sys.path during initialization.
		const char* expectedLibCstr = expectedJpypeLibrary == nullptr
			? nullptr : env->GetStringUTFChars(expectedJpypeLibrary, nullptr);
		JPContext* context = launch(env, interpreter, expectedLibCstr);
		if (expectedLibCstr != nullptr)
			env->ReleaseStringUTFChars(expectedJpypeLibrary, expectedLibCstr);
		if (context == nullptr)
		{
			// launch() already raised a Java exception (fail()/convertException)
			// on every failure path - just unwind without touching a null
			// context, matching startSubInterpreter's identical check below.
			PyEval_SaveThread();
			return nullptr;
		}
		// Py_InitializeFromConfig() leaves the calling thread already holding
		// the GIL, so the PyGILState_Ensure() above found it already locked
		// and returned PyGILState_LOCKED. Per the GILState API contract,
		// PyGILState_Release(PyGILState_LOCKED) is a deliberate no-op - it
		// restores "prior state," and prior state was locked - so it can
		// never undo the implicit hold from initialization. We must instead
		// explicitly drop the GIL with PyEval_SaveThread() so control
		// actually returns to Java without holding it; otherwise this thread
		// holds the GIL forever and any other thread's first
		// PyGILState_Ensure() blocks indefinitely.
		(void) gstate;
		PyEval_SaveThread();
		fflush(stdout);
		return context->getJavaContext();

	} catch (JPBaseError& ex)
	{
		// Reachable only after the PyGILState_Ensure() above succeeded (every
		// earlier failure path uses `goto error_config`/returns directly, not
		// a throw) - so the GIL is always held here. convertException() needs
		// the GIL to inspect the Python exception state, so release it only
		// *after* that call, not before (releasing first crashes inside
		// PyErr_Occurred() - confirmed the hard way). Same leak hazard as the
		// success path above: without this, this thread holds the GIL
		// forever and a later shutdown call on another thread deadlocks.
		convertException(env, ex);
		PyEval_SaveThread();
	}	catch (...)
	{
		fail(env, "C++ exception during start");
		PyEval_SaveThread();
	}
	return nullptr;
}

JNIEXPORT jobject JNICALL Java_org_jpype_internal_NativeLauncherControl_startSubInterpreter
(JNIEnv *env, jclass cls, jobject interpreter,
 jboolean useMainObmalloc, jboolean allowFork, jboolean allowExec,
 jboolean allowThreads, jboolean allowDaemonThreads,
 jboolean checkMultiInterpExtensions, jboolean ownGil)
{
#if PY_VERSION_HEX<0x030c0000
	fail(env, "Subinterpreters not supported");
	return nullptr;
#else

	try
	{
		// CPython's Python/pylifecycle.c's init_interp_settings() enforces
		// exactly one hard cross-field rule: use_main_obmalloc=0 requires
		// check_multi_interp_extensions=1 ("per-interpreter obmalloc does
		// not support single-phase init extension modules" - single-phase
		// modules leak state via PyModuleDef.m_base.m_copy). jpype's
		// SubInterpreterBuilder (org.jpype) validates that same rule
		// Java-side before this call is ever reached, so a violation here
		// would indicate a bug in that validation, not a legitimate caller
		// path. check_multi_interp_extensions=1 additionally requires every
		// extension module the subinterpreter imports (including _jpype
		// itself) to declare multi-phase-init support, so this is
		// reachable for real use now, not just legacy shared-GIL/
		// shared-obmalloc configs.
		PyInterpreterConfig config = {
			.use_main_obmalloc = useMainObmalloc,
			.allow_fork = allowFork,
			.allow_exec = allowExec,
			.allow_threads = allowThreads,
			.allow_daemon_threads = allowDaemonThreads,
			.check_multi_interp_extensions = checkMultiInterpExtensions,
			.gil = ownGil ? PyInterpreterConfig_OWN_GIL : PyInterpreterConfig_DEFAULT_GIL
		};
		PyThreadState *tstate = NULL;

		// Create the subinterpreter
		PyStatus status = Py_NewInterpreterFromConfig(&tstate, &config);
		if (PyStatus_Exception(status)) {
			fail(env, "Failed to create subinterpreter");
			return 0;
		}

		// A subinterpreter re-imports _jpype into its own sys.modules by
		// design (per-interpreter GIL support - see PyJPClass_isWrapperMeta's
		// comment), but always from the one file already `dlopen`'d for the
		// main interpreter -- there's no separate System.load() here to
		// diverge from, so the two-different-files guard in launch() doesn't
		// apply and there's no expected path to pass.
		JPContext* context = launch(env, interpreter, nullptr);
		// launch() already raised a Java exception (fail()/convertException) on
		// any failure path - don't dereference a null context here.
		if (context == nullptr)
		{
			Py_EndInterpreter(tstate);
			PyThreadState_Swap(nullptr);
			return nullptr;
		}
		// Remember the interpreter's original thread state so finishSub() can
		// reattach to it later instead of creating an orphan-causing second one.
		context->modulestate->root_tstate = tstate;
		// Next, we need to release the state so we can return to Java.
		jobject javaContext = context->getJavaContext();
		PyThreadState_Swap(nullptr);
		fflush(stdout);
		return javaContext;
	}
	catch (...)
	{
		fail(env, "C++ exception during subinterpreter start");
	}
	return nullptr;
#endif
}


JNIEXPORT void JNICALL Java_org_jpype_internal_NativeLauncherControl_interactive
(JNIEnv *env, jclass cls, jlong ctx)
{
	auto* context = (JPContext*) ctx;
	try
	{
		JPPyCallAcquire callback(context->modulestate);
		PyRun_InteractiveLoop(stdin, "<stdin>");
	} catch (JPBaseError& ex)
	{
		convertException(env, ex);
	}	catch (...)
	{
		fail(env, "C++ exception during interactive");
	}
}

extern "C" PyThreadState* _PyThreadState_GetCurrent(void);

JNIEXPORT void JNICALL Java_org_jpype_internal_NativeLauncherControl_finishSub
(JNIEnv *env, jclass cls, jlong ctx)
{
#if PY_VERSION_HEX<0x030c0000
	fail(env, "Subinterpreters not supported");
#else
	JPContext* context = (JPContext*) ctx;
	PyJPModuleState* st = context->modulestate;

	// Py_EndInterpreter requires the calling thread to be attached to the
	// target interpreter via the *current* thread state, AND requires that
	// state to be the interpreter's only remaining thread state. The calling
	// Java thread is usually attached to nothing at all (the common case,
	// since startSubInterpreter() released the GIL before returning to
	// Java), but it may already be attached to this exact subinterpreter
	// (e.g. a callback thread that never left it). Handle both: only
	// reattach (and remember what to restore afterward) if we aren't already
	// attached to the right interpreter - and reattach to the interpreter's
	// own root_tstate (the one Py_NewInterpreterFromConfig originally
	// returned), not a freshly created one, since a fresh one would leave
	// root_tstate as an orphaned second thread state and make
	// Py_EndInterpreter fail with "not the last thread".
	PyThreadState *current = _PyThreadState_GetCurrent();
	PyThreadState *restoreTo = nullptr;
	PyThreadState *tstate;
	if (current != nullptr && current->interp == st->interp_state)
	{
		tstate = current;
	}
	else
	{
		tstate = st->root_tstate;
		restoreTo = PyThreadState_Swap(tstate);
	}

	try
	{
		// Mark interpreter as shutting down - prevents JPPyCallAcquire from trying to acquire GIL
		st->is_shutting_down = true;

		// Boot all resources held by Python objects
		PyUnstable_GC_VisitObjects(jpype_eject_visitor, (void*)env);

		// Boot all class references
		context->detachJVM();

		// Note: Py_EndInterpreter will automatically clear modules
		// initialized in this interpreter (including _jpype)
		Py_EndInterpreter(tstate);

		// Restore whatever the calling thread was attached to before (often
		// nullptr, since Java threads don't otherwise hold a thread state).
		PyThreadState_Swap(restoreTo);
	}
	catch (...)
	{
		fail(env, "Error during subinterpreter shutdown");
	}
#endif
}

JNIEXPORT void JNICALL Java_org_jpype_internal_NativeLauncherControl_finishMain
(JNIEnv *env, jclass cls, jlong ctx)
{
	try
	{
		JPContext* context = (JPContext*) ctx;
		PyGILState_STATE gstate = PyGILState_Ensure();
		// Mark interpreter as shutting down - prevents JPPyCallAcquire from trying to acquire GIL
		context->modulestate->is_shutting_down = true;
		context->detachJVM();
		Py_Finalize();
	} catch (JPBaseError& ex)
	{
		convertException(env, ex);
	}	catch (...)
	{
		fail(env, "C++ exception during finish");
	}
}

// Reports whether the calling thread currently holds the GIL. Callable from
// Java at any point (does not require the GIL itself) so leaked or
// double-released holds on undiscovered code paths can be asserted against
// from a point where Java is guaranteed to already have control back.
JNIEXPORT jboolean JNICALL Java_org_jpype_internal_NativeLauncherControl_isGilHeld
(JNIEnv *env, jclass cls)
{
#if PY_VERSION_HEX>=0x030c0000
	// PyGILState_Check() only reflects PyGILState_Ensure()/Release() calls -
	// its TLS bookkeeping (autoTSSkey) is never touched by the manual
	// PyThreadState_Swap()-based attach/detach that JPPyCallAcquire and
	// startSubInterpreter()/finishSub() use for subinterpreters (see
	// jp_pythontypes.cpp, jp_bridge.cpp). On a thread that has ever called
	// PyGILState_Ensure() for the *main* interpreter (e.g. the JVM thread
	// that called MainInterpreter.start()) and later merely swaps in and
	// back out of a subinterpreter's raw thread state, PyGILState_Check()
	// keeps reporting the stale main-interpreter "locked" state even after
	// the subinterpreter swap has correctly detached - a false positive,
	// not a real leak (proven directly by
	// SubInterpreterNGTest#testConcurrentThreadDoesNotBlockAfterSubEval: a
	// second thread's independent GIL acquisition never blocks on it).
	//
	// _PyThreadState_GetCurrent() instead reports genuine attach state for
	// *any* attach mechanism (PyGILState_Ensure included - it also ends up
	// setting the current thread state), so it is the correct check for
	// both the main-interpreter and subinterpreter code paths. (Declared
	// extern "C" above, near finishSub().)
	return (_PyThreadState_GetCurrent() != nullptr) ? JNI_TRUE : JNI_FALSE;
#else
	// Pre-3.12: no subinterpreter support, no manual thread-state swapping -
	// PyGILState_Check() is the documented, reliable check.
	return PyGILState_Check() ? JNI_TRUE : JNI_FALSE;
#endif
}

#ifdef __cplusplus
}
#endif
