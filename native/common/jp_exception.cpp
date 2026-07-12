// --- file: common/jp_exception.cpp ---
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
#include "jp_proxy.h"
#include "pyjp.h"

static_assert(std::is_nothrow_copy_constructible<JPypeException>::value,
			  "S must be nothrow copy constructible");

#if PY_VERSION_HEX < 0x030C0000
// --- Emulation Layer for Python 3.6 - 3.11 ---
inline PyObject* PyErr_GetRaisedException()
{
	PyObject *type = nullptr, *value = nullptr, *traceback = nullptr;
	PyErr_Fetch(&type, &value, &traceback);
	if (type == nullptr)
		return nullptr;
	PyErr_NormalizeException(&type, &value, &traceback);
	if (traceback != nullptr)
	{
		PyException_SetTraceback(value, traceback);
		Py_DECREF(traceback);
	}
	Py_DECREF(type);
	return value;
}

inline void PyErr_SetRaisedException(PyObject* exc)
{
	if (exc == nullptr)
	{
		PyErr_Clear();
		return;
	}
	PyObject* traceback = PyException_GetTraceback(exc); // Returns a new reference
	Py_INCREF(Py_TYPE(exc));
	PyErr_Restore((PyObject*)Py_TYPE(exc), exc, traceback);
}
#endif

PyObject* PyTrace_FromJPStackTrace(JPStackTrace& trace);

JPypeException::JPypeException(JPJavaFrame &frame, jthrowable th, const JPStackInfo& stackInfo)
: std::runtime_error(frame.toString(th)),
  m_Type(JPError::_java_error)
{
	m_Context = frame.getContext();
	m_Throwable = (jthrowable) frame.NewGlobalRef(th);
	JP_TRACE("JAVA EXCEPTION THROWN with java throwable");
	m_Error.l = nullptr;
	from(stackInfo);
}

JPypeException::JPypeException(int type, void* error, const JPStackInfo& stackInfo)
: std::runtime_error("None"), m_Type(type)
{
	JP_TRACE("EXCEPTION THROWN with error", error);
	m_Error.l = error;
	m_Context = nullptr;
	from(stackInfo);
}

JPypeException::JPypeException(int type, void* errType, const string& msn, const JPStackInfo& stackInfo)
: std::runtime_error(msn), m_Type(type)
{
	JP_TRACE("EXCEPTION THROWN", errType, msn);
	m_Error.l = errType;
	m_Context = nullptr;
	//m_Message = msn;
	from(stackInfo);
}

// GCOVR_EXCL_START
// This is only used during startup for OSError

JPypeException::JPypeException(int type,  const string& msn, int errType, const JPStackInfo& stackInfo)
: std::runtime_error(msn), m_Type(type)
{
	JP_TRACE("EXCEPTION THROWN", errType, msn);
	m_Context = nullptr;
	m_Error.i = errType;
	from(stackInfo);
}

JPypeException::JPypeException(const JPypeException &ex) noexcept
		: runtime_error(ex.what()), m_Type(ex.m_Type),  m_Error(ex.m_Error),
		m_Trace(ex.m_Trace), m_Throwable(ex.m_Throwable), m_Context(ex.m_Context)
{
	if (m_Context!=nullptr && m_Throwable != nullptr && m_Context->isRunning())
	{
		JPJavaFrame frame = JPJavaFrame::outer(m_Context);
		m_Throwable = (jthrowable) frame.NewGlobalRef(m_Throwable);
	}
}

