/*****************************************************************************
   Copyright 2004 Steve Ménard

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
#ifndef _JP_EXCEPTION_H_
#define _JP_EXCEPTION_H_

/* All exception are passed as JPypeException.  The type of the exception
 * is specified at creation.  Exceptions may be of type
 * - _java_error - exception generated from within java.
 * - _python_error - excepction generated from within python.
 * - _runtime_error - Failure that will issue a runtime error in python and java.
 * - _type_error - Failure that will issue a type error in python.
 *
 * We must throw the correct exception so that it can properly be handled
 * when returning back to the native code.
 *
 * If we are returning to python, and it is a 
 * - _python_error, then we assume that a python exception has already been 
 *   placed in the python virtual machine.
 * - _java_error, then we will covert it to a python object with the correct
 *   object type.
 * - otherwise, then we will convert it to the requested python error.
 *
 * If we are returning to java, and it is a
 * - _java_error, they we assume there is already an Java exception queue
 *   in the virtual machine.
 * - otherwise convert to a RuntimeException.
 *
 */

#ifndef __FUNCTION_NAME__
#ifdef WIN32   //WINDOWS
#define __FUNCTION_NAME__   __FUNCTION__
#else          //*NIX
#define __FUNCTION_NAME__   __func__
#endif
#endif

/** This is the type of the exception to issue.
 *
 * FIXME consider adding Type to the name.
 */
enum JPError
{
	_java_error = 0,
	_python_error = 1,
	_runtime_error = 2,
	_type_error = 3,
	_value_error = 4,
	_overflow_error = 5,
	_index_error = 6,
	_attribute_error = 7
} ;

// Create a stackinfo for a particular location in the code that can then 
// be passed to the handler routine for auditing.
#define JP_STACKINFO() JPStackInfo(__FUNCTION_NAME__, __FILE__, __LINE__)

// Macros for raising an exception with jpype
//   These must be macros so that we can update the pattern and
//   maintain the appropraite auditing information.  C++ does not
//   have a lot for facitilies to make this easy.
#define JP_RAISE_PYTHON(msg)         { throw JPypeException(JPError::_python_error, msg, JP_STACKINFO()); }
#define JP_RAISE_RUNTIME_ERROR(msg)  { throw JPypeException(JPError::_runtime_error, msg, JP_STACKINFO()); }
#define JP_RAISE_TYPE_ERROR(msg)     { throw JPypeException(JPError::_type_error, msg, JP_STACKINFO()); }
#define JP_RAISE_VALUE_ERROR(msg)    { throw JPypeException(JPError::_value_error, msg, JP_STACKINFO()); }
#define JP_RAISE_OVERFLOW_ERROR(msg) { throw JPypeException(JPError::_overflow_error, msg, JP_STACKINFO()); }
#define JP_RAISE_INDEX_ERROR(msg)    { throw JPypeException(JPError::_index_error, msg, JP_STACKINFO()); }
#define JP_RAISE_ATTRIBUTE_ERROR(msg) { throw JPypeException(JPError::_attribute_error, msg, JP_STACKINFO()); }

// Macro to all after excuting a Python command that can result in
// a failure to convert it to an exception.
#define JP_PY_CHECK()               { if (JPPyErr::occurred()) JP_RAISE_PYTHON(__FUNCTION_NAME__); }

// Macro to use when hardening code
//   Most of these will be removed after core is debugged, but 
//   a few are necessary to handle off normal conditions.
#define ASSERT_NOT_NULL(X) {if (X==NULL) JP_RAISE_RUNTIME_ERROR( "Null Pointer Exception"); }

// Macro to add stack trace info when multiple paths lead to the same trouble spot
#define JP_CATCH catch (JPypeException& ex) { ex.from(JP_STACKINFO()); throw; }

/** Structure to pass around the location within a C++ source file.
 */
class JPStackInfo
{
	const char* function_;
	const char* file_;
	int line_;
public:

	JPStackInfo(const char* function, const char* file, int line)
	: function_(function), file_(file), line_(line)
	{
	}

	const char* getFunction() const
	{
		return function_;
	}

	const char* getFile() const
	{
		return file_;
	}

	int getLine() const
	{
		return line_;
	}
} ;
typedef list<JPStackInfo> JPStackTrace;

/**
 * Exception issued by JPype to indicate an internal problem.
 * 
 * This is primarily focused on transferring exception handling
 * to Python as the majority of errors are reported there.
 * 
 */
class JPypeException
{
public:
	JPypeException(jthrowable, const char* msn, const JPStackInfo& stackInfo);
	JPypeException(JPError errorType, const char* msn, const JPStackInfo& stackInfo);
	JPypeException(JPError errorType, const string& msn, const JPStackInfo& stackInfo);
	JPypeException(const JPypeException& ex);

	~JPypeException();

	void from(const JPStackInfo& info);

	string getMessage();
	string getPythonMessage();
	string getJavaMessage();

	void convertJavaToPython();
	void convertPythonToJava();

	/** Transfer handling of this exception to python. 
	 * 
	 * This should appear in the catch block whenever we return to python.
	 * 
	 */
	void toPython();

	/** Transfer handling of this exception to java. */
	void toJava();

	jthrowable getJavaException();

private:
	JPError m_Type;
	JPStackTrace m_Trace;
	string m_Message;
	JPThrowableRef m_Throwable;
} ;

/**
 * Exception issued with there was a Java exception issued after a java call.
 * 
 * This will just be held until it is converted to python or passed
 * back to java.
 */
class JPJavaException
{
public:

	JPJavaException(const char* msg, const JPStackInfo& stackInfo)
	: file(stackInfo.getFile()), line(stackInfo.getLine())
	{
		message = msg;
	}

	JPJavaException(const JPJavaException& ex)
	: file(ex.file), line(ex.line)
	{
		message = ex.message;
	}

	virtual ~JPJavaException()
	{
	}

	const char* file;
	int line;
	string message;
} ;

#endif
