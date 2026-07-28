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
#ifndef _JP_EXCEPTION_H_
#define _JP_EXCEPTION_H_

/* All exception are passed as JPypeException.  The type of the exception
 * is specified at creation.  Exceptions may be of type
 * - _java_error - exception generated from within java.
 * - _python_error - exception generated from within python.
 * - _runtime_error - Failure that will issue a runtime error in python and java.
 * - _type_error - Failure that will issue a type error in python.
 *
 * We must throw the correct exception so that it can properly be handled
 * when returning to the native code.
 *
 * If we are returning to python, and it is a
 * - _python_error, then the exception was fetched off the thread state and
 *   normalized at the moment of the throw (not left live on the thread state
 *   for the duration of the C++ unwind), and toPython()/toJava() restore it.
 * - _java_error, then we will convert it to a python object with the correct
 *   object type.
 * - otherwise, then we will convert it to the requested python error.
 *
 * If we are returning to java, and it is a
 * - _java_error, then we assume there is already a Java exception queue
 *   in the virtual machine.
 * - otherwise convert to a RuntimeException.
 *
 */
#include <stdexcept>
#ifndef __FUNCTION_NAME__
#ifdef WIN32   //WINDOWS
#define __FUNCTION_NAME__   __FUNCTION__
#else          //*NIX
#define __FUNCTION_NAME__   __func__
#endif
#endif

/**
 * This is the type of the exception to issue.
 */
enum JPError
{
_java_error,
_python_error,
_python_exc,
_os_error_unix,
_os_error_windows,
};

// Create a stackinfo for a particular location in the code that can then
// be passed to the handler routine for auditing.
#define JP_STACKINFO() JPStackInfo(__FUNCTION_NAME__, __FILE__, __LINE__)



// Macro to use when hardening code
//   Most of these will be removed after core is debugged, but
//   a few are necessary to handle off normal conditions.
#define ASSERT_NOT_NULL(X) {if ((X)==NULL) { JP_RAISE(PyExc_RuntimeError,  "Null Pointer Exception");} }

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
using JPStackTrace = vector<JPStackInfo>;

typedef union
{
    int  i;
    void*  l;
} JPErrorUnion;

/**
 * Exception issued by JPype to indicate an internal problem.
 *
 * This is primarily focused on transferring exception handling
 * to Python as the majority of errors are reported there.
 *
 */
class JPypeException : std::runtime_error
{
public:
	JPypeException(JPJavaFrame &frame, jthrowable, const JPStackInfo& stackInfo);
	JPypeException(int type, void* error, const JPStackInfo& stackInfo);
	JPypeException(int type, void* error, const string& msn, const JPStackInfo& stackInfo);
	JPypeException(int type, const string& msn, int error, const JPStackInfo& stackInfo);
    // The copy constructor for an object thrown as an exception must be declared noexcept, including any implicitly-defined copy constructors.
    // Any function declared noexcept that terminates by throwing an exception violates ERR55-CPP. Honor exception specifications.
    JPypeException(const JPypeException &ex) noexcept;
	JPypeException& operator = (const JPypeException& ex);
	~JPypeException() override = default;

	void from(const JPStackInfo& info);

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

	/** Put a captured _python_error exception back on the thread state.
	 *
	 * For callers that need the original Python exception live (e.g. to
	 * chain it as a cause) before they finish handling this JPypeException
	 * themselves, rather than going through toPython()/toJava(). No-op for
	 * any other exception type, or if there is nothing captured.
	 */
	void restorePythonError();

	int getExceptionType() const
	{
		return m_Type;
	}

	jthrowable getThrowable()
	{
		return m_Throwable.get();
	}

private:
	int m_Type;
	JPErrorUnion m_Error{};
	JPStackTrace m_Trace;
	JPThrowableRef m_Throwable;
	std::string m_Message;

	// For _python_error only: the exception is fetched and normalized at the
	// moment of the throw (see JP_RAISE_PYTHON), not left live on the thread
	// state for the length of the C++ unwind. A pending exception left on the
	// thread state is visible to, and can be disturbed by, an incidental GC
	// pass triggered anywhere during that unwind - fetching claims sole
	// ownership so nothing else can touch it before toPython()/toJava()
	// restore it. Only the (already-normalized) instance is held - its class
	// and traceback are recoverable from the instance itself at restore time
	// (see JPPyErr::restore(JPPyObject&)), so there's no need to separately
	// hold references to them too.
	JPPyObject m_PyExcValue;
};

#endif
