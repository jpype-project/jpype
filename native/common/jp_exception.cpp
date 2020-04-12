/*****************************************************************************
   Copyright 2004-2008 Steve MÃ©nard

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

 *****************************************************************************/
#include <Python.h>
#include <frameobject.h>
#include "jpype.h"
#include "jp_exception.h"
#include "pyjp.h"
#include "jp_reference_queue.h"

PyObject* PyTrace_FromJPStackTrace(JPStackTrace& trace);
PyObject *PyTrace_FromJavaException(JPJavaFrame& frame, jthrowable th);

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
// GCOVR_EXCL_STOP

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
	JPJavaFrame frame(m_Context, m_Context->getEnv());
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
	JPPyObject args(JPPyRef::_call, Py_BuildValue("(s)", "Java Exception"));
	JPPyObject cause(JPPyRef::_call, PyObject_Call(PyExc_Exception, args.get(), NULL));
	JPPyObject trace(JPPyRef::_call, PyTrace_FromJavaException(frame, th));
	PyException_SetTraceback(cause.get(), trace.get());
	PyException_SetCause(pyvalue.get(), cause.keep());

	// Transfer to Python
	PyErr_SetObject(type, pyvalue.get());
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

void JPypeException::convertPythonToJava(JPContext* context)
{
	JP_TRACE_IN("JPypeException::convertPythonToJava");
	JPJavaFrame frame(context);
	jthrowable th;
	JPPyErrFrame eframe;
	if (eframe.good && isJavaThrowable(eframe.exceptionClass.get()))
	{
		eframe.good = false;
		JPValue* javaExc = PyJPValue_getJavaSlot(eframe.exceptionValue.get());
		if (javaExc != NULL)
		{
			th = (jthrowable) javaExc->getJavaObject();
			JP_TRACE("Throwing Java", frame.toString(th));
			frame.Throw(th);
			return;
		}
	}

	// Otherwise
	jvalue v[2];
	v[0].j = (jlong) eframe.exceptionClass.get();
	v[1].j = (jlong) eframe.exceptionValue.get();
	th = (jthrowable) frame.CallObjectMethodA(context->getJavaContext(),
			context->m_Context_CreateExceptionID, v);
	context->getReferenceQueue()->registerRef((jobject) th,
			eframe.exceptionClass.get());
	context->getReferenceQueue()->registerRef((jobject) th,
			eframe.exceptionValue.get());
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
			JPPyObject args(JPPyRef::_call, Py_BuildValue("(s)", "C++ Exception"));
			JPPyObject trace(JPPyRef::_call, PyTrace_FromJPStackTrace(m_Trace));
			JPPyObject cause(JPPyRef::_accept, PyObject_Call(PyExc_Exception, args.get(), NULL));
			if (!cause.isNull())
			{
				PyException_SetTraceback(cause.get(), trace.get());
				PyException_SetCause(eframe.exceptionValue.get(), cause.keep());
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
			JPTracer::trace("Inner Python:", ((PyTypeObject*) eframe.exceptionClass.get())->tp_name);
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
		JPJavaFrame frame(context, context->getEnv());
		if (m_Type == JPError::_python_error)
		{
			JP_TRACE("Python exception");
			convertPythonToJava(context);
			return;
		}

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
		frame->f_back = last_traceback->tb_frame;
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

namespace
{
jmethodID s_Throwable_GetStackTraceID;
jmethodID s_StackTraceElement_GetFileName;
jmethodID s_StackTraceElement_GetMethodName;
jmethodID s_StackTraceElement_GetLineNumber;
jmethodID s_StackTraceElement_GetClassName;
}

void JPException_init(JPJavaFrame &frame)
{
	JP_TRACE_IN("JPException_init");
	jclass s_ThrowableClass = (jclass) frame.FindClass("java/lang/Throwable");
	s_Throwable_GetStackTraceID = frame.GetMethodID(s_ThrowableClass, "getStackTrace", "()[Ljava/lang/StackTraceElement;");

	jclass s_StackTraceElementClass = (jclass) frame.FindClass("java/lang/StackTraceElement");
	s_StackTraceElement_GetFileName = frame.GetMethodID(s_StackTraceElementClass, "getFileName", "()Ljava/lang/String;");
	s_StackTraceElement_GetMethodName = frame.GetMethodID(s_StackTraceElementClass, "getMethodName", "()Ljava/lang/String;");
	s_StackTraceElement_GetClassName = frame.GetMethodID(s_StackTraceElementClass, "getClassName", "()Ljava/lang/String;");
	s_StackTraceElement_GetLineNumber = frame.GetMethodID(s_StackTraceElementClass, "getLineNumber", "()I");
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

PyObject *PyTrace_FromJavaException(JPJavaFrame& frame, jthrowable th)
{
	PyTracebackObject *last_traceback = NULL;
	jvalue args;
	jarray obj = (jarray) frame.CallObjectMethodA(th, s_Throwable_GetStackTraceID, &args);
	jsize sz = frame.GetArrayLength(obj);
	PyObject *dict = PyModule_GetDict(PyJPModule);
	for (jsize i = 0; i < sz; ++i)
	{
		jobject elem = frame.GetObjectArrayElement((jobjectArray) obj, i);
		string filename, method;
		jstring jfilename = (jstring) frame.CallObjectMethodA(elem,
				s_StackTraceElement_GetFileName, &args);
		jstring jmethodname = (jstring) frame.CallObjectMethodA(elem,
				s_StackTraceElement_GetMethodName, &args);
		jstring jclassname = (jstring) frame.CallObjectMethodA(elem,
				s_StackTraceElement_GetClassName, &args);
		if (jfilename != NULL)
			filename = frame.toStringUTF8(jfilename);
		else
			filename = frame.toStringUTF8(jclassname) + ".java";
		if (jmethodname != NULL)
			method = frame.toStringUTF8(jclassname) + "." + frame.toStringUTF8(jmethodname);
		jint lineNum = frame.CallIntMethodA(elem, s_StackTraceElement_GetLineNumber, &args);

		last_traceback = tb_create(last_traceback, dict,  filename.c_str(),
				method.c_str(), lineNum);
	}
	if (last_traceback == NULL)
		Py_RETURN_NONE;
	return (PyObject*) last_traceback;
}
