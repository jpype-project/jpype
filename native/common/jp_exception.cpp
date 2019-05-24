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
#include <Python.h>
#include <jpype.h>

#include "pyjp_value.h"

JPypeException::JPypeException(jthrowable th, const char* msn, const JPStackInfo& stackInfo) : m_Throwable(th)
{
	JPJavaFrame frame;
	JP_TRACE("JAVA EXCEPTION THROWN:", msn);
	m_Type = JPError::_java_error;
	m_Message = msn;
	from(stackInfo);
}

JPypeException::JPypeException(JPError::Type errType, const char* msn, const JPStackInfo& stackInfo)
{
	JP_TRACE("EXCEPTION THROWN:", errType, msn);
	m_Type = errType;
	m_Message = msn;
	from(stackInfo);
}

JPypeException::JPypeException(JPError::Type errType, const string& msn, const JPStackInfo& stackInfo)
{
	JP_TRACE("EXCEPTION THROWN", errType, msn);
	m_Type = errType;
	m_Message = msn;
	from(stackInfo);
}

JPypeException::JPypeException(JPError::Type errType, int err, const string& msn, const JPStackInfo& stackInfo)
{
	JP_TRACE("EXCEPTION THROWN", errType, msn);
	m_Type = errType;
	m_Message = msn;
	m_Error = err;
	from(stackInfo);
}

JPypeException::JPypeException(const JPypeException& ex)
: m_Trace(ex.m_Trace), m_Throwable(ex.m_Throwable)
{
	m_Type = ex.m_Type;
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
			// Convert the exception value to string
			string pyStrValue = eframe.exceptionValue.str();
			if (!pyStrValue.empty())
			{
				ss << ": " << pyStrValue;
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
	JPClass* cls = JPPythonEnv::getJavaClass(exceptionClass);
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
			JPValue* javaExc = JPPythonEnv::getJavaValue(eframe.exceptionClass.get());
			//                        JPValue* javaExc = JPPythonEnv::getJavaValue(
			//		  	    JPPythonEnv::getJavaException(
			//                             eframe.exceptionClass.get(),
			//                            eframe.exceptionValue.get()).get());
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
	JP_TRACE_IN("JPypeException::convertJavaToPython");
	JPJavaFrame frame;
	jthrowable th = m_Throwable.get();

	// Convert to Python object
	JPClass* cls = JPTypeManager::findClassForObject((jobject) th);
	if (cls == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, JPJni::toString(th).c_str());
		return;
	}

	// Start converting object by converting class
	JPPyObject pycls = JPPythonEnv::newJavaClass(cls);
	if (pycls.isNull())
	{
		PyErr_SetString(PyExc_RuntimeError, JPJni::toString(th).c_str());
		return;
	}

	// Okay now we just need to make the PyJPValue
	jvalue v;
	v.l = th;
	JPPyObject pyvalue = JPPythonEnv::newJavaObject(JPValue(cls, v));

	// Transfer to python
	PyErr_SetObject(pycls.get(), pyvalue.get());
	JP_TRACE_OUT;
}

