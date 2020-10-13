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
#include "jp_reference_queue.h"

PyObject* PyTrace_FromJPStackTrace(JPStackTrace& trace);

JPypeException::JPypeException(JPJavaFrame &frame, jthrowable th, const JPStackInfo& stackInfo)
: m_Throwable(frame, th)
{
	JP_TRACE("JAVA EXCEPTION THROWN with java throwable");
	m_Context = frame.getContext();
	m_Type = JPError::_java_error;
	m_Error.l = NULL;
	m_Message = frame.toString(th);
	from(stackInfo);
}

JPypeException::JPypeException(int type, void* error, const JPStackInfo& stackInfo)
{
	JP_TRACE("EXCEPTION THROWN with error", error);
	m_Type = type;
	m_Error.l = error;
	m_Message = "None";
	from(stackInfo);
}

JPypeException::JPypeException(int type, void* errType, const string& msn, const JPStackInfo& stackInfo)
{
	JP_TRACE("EXCEPTION THROWN", errType, msn);
	m_Type = type;
	m_Error.l = errType;
	m_Message = msn;
	from(stackInfo);
}

// GCOVR_EXCL_START
// This is only used during startup for OSError

JPypeException::JPypeException(int type,  const string& msn, int errType, const JPStackInfo& stackInfo)
{
	JP_TRACE("EXCEPTION THROWN", errType, msn);
	m_Type = type;
	m_Error.i = errType;
	m_Message = msn;
	from(stackInfo);
}

JPypeException::JPypeException(const JPypeException& ex)
: m_Context(ex.m_Context), m_Trace(ex.m_Trace), m_Throwable(ex.m_Throwable)
{
	m_Type = ex.m_Type;
	m_Error = ex.m_Error;
	m_Message = ex.m_Message;
}

JPypeException& JPypeException::operator = (const JPypeException& ex)
{
	m_Context = ex.m_Context;
	m_Type = ex.m_Type;
	m_Trace = ex.m_Trace;
	m_Throwable = ex.m_Throwable;
	m_Error = ex.m_Error;
	m_Message = ex.m_Message;
	return *this;
}
// GCOVR_EXCL_STOP

JPypeException::~JPypeException()
{
}

void JPypeException::from(const JPStackInfo& info)
{
	JP_TRACE("EXCEPTION FROM: ", info.getFile(), info.getLine());
	m_Trace.push_back(info);
}

// Okay from this point on we have to suit up in full Kevlar because
// this code must handle every conceivable and still reach a resolution.
// Exceptions may be throws during initialization where only a fraction
// of the resources are available, during the middle of normal operation,
// or worst of all as the system is being yanked out from under us during
// shutdown.  Each and every one of these cases must be considered.
// Further each and every function called here must be hardened similarly
// or they will become the weak link. And remember it is not paranoia if
// they are actually out to get you.
//
// Onward my friends to victory or a glorious segfault!

string JPypeException::getMessage()
{
	JP_TRACE_IN("JPypeException::getMessage");
	// Must be bullet proof
	try
	{
		stringstream str;
		str << m_Message << endl;
		JP_TRACE(str.str());
		return str.str();
		// GCOVR_EXCL_START
	} catch (...)
	{
		return "error during get message";
	}
	JP_TRACE_OUT;
	// GCOVR_EXCL_STOP
}

bool isJavaThrowable(PyObject* exceptionClass)
{
	JPClass* cls = PyJPClass_getJPClass(exceptionClass);
	if (cls == NULL)
		return false;
	return cls->isThrowable();
}

