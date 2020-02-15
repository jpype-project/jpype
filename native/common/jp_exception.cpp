/*****************************************************************************
   Copyright 2004-2008 Steve Ménard

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
#include "jpype.h"

JPypeException::JPypeException(jthrowable th, const char* msn, const JPStackInfo& stackInfo)
: m_Throwable(th)
{
	JPJavaFrame frame;
	JP_TRACE("JAVA EXCEPTION THROWN:", msn);
	m_Type = JPError::_java_error;
	m_Error.l = NULL;
	m_Message = msn;
	from(stackInfo);
}

JPypeException::JPypeException(int type, void* error, const char* msn, const JPStackInfo& stackInfo)
{
	JP_TRACE("EXCEPTION THROWN:", error, msn);
	m_Type = type;
	m_Error.l = error;
	if (msn == NULL)
		msn = "None";
	m_Message = msn;
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

JPypeException::JPypeException(int type,  const string& msn, int errType, const JPStackInfo& stackInfo)
{
	JP_TRACE("EXCEPTION THROWN", errType, msn);
	m_Type = type;
	m_Error.i = errType;
	m_Message = msn;
	from(stackInfo);
}

JPypeException::JPypeException(const JPypeException& ex)
: m_Trace(ex.m_Trace), m_Throwable(ex.m_Throwable)
{
	m_Type = ex.m_Type;
	m_Error = ex.m_Error;
	m_Message = ex.m_Message;
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
		for (JPStackTrace::iterator iter = this->m_Trace.begin();
				iter != m_Trace.end(); ++iter)
		{
			str << "\tat " << iter->getFunction() << "(" << iter->getFile() << ":" << iter->getLine() << ")" << endl;
		}

		JP_TRACE(str.str());
		return str.str();
	} catch (...)
	{
		return "error during get message";
	}
	JP_TRACE_OUT;
}

string JPypeException::getPythonMessage()
{
	// Must be bullet proof
	try
	{
		JPPyErrFrame eframe;
		if (!eframe.good)
			return "no error reported";
		JPPyString className(eframe.exceptionClass.getAttrString("__name__"));
		stringstream ss;
		ss << JPPyString::asStringUTF8(className.get());

		// Exception value
		if (!eframe.exceptionValue.isNull())
		{
			// Special handling here so that we don't fail on exceptions.
			string pyStrValue;
			PyObject *sval = PyObject_Str(eframe.exceptionValue.get());
			if (sval != NULL)
			{
				try
				{
					pyStrValue = JPPyString::asStringUTF8(sval);
					ss << ": " << pyStrValue;

				} catch (...)
				{
				}
				Py_DECREF(sval);
			}
		}

		return ss.str();
	} catch (...)
	{
		return "unknown error";
	}
}

string JPypeException::getJavaMessage()
{
	try
	{
		JPJavaFrame frame;
		return JPJni::toString((jobject) frame.ExceptionOccurred());
	} catch (...)
	{
		return "unknown error";
	}
}

bool isJavaThrowable(PyObject* exceptionClass)
{
	JPClass* cls = PyJPClass_getJPClass(exceptionClass);
	if (cls == NULL)
		return false;
	return cls->isThrowable();
}

jthrowable JPypeException::getJavaException()
{
	// Must be bullet proof
	try
	{
		JPPyErrFrame eframe;
		if (eframe.good && isJavaThrowable(eframe.exceptionClass.get()))
		{
			eframe.good = false;
			JPValue* javaExc = PyJPValue_getJavaSlot(eframe.exceptionClass.get());
			if (javaExc != NULL)
				return (jthrowable) javaExc->getJavaObject();
		}
		return NULL;
	} catch (...)
	{
		return NULL;
	}
}

void JPypeException::convertJavaToPython()
{
	// Welcome to paranoia land, where they really are out to get you!
	JP_TRACE_IN("JPypeException::convertJavaToPython");

	// Okay we can get to a frame to talk to the object
	JPJavaFrame frame;
	jthrowable th = m_Throwable.get();

	// Convert to Python object
	JP_TRACE("Convert to python");
	JPClass* cls = JPTypeManager::findClassForObject((jobject) th);
	if (cls == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, JPJni::toString(th).c_str());
		return;
	}

	// Create the exception object (this may fail)
	jvalue v;
	v.l = th;
	JPPyObject pyvalue = PyJPValue_create(JPValue(cls, v));
	if (pyvalue.isNull())
	{
		PyErr_SetString(PyExc_RuntimeError, JPJni::toString(th).c_str());
		return;
	}
	PyObject *type = (PyObject*) Py_TYPE(pyvalue.get());
	Py_INCREF(type);

	// Transfer to python
	PyErr_SetObject(type, pyvalue.get());
	JP_TRACE_OUT;
}

void JPypeException::convertPythonToJava()
{
	JP_TRACE_IN("JPypeException::convertPythonToJava");
	JPJavaFrame frame;
	jthrowable th;
	{
		JPPyErrFrame eframe;
		if (eframe.good && isJavaThrowable(eframe.exceptionClass.get()))
		{
			eframe.good = false;
			JPValue* javaExc = PyJPValue_getJavaSlot(eframe.exceptionValue.get());
			if (javaExc != NULL)
			{
				//th = (jthrowable) frame.NewLocalRef(javaExc->getJavaObject());
				th = (jthrowable) javaExc->getJavaObject();
				JP_TRACE("Throwing Java", JPJni::toString(th));
				frame.Throw(th);
				return;
			}
		}
	}

	// Otherwise
	string pyMessage = "Python exception thrown: " + getPythonMessage();
	JP_TRACE(pyMessage);
	PyErr_Clear();
	frame.ThrowNew(JPJni::s_RuntimeExceptionClass, pyMessage.c_str());
	JP_TRACE_OUT;
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
		}

		if (m_Type == JPError::_python_error)
		{
			JP_TRACE("Python exception");
			// Error is already in the stack
			return;
		}

		if (m_Type == JPError::_method_not_found)
		{
			JP_TRACE("Runtime error");
			PyErr_SetString(PyExc_RuntimeError, mesg.c_str());
		}

		if (m_Type == JPError::_os_error_unix)
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
			return;
		}

		if (m_Type == JPError::_os_error_windows)
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
			return;
		}

		if (m_Type == JPError::_python_exc)
		{
			// All others are Python errors
			JP_TRACE(Py_TYPE(m_Error.l)->tp_name);
			PyErr_SetString((PyObject*) m_Error.l, mesg.c_str());
			return;
		}
		JP_TRACE("Unknown error");
		PyErr_SetString(PyExc_RuntimeError, mesg.c_str());
		return;
	} catch (JPypeException& ex)
	{
		// Print our parting words
		JPTracer::trace("Fatal error in exception handling");
		JPTracer::trace("Handling:", mesg);
		JPTracer::trace("Type:", m_Error.l);
		if (ex.m_Type == JPError::_python_error)
			JPTracer::trace("Inner Python:", ex.getPythonMessage());
		else if (ex.m_Type == JPError::_java_error)
			JPTracer::trace("Inner Java:", ex.getJavaMessage());
		else
			JPTracer::trace("Inner:", ex.getMessage());

		JPStackInfo info = ex.m_Trace.front();
		JPTracer::trace(info.getFile(), info.getFunction(), info.getLine());

		// Heghlu'meH QaQ jajvam!
		PyErr_SetString(PyExc_RuntimeError, "Fatal error occurred");
		return;

		//		int *i = 0;
		//		*i = 0;
	} catch (...)
	{
		// urp?!
		JPTracer::trace("Fatal error in exception handling");

		// You shall not pass!
		int *i = 0;
		*i = 0;
	}
	JP_TRACE_OUT;
}

void JPypeException::toJava()
{
	JP_TRACE_IN("JPypeException::toJava");
	try
	{
		string mesg = getMessage();
		JP_TRACE(mesg);
		JPJavaFrame frame;
		if (m_Type == JPError::_python_error)
		{
			JP_TRACE("Python exception");
			convertPythonToJava();
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
			frame.ThrowNew(JPJni::s_NoSuchMethodErrorClass, mesg.c_str());
			return;
		}
		// All others are issued as RuntimeExceptions
		JP_TRACE("String exception");
		frame.ThrowNew(JPJni::s_RuntimeExceptionClass, mesg.c_str());
		return;
	} catch (JPypeException ex)
	{
		// Print our parting words.
		JPTracer::trace("Fatal error in exception handling");
		JPStackInfo info = ex.m_Trace.front();
		JPTracer::trace(info.getFile(), info.getFunction(), info.getLine());

		// Take one for the team.
		int *i = 0;
		*i = 0;
	} catch (...)
	{
		// urp?!
		JPTracer::trace("Fatal error in exception handling");

		// It is pointless, I can't go on.
		int *i = 0;
		*i = 0;
	}
	JP_TRACE_OUT;
}
