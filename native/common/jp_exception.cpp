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

static_assert(std::is_nothrow_copy_constructible<JPypeException>::value,
              "S must be nothrow copy constructible");

PyObject* PyTrace_FromJPStackTrace(JPStackTrace& trace);

JPypeException::JPypeException(JPJavaFrame &frame, jthrowable th, const JPStackInfo& stackInfo)
: std::runtime_error(frame.toString(th)),
  m_Type(JPError::_java_error),
  m_Throwable(frame, th)
{
	JP_TRACE("JAVA EXCEPTION THROWN with java throwable");
	m_Error.l = nullptr;
	from(stackInfo);
}

JPypeException::JPypeException(int type, void* error, const JPStackInfo& stackInfo)
: std::runtime_error("None"), m_Type(type)
{
	JP_TRACE("EXCEPTION THROWN with error", error);
	m_Error.l = error;
	from(stackInfo);
}

JPypeException::JPypeException(int type, void* errType, const string& msn, const JPStackInfo& stackInfo)
: std::runtime_error(msn), m_Type(type)
{
	JP_TRACE("EXCEPTION THROWN", errType, msn);
	m_Error.l = errType;
	//m_Message = msn;
	from(stackInfo);
}

// GCOVR_EXCL_START
// This is only used during startup for OSError

JPypeException::JPypeException(int type,  const string& msn, int errType, const JPStackInfo& stackInfo)
: std::runtime_error(msn), m_Type(type)
{
	JP_TRACE("EXCEPTION THROWN", errType, msn);
	m_Error.i = errType;
	from(stackInfo);
}

JPypeException::JPypeException(const JPypeException &ex) noexcept
        : runtime_error(ex.what()), m_Type(ex.m_Type),  m_Error(ex.m_Error),
        m_Trace(ex.m_Trace), m_Throwable(ex.m_Throwable)
{
}

JPypeException& JPypeException::operator = (const JPypeException& ex)
{
	if(this == &ex)
	{
		return *this;
	}
	m_Type = ex.m_Type;
	m_Trace = ex.m_Trace;
	m_Throwable = ex.m_Throwable;
	m_Error = ex.m_Error;
	return *this;
}
// GCOVR_EXCL_STOP

void JPypeException::from(const JPStackInfo& info)
{
	JP_TRACE("EXCEPTION FROM: ", info.getFile(), info.getLine());
	m_Trace.push_back(info);
}

bool isJavaThrowable(PyObject* exceptionClass)
{
	JPClass* cls = PyJPClass_getJPClass(exceptionClass);
	if (cls == nullptr)
		return false;
	return cls->isThrowable();
}

void JPypeException::convertJavaToPython()
{
	// Welcome to paranoia land, where they really are out to get you!
	JP_TRACE_IN("JPypeException::convertJavaToPython");
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
	jthrowable th = m_Throwable.get();
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
	}	catch (JPypeException& ex)
	{
		(void) ex;
		JP_TRACE("FAILURE IN CAUSE");
		// Any failures in this optional action should be ignored.
		// worst case we don't print as much diagnostics.
	}

	// Transfer to Python
	PyErr_SetObject(type, pyvalue.get());
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