void JPypeException::convertJavaToPython()
{
	// Welcome to paranoia land, where they really are out to get you!
	JP_TRACE_IN("JPypeException::convertJavaToPython");
	// GCOVR_EXCL_START
	if (m_Context == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "Unable to convert java error, context is null.");
		return;
	}
	// GCOVR_EXCL_STOP

	// Okay we can get to a frame to talk to the object
	JPJavaFrame frame = JPJavaFrame::external(m_Context, m_Context->getEnv());
	jthrowable th = m_Throwable.get();
	jvalue v;
	v.l = th;
	// GCOVR_EXCL_START
	// This is condition is only hit if something fails during the initial boot
	if (m_Context->getJavaContext() == NULL || m_Context->m_Context_GetExcClassID == NULL)
	{
		PyErr_SetString(PyExc_SystemError, frame.toString(th).c_str());
		return;
	}
	// GCOVR_EXCL_STOP
	jlong pycls = frame.CallLongMethodA(m_Context->getJavaContext(), m_Context->m_Context_GetExcClassID, &v);
	if (pycls != 0)
	{
		jlong value = frame.CallLongMethodA(m_Context->getJavaContext(), m_Context->m_Context_GetExcValueID, &v);
		PyErr_SetObject((PyObject*) pycls, (PyObject*) value);
		return;
	}
	JP_TRACE("Check typemanager");
	// GCOVR_EXCL_START
	if (!m_Context->isRunning())
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
	if (cls == NULL)
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
	JPPyObject cause = JPPyObject::call(PyObject_Call(PyExc_Exception, args.get(), NULL));
	JPPyObject trace = PyTrace_FromJavaException(frame, th, NULL);

	// Attach Java causes as well.
	try
	{
		jthrowable jcause = frame.getCause(th);
		if (jcause != NULL)
		{
			jvalue a;
			a.l = (jobject) jcause;
			JPPyObject prev = frame.getContext()->_java_lang_Object->convertToPythonObject(frame, a, false);
			PyJPException_normalize(frame, prev, jcause, th);
			PyException_SetCause(cause.get(), prev.keep());
		}
		PyException_SetTraceback(cause.get(), trace.get());
		PyException_SetCause(pyvalue.get(), cause.keep());
	}	catch (JPypeException& ex)
	{
		JP_TRACE("FAILURE IN CAUSE");
		// Any failures in this optional action should be ignored.
		// worst case we don't print as much diagnostics.
	}

	// Transfer to Python
	PyErr_SetObject(type, pyvalue.get());
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

