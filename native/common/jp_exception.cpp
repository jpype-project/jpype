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
#include <Python.h>
#include <frameobject.h>

#include "jpype.h"
#include "jp_exception.h"
#include "pyjp.h"

PyObject* PyTrace_FromJPStackTrace(JPStackTrace& trace);

bool isJavaThrowable(PyObject* exceptionClass)
{
	JPClass* cls = PyJPClass_getJPClass(exceptionClass);
	if (cls == nullptr)
		return false;
	return cls->isThrowable();
}

/** Shared conversion logic for a Java-originated exception, used by
 * JPJavaError::toPython().
 */
void convertJavaToPython(jthrowable th)
{
	// Welcome to paranoia land, where they really are out to get you!
	JP_TRACE_IN("convertJavaToPython");
	// GCOVR_EXCL_START
	JPContext* context = JPContext_global;
	if (context == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "Unable to convert java error, context is null.");
		return;
	}
	// GCOVR_EXCL_STOP

	// Okay we can get to a frame to talk to the object
	JPJavaFrame frame = JPJavaFrame::external(context->getEnv());
	jvalue v;
	v.l = th;
	// GCOVR_EXCL_START
	// This is condition is only hit if something fails during the initial boot
	if (context->getJavaContext() == nullptr || context->m_Context_GetExcClassID == nullptr)
	{
		PyErr_SetString(PyExc_SystemError, frame.toString(th).c_str());
		return;
	}
	// GCOVR_EXCL_STOP
	jlong pycls = frame.CallLongMethodA(context->getJavaContext(), context->m_Context_GetExcClassID, &v);
	if (pycls != 0)
	{
		jlong value = frame.CallLongMethodA(context->getJavaContext(), context->m_Context_GetExcValueID, &v);
		PyErr_SetObject((PyObject*) pycls, (PyObject*) value);
		return;
	}
	JP_TRACE("Check typemanager");
	// GCOVR_EXCL_START
	if (!context->isRunning())
	{
		PyErr_SetString(PyExc_RuntimeError, frame.toString((jobject) th).c_str());
		return;
	}
	// GCOVR_EXCL_STOP

	// Convert to Python object
	JP_TRACE("Convert to python");
	JPClass* cls = frame.findClassForObject((jobject) th);

	// GCOVR_EXCL_START
	// This sanity check can only fail if the type system fails to find a
	// class for the current exception.
	if (cls == nullptr)
	{
		// Nope, no class found
		PyErr_SetString(PyExc_RuntimeError, frame.toString(th).c_str());
		return;
	}
	// GCOVR_EXCL_STOP

	// Create the exception object (this may fail)
	v.l = th;
	JPPyObject pyvalue = cls->convertToPythonObject(frame, v, false);

	// GCOVR_EXCL_START
	// This sanity check can only be hit if the exception failed during
	// conversion in some extraordinary way.
	if (pyvalue.isNull())
	{
		PyErr_SetString(PyExc_RuntimeError, frame.toString(th).c_str());
		return;
	}
	// GCOVR_EXCL_STOP

	PyObject *type = (PyObject*) Py_TYPE(pyvalue.get());
	Py_INCREF(type);

	// Add cause to the exception
	JPPyObject args = JPPyObject::call(Py_BuildValue("(s)", "Java Exception"));
	JPPyObject cause = JPPyObject::call(PyObject_Call(PyExc_Exception, args.get(), nullptr));
	JPPyObject trace = PyTrace_FromJavaException(frame, th, nullptr);

	// Attach Java causes as well.
	try
	{
		jthrowable jcause = frame.getCause(th);
		if (jcause != nullptr)
		{
			jvalue a;
			a.l = (jobject) jcause;
			JPPyObject prev = context->_java_lang_Object->convertToPythonObject(frame, a, false);
			PyJPException_normalize(frame, prev, jcause, th);
			PyException_SetCause(cause.get(), prev.keep());
		}
		if (trace.get() != nullptr)
			PyException_SetTraceback(cause.get(), trace.get());
		PyException_SetCause(pyvalue.get(), cause.keep());
	}	catch (...)
	{
		JP_TRACE("FAILURE IN CAUSE");
		// Any failures in this optional action should be ignored.
		// worst case we don't print as much diagnostics.
	}

	// Transfer to Python
	PyErr_SetObject(type, pyvalue.get());
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

/** Shared conversion logic for a Python-originated exception (already set
 * on the thread state at call time), used by JPPythonError::toJava() and
 * JPInternalError::toJava(). mesg is only used for the extremely early
 * startup fallback where no JPContext exists yet to build a real exception.
 */