void JPypeException::convertPythonToJava()
{
	JP_TRACE_IN("JPypeException::convertPythonToJava");
	JPJavaFrame frame = JPJavaFrame::outer();
	JPContext *context = frame.getContext();
	jthrowable th;
	JPPyErrFrame eframe;
	if (eframe.good && isJavaThrowable(eframe.m_ExceptionClass.get()))
	{
		eframe.good = false;
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
		frame.ThrowNew(frame.FindClass("java/lang/RuntimeException"), std::runtime_error::what());
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

void JPypeException::toPython()
{
	const char* mesg = nullptr;
	JP_TRACE_IN("JPypeException::toPython");
	JP_TRACE("err", PyErr_Occurred());
	try
	{
		// Check the signals before processing the exception
		// It may be a signal when interrupted Java in which case
		// the signal takes precedence.
		if (PyErr_CheckSignals()!=0)
			return;

		mesg = std::runtime_error::what();
		JP_TRACE(m_Error.l);
		JP_TRACE(mesg);

		// We already have a Python error on the stack.
		if (PyErr_Occurred())
			return;

		if (m_Type == JPError::_java_error)
		{
			JP_TRACE("Java exception");
			JPypeException::convertJavaToPython();
			return;
		} else if (m_Type == JPError::_python_error)
		{
			// Already on the stack
		}// This section is only reachable during startup of the JVM.
			// GCOVR_EXCL_START
		else if (m_Type == JPError::_os_error_unix)
		{
			std::stringstream ss;
			ss << "JVM DLL not found: " << mesg;
			PyObject* val = Py_BuildValue("(iz)", m_Error.i,
					ss.str().c_str());
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
		} else if (m_Type == JPError::_os_error_windows)
		{
			std::stringstream ss;
			ss << "JVM DLL not found: " << mesg;
			PyObject* val = Py_BuildValue("(izzi)", 2,
					ss.str().c_str(), NULL, m_Error.i);
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
		}// GCOVR_EXCL_STOP

		else if (m_Type == JPError::_python_exc)
		{
			// All others are Python errors
			JP_TRACE(Py_TYPE(m_Error.l)->tp_name);
			PyErr_SetString((PyObject*) m_Error.l, mesg);
		} else
		{
			// This should not be possible unless we failed to cover one of the
			// exception type codes.
			JP_TRACE("Unknown error");
			PyErr_SetString(PyExc_RuntimeError, mesg); // GCOVR_EXCL_LINE
		}

		// Attach our info as the cause
		if (_jp_cpp_exceptions)
		{
			JPPyErrFrame eframe;
			eframe.normalize();
			JPPyObject args = JPPyObject::call(Py_BuildValue("(s)", "C++ Exception"));
			JPPyObject trace = JPPyObject::call(PyTrace_FromJPStackTrace(m_Trace));
			JPPyObject cause = JPPyObject::accept(PyObject_Call(PyExc_Exception, args.get(), nullptr));
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
			return;  // Let these go to Python, so we can see the error
		} else if (ex.m_Type == JPError::_java_error)
			JPTracer::trace("Inner Java:", ex.what());
		else
			JPTracer::trace("Inner:", ex.what());

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
		int *i = nullptr;
		*i = 0;
	}
	// GCOVR_EXCL_STOP
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

void JPypeException::toJava()
{
	JP_TRACE_IN("JPypeException::toJava");
	JPContext* context = JPContext_global;
	try
	{
		const char* mesg = what();
		JPJavaFrame frame = JPJavaFrame::external(context->getEnv());
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

		if (m_Type == JPError::_python_error)
		{
			JPPyCallAcquire callback;
			JP_TRACE("Python exception");
			convertPythonToJava();
			return;
		}

		if (m_Type == JPError::_python_exc)
		{
			JPPyCallAcquire callback;
			// All others are Python errors
			JP_TRACE(Py_TYPE(m_Error.l)->tp_name);
			PyErr_SetString((PyObject*) m_Error.l, mesg);
			convertPythonToJava();
			return;
		}

		// All others are issued as RuntimeExceptions
		JP_TRACE("String exception");
		frame.ThrowNew(context->m_RuntimeException.get(), mesg);
		return;
	}	catch (JPypeException& ex)  // GCOVR_EXCL_LINE
	{	// GCOVR_EXCL_START
		// Print our parting words.
		JPTracer::trace("Fatal error in exception handling");
		JPStackInfo info = ex.m_Trace.front();
		JPTracer::trace(info.getFile(), info.getFunction(), info.getLine());

		// Take one for the team.
		int *i = nullptr;
		*i = 0;
		// GCOVR_EXCL_STOP
	} catch (...) // GCOVR_EXCL_LINE
	{
		// GCOVR_EXCL_START
		// urp?!
		JPTracer::trace("Fatal error in exception handling");

		// It is pointless, I can't go on.
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
	return (PyObject*) last_traceback;
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