void JPypeException::convertPythonToJava(JPContext* context)
{
	JP_TRACE_IN("JPypeException::convertPythonToJava");
	JPJavaFrame frame = JPJavaFrame::outer(context);
	jthrowable th;
	JPPyErrFrame eframe;
	if (eframe.good && isJavaThrowable(eframe.m_ExceptionClass.get()))
	{
		eframe.good = false;
		JPValue* javaExc = PyJPValue_getJavaSlot(eframe.m_ExceptionValue.get());
		if (javaExc != NULL)
		{
			th = (jthrowable) javaExc->getJavaObject();
			JP_TRACE("Throwing Java", frame.toString(th));
			frame.Throw(th);
			return;
		}
	}

	if (context->m_Context_CreateExceptionID == NULL)
	{
		frame.ThrowNew(frame.FindClass("java/lang/RuntimeException"), getMessage().c_str());
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

int JPError::_java_error = 1;
int JPError::_python_error = 2;
int JPError::_python_exc = 3;
int JPError::_os_error_unix = 10;
int JPError::_os_error_windows = 11;
int JPError::_method_not_found = 20;

void JPypeException::toPython()
{
	string mesg;
	JP_TRACE_IN("JPypeException::toPython");
	try
	{
		// Check the signals before processing the exception
		// It may be a signal when interrupted Java in which case
		// the signal takes precedence.
		if (PyErr_CheckSignals()!=0)
			return;

		mesg = getMessage();
		JP_TRACE(m_Error.l);
		JP_TRACE(mesg.c_str());
		if (m_Type == JPError::_java_error)
		{
			JP_TRACE("Java exception");
			JPypeException::convertJavaToPython();
			return;
		} else if (m_Type == JPError::_python_error)
		{
			// Already on the stack
		} else if (m_Type == JPError::_method_not_found)
		{
			// This is hit when a proxy fails to implement a required
			// method.  Only older style proxies should be able hit this.
			JP_TRACE("Runtime error");
			PyErr_SetString(PyExc_RuntimeError, mesg.c_str());
		}// This section is only reachable during startup of the JVM.
			// GCOVR_EXCL_START
		else if (m_Type == JPError::_os_error_unix)
		{
			std::stringstream ss;
			ss << "JVM DLL not found: " << mesg;
			PyObject* val = Py_BuildValue("(iz)", m_Error.i,
					ss.str().c_str());
			if (val != NULL)
			{
				PyObject* exc = PyObject_Call(PyExc_OSError, val, NULL);
				Py_DECREF(val);
				if (exc != NULL)
				{
					PyErr_SetObject(PyExc_OSError, exc);
					Py_DECREF(exc);
				}
			}
		} else if (m_Type == JPError::_os_error_windows)
		{
			std::stringstream ss;
			ss << "JVM DLL not found: " << mesg;
			PyObject* val = Py_BuildValue("(izzi)", 2,
					ss.str().c_str(), NULL, m_Error.i);
			if (val != NULL)
			{
				PyObject* exc = PyObject_Call(PyExc_OSError, val, NULL);
				Py_DECREF(val);
				if (exc != NULL)
				{
					PyErr_SetObject(PyExc_OSError, exc);
					Py_DECREF(exc);
				}
			}
		}// GCOVR_EXCL_STOP

		else if (m_Type == JPError::_python_exc)
		{
			// All others are Python errors
			JP_TRACE(Py_TYPE(m_Error.l)->tp_name);
			PyErr_SetString((PyObject*) m_Error.l, mesg.c_str());
		} else
		{
			// This should not be possible unless we failed to cover one of the
			// exception type codes.
			JP_TRACE("Unknown error");
			PyErr_SetString(PyExc_RuntimeError, mesg.c_str()); // GCOVR_EXCL_LINE
		}

		// Attach our info as the cause
		if (_jp_cpp_exceptions)
		{
			JPPyErrFrame eframe;
			eframe.normalize();
			JPPyObject args = JPPyObject::call(Py_BuildValue("(s)", "C++ Exception"));
			JPPyObject trace = JPPyObject::call(PyTrace_FromJPStackTrace(m_Trace));
			JPPyObject cause = JPPyObject::accept(PyObject_Call(PyExc_Exception, args.get(), NULL));
			if (!cause.isNull())
			{
				PyException_SetTraceback(cause.get(), trace.get());
				PyException_SetCause(eframe.m_ExceptionValue.get(), cause.keep());
			}
		}
	}// GCOVR_EXCL_START
	catch (JPypeException& ex)
	{
		// Print our parting words
		JPTracer::trace("Fatal error in exception handling");
		JPTracer::trace("Handling:", mesg);
		JPTracer::trace("Type:", m_Error.l);
		if (ex.m_Type == JPError::_python_error)
		{
			JPPyErrFrame eframe;
			JPTracer::trace("Inner Python:", ((PyTypeObject*) eframe.m_ExceptionClass.get())->tp_name);
			return;  // Let these go to Python so we can see the error
		} else if (ex.m_Type == JPError::_java_error)
			JPTracer::trace("Inner Java:", ex.getMessage());
		else
			JPTracer::trace("Inner:", ex.getMessage());

		JPStackInfo info = ex.m_Trace.front();
		JPTracer::trace(info.getFile(), info.getFunction(), info.getLine());

		// Heghlu'meH QaQ jajvam!
		PyErr_SetString(PyExc_RuntimeError, "Fatal error occurred");
		return;
	} catch (...)
	{
		// urp?!
		JPTracer::trace("Fatal error in exception handling");

		// You shall not pass!
		int *i = 0;
		*i = 0;
	}
	// GCOVR_EXCL_STOP
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

void JPypeException::toJava(JPContext *context)
{
	JP_TRACE_IN("JPypeException::toJava");
	try
	{
		string mesg = getMessage();
		JPJavaFrame frame = JPJavaFrame::external(context, context->getEnv());
		if (m_Type == JPError::_java_error)
		{
			JP_TRACE("Java exception");
			//JP_TRACE(context->toString((jobject) frame.ExceptionOccurred()));
			if (m_Throwable.get() != 0)
			{
				JP_TRACE("Java rethrow");
				frame.Throw(m_Throwable.get());
				return;
			}
			return;
		}

		if (m_Type == JPError::_method_not_found)
		{
			frame.ThrowNew(context->m_NoSuchMethodError.get(), mesg.c_str());
			return;
		}

		if (m_Type == JPError::_python_error)
		{
			JPPyCallAcquire callback;
			JP_TRACE("Python exception");
			convertPythonToJava(context);
			return;
		}

		if (m_Type == JPError::_python_exc)
		{
			JPPyCallAcquire callback;
			// All others are Python errors
			JP_TRACE(Py_TYPE(m_Error.l)->tp_name);
			PyErr_SetString((PyObject*) m_Error.l, mesg.c_str());
			convertPythonToJava(context);
			return;
		}

		// All others are issued as RuntimeExceptions
		JP_TRACE("String exception");
		frame.ThrowNew(context->m_RuntimeException.get(), mesg.c_str());
		return;
	}	catch (JPypeException& ex)  // GCOVR_EXCL_LINE
	{	// GCOVR_EXCL_START
		// Print our parting words.
		JPTracer::trace("Fatal error in exception handling");
		JPStackInfo info = ex.m_Trace.front();
		JPTracer::trace(info.getFile(), info.getFunction(), info.getLine());

		// Take one for the team.
		int *i = 0;
		*i = 0;
		// GCOVR_EXCL_STOP
	} catch (...) // GCOVR_EXCL_LINE
	{
		// GCOVR_EXCL_START
		// urp?!
		JPTracer::trace("Fatal error in exception handling");

		// It is pointless, I can't go on.
		int *i = 0;
		*i = 0;
		// GCOVR_EXCL_STOP
	}
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

PyTracebackObject *tb_create(
		PyTracebackObject *last_traceback,
		PyObject *dict,
		const char* filename,
		const char* funcname,
		int linenum)
{
	// Create a code for this frame.
	PyCodeObject *code = PyCode_NewEmpty(filename, funcname, linenum);
	// Create a frame for the traceback.
	PyFrameObject *frame = (PyFrameObject*) PyFrame_Type.tp_alloc(&PyFrame_Type, 0);
	frame->f_back = NULL;
	if (last_traceback != NULL)
	{
		frame->f_back = last_traceback->tb_frame;
		Py_INCREF(frame->f_back);
	}
	frame->f_builtins = dict;
	Py_INCREF(frame->f_builtins);
	frame->f_code = (PyCodeObject*) code;
	frame->f_executing = 0;
	frame->f_gen = NULL;
	frame->f_globals = dict;
	Py_INCREF(frame->f_globals);
	frame->f_iblock = 0;
	frame->f_lasti = 0;
	frame->f_lineno = 0;
	frame->f_locals = NULL;
	frame->f_localsplus[0] = 0;
	frame->f_stacktop = NULL;
	frame->f_trace = NULL;
	frame->f_valuestack = 0;
#if PY_VERSION_HEX>=0x03070000
	frame->f_trace_lines = 0;
	frame->f_trace_opcodes = 0;
#endif

	// Create a traceback
	PyTracebackObject *traceback = (PyTracebackObject*)
			PyTraceBack_Type.tp_alloc(&PyTraceBack_Type, 0);
	traceback->tb_frame = frame;
	traceback->tb_lasti = frame->f_lasti;
	traceback->tb_lineno = linenum;
	traceback->tb_next = last_traceback;
	return traceback;
}

PyObject* PyTrace_FromJPStackTrace(JPStackTrace& trace)
{
	PyTracebackObject *last_traceback = NULL;
	PyObject *dict = PyModule_GetDict(PyJPModule);
	for (JPStackTrace::iterator iter = trace.begin(); iter != trace.end(); ++iter)
	{
		last_traceback = tb_create(last_traceback, dict,  iter->getFile(),
				iter->getFunction(), iter->getLine());
	}
	if (last_traceback == NULL)
		Py_RETURN_NONE;
	return (PyObject*) last_traceback;
}

JPPyObject PyTrace_FromJavaException(JPJavaFrame& frame, jthrowable th, jthrowable prev)
{
	PyTracebackObject *last_traceback = NULL;
	JPContext *context = frame.getContext();
	jvalue args[2];
	args[0].l = th;
	args[1].l = prev;
	if (context->m_Context_GetStackFrameID == NULL)
		return JPPyObject();

	JNIEnv* env = frame.getEnv();
	jobjectArray obj = (jobjectArray) env->CallObjectMethodA(context->getJavaContext(),
			context->m_Context_GetStackFrameID, args);

	// Eat any exceptions that were generated
	if (env->ExceptionCheck() == JNI_TRUE)
		env->ExceptionClear();

	if (obj == NULL)
		return JPPyObject();
	jsize sz = frame.GetArrayLength(obj);
	PyObject *dict = PyModule_GetDict(PyJPModule);
	for (jsize i = 0; i < sz; i += 4)
	{
		string filename, method;
		jstring jclassname = (jstring) frame.GetObjectArrayElement(obj, i);
		jstring jmethodname = (jstring) frame.GetObjectArrayElement(obj, i + 1);
		jstring jfilename = (jstring) frame.GetObjectArrayElement(obj, i + 2);
		if (jfilename != NULL)
			filename = frame.toStringUTF8(jfilename);
		else
			filename = frame.toStringUTF8(jclassname) + ".java";
		if (jmethodname != NULL)
			method = frame.toStringUTF8(jclassname) + "." + frame.toStringUTF8(jmethodname);
		jint lineNum =
				frame.CallIntMethodA(frame.GetObjectArrayElement(obj, i + 3), context->_java_lang_Integer->m_IntValueID, 0);

		last_traceback = tb_create(last_traceback, dict,  filename.c_str(),
				method.c_str(), lineNum);
		frame.DeleteLocalRef(jclassname);
		frame.DeleteLocalRef(jmethodname);
		frame.DeleteLocalRef(jfilename);
	}
	if (last_traceback == NULL)
		return JPPyObject();
	return JPPyObject::call((PyObject*) last_traceback);
}