void convertPythonToJava(const char* mesg)
{
	JP_TRACE_IN("convertPythonToJava");
	JPJavaFrame frame = JPJavaFrame::outer();
	JPContext *context = frame.getContext();
	jthrowable th;
	JPPyErrFrame eframe;
	if (eframe.m_good && isJavaThrowable(eframe.m_ExceptionClass.get()))
	{
		eframe.m_good = false;
		JPValue* javaExc = PyJPValue_getJavaSlot(eframe.m_ExceptionValue.get());
		if (javaExc != nullptr)
		{
			th = (jthrowable) javaExc->getJavaObject();
			JP_TRACE("Throwing Java", frame.toString(th));
			frame.Throw(th);
			return;
		}
	}

	if (context->m_Context_CreateExceptionID == nullptr)
	{
		frame.ThrowNew(frame.FindClass("java/lang/RuntimeException"), mesg);
		return;
	}


	// Otherwise
	jvalue v[2];
	v[0].j = (jlong) eframe.m_ExceptionClass.get();
	v[1].j = (jlong) eframe.m_ExceptionValue.get();
	th = (jthrowable) frame.CallObjectMethodA(context->getJavaContext(),
			context->m_Context_CreateExceptionID, v);
	frame.registerRef((jobject) th, eframe.m_ExceptionClass.get());
	frame.registerRef((jobject) th, eframe.m_ExceptionValue.get());
	eframe.clear();
	frame.Throw(th);
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

/**
 * Print a user-facing crash banner for the deliberate fail-fast crashes
 * below (each JPBaseError subclass's toPython()/toJava() catch(...) blocks).
 *
 * These crashes fire only when JPype's own exception-conversion code -
 * which is not allowed to fail - fails anyway, leaving interpreter/JNI
 * error state too corrupted to unwind safely. Terminating immediately is
 * safer than continuing to run on corrupted state, but from a user's
 * perspective this looks exactly like a spontaneous segfault, so we say
 * plainly what happened and how to report it, before the process dies.
 *
 * Coverage is excluded: exercising this path requires corrupting the
 * interpreter state and killing the test process.
 */
// GCOVR_EXCL_START
static void reportFatalFailFast(const char* detail)
{
	fprintf(stderr,
			"================================================================================\n"
			"JPype has hit an internal error it cannot safely recover from and must stop the\n"
			"JVM immediately.\n"
			"\n"
			"This is not something you did wrong in your own code - it means JPype's own\n"
			"exception-handling machinery hit a case it doesn't know how to handle. Please\n"
			"help fix it by reporting this at:\n"
			"\n"
			"    https://github.com/jpype-project/jpype/issues\n"
			"\n"
			"along with the smallest script you can find that reproduces it, and the detail\n"
			"below:\n"
			"\n"
			"    %s\n"
			"\n"
			"JPype is intentionally crashing now rather than continuing to run with\n"
			"corrupted internal state.\n"
			"================================================================================\n",
			detail);
	fflush(stderr);
}
// GCOVR_EXCL_STOP

/** Shared "attach our C++ stack trace as the cause" tail behavior, common to
 * every JPBaseError::toPython() except JPJavaError's (which already attaches
 * its own Java-side cause inside convertJavaToPython()).
 */
static void attachCppCause(const JPStackTrace& trace)
{
	if (!_jp_cpp_exceptions)
		return;
	JPPyErrFrame eframe;
	eframe.normalize();
	JPPyObject args = JPPyObject::call(Py_BuildValue("(s)", "C++ Exception"));
	JPPyObject tb = JPPyObject::call(PyTrace_FromJPStackTrace(const_cast<JPStackTrace&>(trace)));
	JPPyObject cause = JPPyObject::accept(PyObject_Call(PyExc_Exception, args.get(), nullptr));
	// eframe.m_ExceptionValue can be null here - e.g. if the Python error
	// state was already cleared by the time this runs (see
	// JPPyErrFrame::normalize()'s own null-guard for the same class of
	// issue) - PyException_SetCause requires a real exception instance as
	// self and segfaults on null.
	if (!cause.isNull() && eframe.m_ExceptionValue.get() != nullptr)
	{
		PyException_SetTraceback(cause.get(), tb.get());
		PyException_SetCause(eframe.m_ExceptionValue.get(), cause.keep());
	}
}

void JPJavaError::toPython()
{
	JP_TRACE_IN("JPJavaError::toPython");
	try
	{
		if (PyErr_CheckSignals() != 0)
			return;
		if (PyErr_Occurred())
			return;
		convertJavaToPython(getThrowable());
	} catch (...) // GCOVR_EXCL_LINE
	{
		// GCOVR_EXCL_START
		JPTracer::trace("Fatal error in exception handling");
		reportFatalFailFast("An unexpected error occurred while converting a Java exception "
				"back to Python (JPJavaError::toPython).");
		int *i = nullptr;
		*i = 0;
		// GCOVR_EXCL_STOP
	}
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

void JPJavaError::toJava()
{
	JP_TRACE_IN("JPJavaError::toJava");
	if (getThrowable() != nullptr)
	{
		JPContext* context = JPContext_global;
		JPJavaFrame frame = JPJavaFrame::external(context->getEnv());
		JP_TRACE("Java rethrow");
		frame.Throw(getThrowable());
	}
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

void JPPythonError::toPython()
{
	JP_TRACE_IN("JPPythonError::toPython");
	try
	{
		if (PyErr_CheckSignals() != 0)
			return;
		if (PyErr_Occurred())
			return;
		// Restore the exception fetched and normalized at throw time (see
		// JPPythonError::fetch()) - it was deliberately held off the thread
		// state for the whole unwind so no incidental GC pass could disturb
		// it.
		JPPyErr::restore(m_PyExcValue);
		attachCppCause(trace());
	} catch (...) // GCOVR_EXCL_LINE
	{
		// GCOVR_EXCL_START
		JPTracer::trace("Fatal error in exception handling");
		reportFatalFailFast("An unexpected error occurred while restoring a Python exception "
				"(JPPythonError::toPython).");
		int *i = nullptr;
		*i = 0;
		// GCOVR_EXCL_STOP
	}
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

void JPPythonError::toJava()
{
	JP_TRACE_IN("JPPythonError::toJava");
	try
	{
		JPPyCallAcquire callback;
		// convertPythonToJava() expects to find the exception live on the
		// thread state - restore what was fetched off it at throw time.
		JPPyErr::restore(m_PyExcValue);
		convertPythonToJava(what());
	} catch (...) // GCOVR_EXCL_LINE
	{
		// GCOVR_EXCL_START
		JPTracer::trace("Fatal error in exception handling");
		reportFatalFailFast("An unexpected error occurred while JPype was converting a Python "
				"exception into its Java equivalent (JPPythonError::toJava).");
		int *i = nullptr;
		*i = 0;
		// GCOVR_EXCL_STOP
	}
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

void JPInternalError::toPython()
{
	JP_TRACE_IN("JPInternalError::toPython");
	try
	{
		if (PyErr_CheckSignals() != 0)
			return;
		if (PyErr_Occurred())
			return;

		const char* mesg = what();
		if (isOSError())
		{
			// This section is only reachable during startup of the JVM.
			// GCOVR_EXCL_START
			std::stringstream ss;
			ss << "JVM DLL not found: " << mesg;
#ifdef WIN32
			PyObject* val = Py_BuildValue("(izzi)", 2, ss.str().c_str(), NULL, getOSErrorCode());
#else
			PyObject* val = Py_BuildValue("(iz)", getOSErrorCode(), ss.str().c_str());
#endif
			if (val != nullptr)
			{
				PyObject* exc = PyObject_Call(PyExc_OSError, val, nullptr);
				Py_DECREF(val);
				if (exc != nullptr)
				{
					PyErr_SetObject(PyExc_OSError, exc);
					Py_DECREF(exc);
				}
			}
			// GCOVR_EXCL_STOP
		} else
		{
			PyErr_SetString((PyObject*) getPyExcType(), mesg);
		}

		attachCppCause(trace());
	} catch (...) // GCOVR_EXCL_LINE
	{
		// GCOVR_EXCL_START
		JPTracer::trace("Fatal error in exception handling");
		reportFatalFailFast("An unexpected error occurred while issuing a JPype-internal "
				"exception (JPInternalError::toPython).");
		int *i = nullptr;
		*i = 0;
		// GCOVR_EXCL_STOP
	}
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

void JPInternalError::toJava()
{
	JP_TRACE_IN("JPInternalError::toJava");
	try
	{
		JPContext* context = JPContext_global;
		JPJavaFrame frame = JPJavaFrame::external(context->getEnv());
		const char* mesg = what();
		if (!isOSError())
		{
			JPPyCallAcquire callback;
			PyErr_SetString((PyObject*) getPyExcType(), mesg);
			convertPythonToJava(mesg);
			return;
		}
		// GCOVR_EXCL_START
		// OS errors are startup-only and never reach toJava() in practice
		// (no JVM/JNI frame exists yet at that point) - issued as a
		// RuntimeException for parity with the legacy fallback branch.
		frame.ThrowNew(context->m_RuntimeException.get(), mesg);
		// GCOVR_EXCL_STOP
	} catch (...) // GCOVR_EXCL_LINE
	{
		// GCOVR_EXCL_START
		JPTracer::trace("Fatal error in exception handling");
		reportFatalFailFast("An unexpected error occurred while JPype was converting an "
				"internal exception into its Java equivalent (JPInternalError::toJava).");
		int *i = nullptr;
		*i = 0;
		// GCOVR_EXCL_STOP
	}
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

PyObject *tb_create(
		PyObject *last_traceback,
		PyObject *dict,
		const char* filename,
		const char* funcname,
		int linenum)
{
	// Create a code for this frame. (ref count is 1)
	JPPyObject code = JPPyObject::accept((PyObject*)PyCode_NewEmpty(filename, funcname, linenum));

	// If we don't get the code object there is no point
	if (code.get() == nullptr)
		return nullptr;

	// Create a frame for the traceback.
	PyThreadState *state = PyThreadState_GET();
	PyFrameObject *pframe = PyFrame_New(state, (PyCodeObject*) code.get(), dict, NULL);
	JPPyObject frame = JPPyObject::accept((PyObject*)pframe);

	// If we don't get the frame object there is no point
	if (frame.get() == nullptr)
		return nullptr;

	// Create a traceback
#if PY_MINOR_VERSION<11
	JPPyObject lasti = JPPyObject::claim(PyLong_FromLong(pframe->f_lasti));
#else
	JPPyObject lasti = JPPyObject::claim(PyLong_FromLong(PyFrame_GetLasti(pframe)));
#endif
	JPPyObject linenuma = JPPyObject::claim(PyLong_FromLong(linenum));
	JPPyObject tuple = JPPyTuple_Pack(Py_None, frame.get(), lasti.get(), linenuma.get());
	JPPyObject traceback = JPPyObject::accept(PyObject_Call((PyObject*) &PyTraceBack_Type, tuple.get(), NULL));

	// We could fail in process
	if (traceback.get() == nullptr)
	{
		return nullptr;
	}

	return traceback.keep();
}

PyObject* PyTrace_FromJPStackTrace(JPStackTrace& trace)
{
	PyObject *last_traceback = nullptr;
	PyObject *dict = PyModule_GetDict(PyJPModule);
	for (auto& iter : trace)
	{
		last_traceback = tb_create(last_traceback, dict, iter.getFile(),
				iter.getFunction(), iter.getLine());
	}
	if (last_traceback == nullptr)
		Py_RETURN_NONE;
	return last_traceback;
}

JPPyObject PyTrace_FromJavaException(JPJavaFrame& frame, jthrowable th, jthrowable prev)
{
	PyObject *last_traceback = NULL;
	JPContext *context = frame.getContext();
	jvalue args[2];
	args[0].l = th;
	args[1].l = prev;
	if (context->m_Context_GetStackFrameID == nullptr)
		return {};

	JNIEnv* env = frame.getEnv();
	jobjectArray obj = static_cast<jobjectArray>(env->CallObjectMethodA(context->getJavaContext(),
			context->m_Context_GetStackFrameID, args));

	// Eat any exceptions that were generated
	if (env->ExceptionCheck() == JNI_TRUE)
		env->ExceptionClear();

	if (obj == nullptr)
		return {};
	jsize sz = frame.GetArrayLength(obj);
	PyObject *dict = PyModule_GetDict(PyJPModule);
	for (jsize i = 0; i < sz; i += 4)
	{
		string filename, method;
		auto jclassname = static_cast<jstring>(frame.GetObjectArrayElement(obj, i));
		auto jmethodname = static_cast<jstring>(frame.GetObjectArrayElement(obj, i + 1));
		auto jfilename = static_cast<jstring>(frame.GetObjectArrayElement(obj, i + 2));
		if (jfilename != nullptr)
			filename = frame.toStringUTF8(jfilename);
		else
			filename = frame.toStringUTF8(jclassname) + ".java";
		if (jmethodname != nullptr)
			method = frame.toStringUTF8(jclassname) + "." + frame.toStringUTF8(jmethodname);
		jint lineNum =
				frame.CallIntMethodA(frame.GetObjectArrayElement(obj, i + 3), context->_java_lang_Integer->m_IntValueID, nullptr);

		// sending -1 will cause issues on Windows
		if (lineNum<0)
			lineNum = 0;

		last_traceback = tb_create(last_traceback, dict,  filename.c_str(),
				method.c_str(), lineNum);
		frame.DeleteLocalRef(jclassname);
		frame.DeleteLocalRef(jmethodname);
		frame.DeleteLocalRef(jfilename);
	}
	if (last_traceback == nullptr)
		return {};
	return JPPyObject::call((PyObject*) last_traceback);
}