JPypeException::~JPypeException()
{
	if (m_Context!=nullptr && m_Throwable != nullptr && m_Context->isRunning())
		m_Context->getEnv()->DeleteGlobalRef(m_Throwable);
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
	JPContext* context = m_Context;
	if (context == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "Unable to convert java error, context is null.");
		return;
	}
	JPJavaFrame frame = JPJavaFrame::outer(context);
	// GCOVR_EXCL_STOP

	jthrowable th = m_Throwable;
	jvalue v;
	v.l = th;

	if (context->m_ProxyType_UnwrapPythonExceptionID != nullptr)
	{
		jlong py_instance_ptr = frame.CallStaticLongMethodA(
			(jclass) context->m_ProxyTypeClass, 
			context->m_ProxyType_UnwrapPythonExceptionID, 
			&v
		);  // borrowed reference, lifespan held by th


		if (py_instance_ptr != 0)
		{
			JPProxy* jp_proxy = (JPProxy*) py_instance_ptr;
			PyJPProxy* py_proxy = jp_proxy->m_Instance;
			PyObject* exc = py_proxy->m_Target;
			// Restore the original exception into Python's error registers
			PyErr_SetObject((PyObject*) Py_TYPE(exc), exc);
			JP_TRACE("Successfully unwrapped and restored Python exception from Java proxy");
			return;
		}
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

	// We died in the boot sequence and don't have required resources
	if (!context->getTypeManager()->isReady())
	{
		PyErr_SetString(PyExc_RuntimeError, frame.toString(th).c_str());
		return;
	}
	
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

	// Create the exception object (this may fail, e.g. the smuggler guard in
	// JPClass::convertToPythonObject rejecting a Java exception object that
	// wraps a Python proxy created by a different interpreter). We are
	// already inside exception handling here, so a throw escaping this
	// point unwinds through the caller's exception-conversion machinery too -
	// asbestos required. Catch locally and fall back to the same safe,
	// contextual RuntimeError the (now largely unreachable) null-check below
	// was meant to provide, rather than letting it fall through to the
	// generic "Fatal error occurred" at the outer safety net.
	v.l = th;
	JPPyObject pyvalue;
	try
	{
		pyvalue = cls->convertToPythonObject(frame, v, false);
	} catch (JPypeException& ex)
	{
		(void) ex;
		PyErr_SetString(PyExc_RuntimeError, frame.toString(th).c_str());
		return;
	}

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

static void fail(JPJavaFrame& frame, const char *msg)
{
	JPContext *context = frame.getContext();
	frame.ThrowNew((jclass) context->m_RuntimeException, msg);
}

void JPypeException::convertPythonToJava(JPJavaFrame& frame)
{
	JP_TRACE_IN("JPypeException::convertPythonToJava");
	JPContext *context = frame.getContext();
	jthrowable th;

	JPPyObject exc = JPPyObject::claim(PyErr_GetRaisedException()); // Clear the exception state
	if (isJavaThrowable((PyObject*) Py_TYPE(exc.get())))
	{
		JPValue* javaExc = PyJPValue_getJavaSlot(exc.get());
		if (javaExc != nullptr)
		{
			th = (jthrowable) javaExc->getJavaObject(frame);
			JP_TRACE("Throwing Java", frame.toString(th));
			frame.Throw(th);
			return;
		}
	}

	if (!context->isRunning())
	{
		JPPyObject v_repr = JPPyObject::claim(PyObject_Repr(exc.get()));
		const char* msg = v_repr.isValid() ? PyUnicode_AsUTF8(v_repr.get()) : "Undetermined error";
		fail(frame, msg);
		return;
	}

#if 0
	JPPyObject c_repr = JPPyObject::claim(PyObject_Repr((PyObject*) Py_TYPE(exc.get())));
	JPPyObject v_repr = JPPyObject::claim(PyObject_Repr(exc.get()));
	printf("DEBUG BOOTSTRAP:\n");
	printf("  Class: %s\n", c_repr.isValid() ? PyUnicode_AsUTF8(c_repr.get()) : "NULL");
	printf("  Value: %s\n", v_repr.isValid() ? PyUnicode_AsUTF8(v_repr.get()) : "NULL");
#endif

	// Locate and invoke our Python module's conversion engine: _jpype._pyexc_convert
	// We fetch the module function directly via the shared JPContext
	PyObject* pyexc_convert_fn = context->m_PyExcConvert; 
	if (pyexc_convert_fn == nullptr)
	{
		JPPyObject v_repr = JPPyObject::claim(PyObject_Repr(exc.get()));
		const char* msg = v_repr.isValid() ? PyUnicode_AsUTF8(v_repr.get()) : "Undetermined error";
		fail(frame, msg);
		return;
	}

	// Execute the Python conversion method: _pyexc_convert(exc)
	// This returns our normalized _jpype._JProxy instance wrapping the Java Throwable
	JPPyObject proxy_res = JPPyObject::claim(PyObject_CallFunctionObjArgs(pyexc_convert_fn, exc.get(), nullptr));
	if (proxy_res.isNull())
	{
		// If Python code raises an unhandled exception during conversion, it's trapped here
		JPPyObject v_repr = JPPyObject::claim(PyObject_Repr(exc.get()));
		const char* msg = v_repr.isValid() ? PyUnicode_AsUTF8(v_repr.get()) : "Undetermined error";
		fail(frame, msg);
		return;
	}

	// Peel back the Python wrapper layers to find the core JPValue slot
	JPValue* javaExc = PyJPValue_getJavaSlot(proxy_res.get());
	if (javaExc == nullptr)
	{
		fail(frame, "JPype Engine Error: Proxy carried no Java slot");
		return;
	}

	// Extract the raw JNI handle out of the slot
	th = (jthrowable) javaExc->getJavaObject(frame);
	if (th == nullptr)
	{
		fail(frame, "JPype Engine Error: Underlying JNI handle is null");
		return;
	}

	// 5. Register the reference cleanup path
	// Ties the lifespan of the underlying Python exception instance to the life of the Java Throwable proxy
	frame.registerRef((jobject) th, exc.keep());

	// 6. Push the finalized exception across the JNI bridge into the JVM execution context
	JP_TRACE("Throwing Java", frame.toString(th));
	frame.Throw(th);
	JP_TRACE_OUT;
}

string JPypeException::toString()
{
	const char* mesg = what();
	if (m_Type == JPError::_java_error)
	{
		if (m_Throwable != 0)
		{
			return string(mesg) + ": java throwable";
		}
		return mesg;
	}

	if (m_Type == JPError::_python_error)
	{
		return string(mesg) + ": python exception";
	}

	return mesg;
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
			JPPyObject val = JPPyObject::call(Py_BuildValue("(iz)", m_Error.i, ss.str().c_str()));
			JPPyObject exc = JPPyObject::call(PyObject_Call(PyExc_OSError, val.get(), nullptr));
			PyErr_SetObject(PyExc_OSError, exc.get());
		} else if (m_Type == JPError::_os_error_windows)
		{
			std::stringstream ss;
			ss << "JVM DLL not found: " << mesg;
			JPPyObject val = JPPyObject::call(Py_BuildValue("(izzi)", 2, ss.str().c_str(), NULL, m_Error.i));
			JPPyObject exc = JPPyObject::call(PyObject_Call(PyExc_OSError, val.get(), nullptr));
			PyErr_SetObject(PyExc_OSError, exc.get());
		}// GCOVR_EXCL_STOP

		else if (m_Type == JPError::_python_exc)
		{
			// All others are Python errors
			PyObject* errType = PyExc_RuntimeError;	
			if (m_Error.l != nullptr)
				errType = (PyObject*) m_Error.l;
			JP_TRACE(Py_TYPE(errType)->tp_name);
			PyErr_SetString((PyObject*) errType, mesg);
		} else
		{
			// This should not be possible unless we failed to cover one of the
			// exception type codes.
			JP_TRACE("Unknown error");
			PyErr_SetString(PyExc_RuntimeError, mesg); // GCOVR_EXCL_LINE
		}


#if 0
// Not sure how to get state here
		// Attach our info as the cause
		if (_jp_cpp_exceptions)
		{
			JPPyObject activeException = JPPyObject::claim(PyErr_GetRaisedException());
			if (activeException.isValid())
			{
				JPPyObject args = JPPyObject::call(Py_BuildValue("(s)", "C++ Exception"));
				JPPyObject trace = JPPyObject::call(PyTrace_FromJPStackTrace(m_Trace));
				JPPyObject cause = JPPyObject::call(PyObject_Call(PyExc_Exception, args.get(), nullptr));
				
				PyException_SetTraceback(cause.get(), trace.get());
				PyException_SetCause(activeException.get(), cause.keep());
				PyErr_SetRaisedException(activeException.keepNull());
			}
		}
#endif
	}// GCOVR_EXCL_START
	catch (JPypeException& ex)
	{
		// Print our parting words
		JPTracer::trace("Fatal error in exception handling");
		JPTracer::trace("Handling:", mesg);
		JPTracer::trace("Type:", m_Error.l);

#if 1
		printf("Fatal error in exception handling\n");
		printf("Handling: %s\n", mesg);
#endif

		if (ex.m_Type == JPError::_python_error)
		{
			// 1. Snatch the exception instance cleanly from the stack (Python 3.12 style)
			JPPyObject activeException = JPPyObject::claim(PyErr_GetRaisedException());
			if (activeException.isValid())
			{
				// 2. Safe type tracking: Get the type name directly from the instance object layout
				const char* typeName = Py_TYPE(activeException.get())->tp_name;
				JPTracer::trace("Inner Python:", typeName);
				
				// 3. Put it right back onto the Python error stack undisturbed
				PyErr_SetRaisedException(activeException.keepNull());
			}
			return;  // Let these go to Python, so we can see the error
		} 
		else if (ex.m_Type == JPError::_java_error)
			JPTracer::trace("Inner Java:", ex.what());
		else
			JPTracer::trace("Inner:", ex.what());

		JPStackInfo info = ex.m_Trace.front();
		JPTracer::trace(info.getFile(), info.getFunction(), info.getLine());

		// Heghlu'meH QaQ jajvam! ("Today is a good day to die!")
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

void JPypeException::toJava(JPJavaFrame& frame)
{
	JP_TRACE_IN("JPypeException::toJava");
	JPContext* context = frame.getContext();
	try
	{
		const char* mesg = what();
		if (m_Type == JPError::_java_error)
		{
			JP_TRACE("Java exception");
			//JP_TRACE(context->toString((jobject) frame.ExceptionOccurred()));
			if (m_Throwable != 0)
			{
				JP_TRACE("Java rethrow");
				frame.Throw(m_Throwable);
				return;
			}
			return;
		}

		if (m_Type == JPError::_python_error)
		{
			JP_TRACE("Python exception");
			convertPythonToJava(frame);
			return;
		}

		if (m_Type == JPError::_python_exc)
		{
			// All others are Python errors
			JP_TRACE(Py_TYPE(m_Error.l)->tp_name);
			PyErr_SetString((PyObject*) m_Error.l, mesg);
			convertPythonToJava(frame);
			return;
		}

		// All others are issued as RuntimeExceptions
		JP_TRACE("String exception");
		frame.ThrowNew(context->m_RuntimeException, mesg);
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
	
	// Grab the global builtins dictionary. It is always available on the current 
	// thread/interpreter state, requires no module pointers, and is 100% reentrant-safe.
	PyObject *dict = PyEval_GetBuiltins(); 
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
	if (context->m_Support_GetStackFrameID == nullptr)
		return {};

	JNIEnv* env = frame.getEnv();
	jobjectArray obj = static_cast<jobjectArray>(env->CallStaticObjectMethodA(context->m_SupportClass,
			context->m_Support_GetStackFrameID, args));

	// Eat any exceptions that were generated
	if (env->ExceptionCheck() == JNI_TRUE)
		env->ExceptionClear();

	if (obj == nullptr)
		return {};
	jsize sz = frame.GetArrayLength(obj);

	PyObject *dict = context->modulestate->module_dict;
	if (dict == nullptr)
		dict = PyEval_GetBuiltins(); 

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