void JPypeException::convertPythonToJava()
{
	JP_TRACE_IN("JPypeException::toPython");
	JPJavaFrame frame;
	jthrowable th;
	{
		JPPyErrFrame eframe;
		if (eframe.good && isJavaThrowable(eframe.exceptionClass.get()))
		{
			eframe.good = false;
			JPValue* javaExc = JPPythonEnv::getJavaValue(eframe.exceptionValue.get());
			//	    JPPythonEnv::getJavaException(
			//              eframe.exceptionClass.get(),
			//             eframe.exceptionValue.get()).get());
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

void JPypeException::toPython()
{
	string mesg;
	JP_TRACE_IN("JPypeException::toPython");
	try
	{
		mesg = getMessage();
		switch (m_Type)
		{
			case JPError::_java_error:
				JP_TRACE("Java exception");
				JPypeException::convertJavaToPython();
				return;

			case JPError::_python_error:
				JP_TRACE("Python exception");
				JP_TRACE(getPythonMessage());
				// Error is already in the stack
				return;

			case JPError::_runtime_error:
				JP_TRACE("Runtime error");
				PyErr_SetString(PyExc_RuntimeError, mesg.c_str());
				return;

			case JPError::_type_error:
				JP_TRACE("Type error");
				PyErr_SetString(PyExc_TypeError, mesg.c_str());
				return;

			case JPError::_value_error:
				JP_TRACE("Value error");
				PyErr_SetString(PyExc_ValueError, mesg.c_str());
				return;

			case JPError::_overflow_error:
				JP_TRACE("Overflow error");
				PyErr_SetString(PyExc_OverflowError, mesg.c_str());
				return;

			case JPError::_index_error:
				JP_TRACE("Index error");
				PyErr_SetString(PyExc_IndexError, mesg.c_str());
				return;

			case JPError::_attribute_error:
				JP_TRACE("Attribute error");
				PyErr_SetString(PyExc_AttributeError, mesg.c_str());
				return;

			case JPError::_os_error_unix:
			{
				PyObject* val = Py_BuildValue("(iz)", m_Error, (string("JVM DLL not found: ") + mesg).c_str());
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

			case JPError::_os_error_windows:
			{
				PyObject* val = Py_BuildValue("(izzi)", 2, (string("JVM DLL not found: ") + mesg).c_str(), NULL, m_Error);
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

			default:
				JP_TRACE("Unknown Error");
				PyErr_SetString(PyExc_RuntimeError, mesg.c_str());
				return;
		}
	} catch (JPypeException& ex)
	{
		// Print our parting words
		JPypeTracer::trace("Fatal error in exception handling");
		JPypeTracer::trace("Handling:", mesg);
		JPypeTracer::trace("Type:", m_Type);
		if (ex.m_Type == JPError::_python_error)
			JPypeTracer::trace("Inner Python:", ex.getPythonMessage());
		else if (ex.m_Type == JPError::_java_error)
			JPypeTracer::trace("Inner Java:", ex.getJavaMessage());
		else
			JPypeTracer::trace("Inner:", ex.getMessage());

		JPStackInfo info = ex.m_Trace.front();
		JPypeTracer::trace(info.getFile(), info.getFunction(), info.getLine());

		// Heghlu'meH QaQ jajvam!
		PyErr_SetString(PyExc_RuntimeError, "Fatal error occurred");
		return;

		//		int *i = 0;
		//		*i = 0;
	} catch (...)
	{
		// urp?!
		JPypeTracer::trace("Fatal error in exception handling");

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
		switch (m_Type)
		{
			case JPError::_python_error:
				//Python errors need to be converted
				JP_TRACE("Python exception");
				convertPythonToJava();
				return;

			case JPError::_java_error:
				// Java errors are already registered
				JP_TRACE("Java exception");
				JP_TRACE(JPJni::toString((jobject) frame.ExceptionOccurred()));
				if (m_Throwable.get() != 0)
				{
					frame.Throw(m_Throwable.get());
					return;
				}

			default:
				// All others are issued as RuntimeExceptions
				JP_TRACE("JPype Error");
				break;
		}

		JP_TRACE("String exception");
		frame.ThrowNew(JPJni::s_RuntimeExceptionClass, mesg.c_str());
	} catch (JPypeException ex)
	{
		// Print our parting words.
		JPypeTracer::trace("Fatal error in exception handling");
		JPStackInfo info = ex.m_Trace.front();
		JPypeTracer::trace(info.getFile(), info.getFunction(), info.getLine());

		// Take one for the team.
		int *i = 0;
		*i = 0;
	} catch (...)
	{
		// urp?!
		JPypeTracer::trace("Fatal error in exception handling");

		// It is pointless, I can't go on.
		int *i = 0;
		*i = 0;
	}
	JP_TRACE_OUT;
}
