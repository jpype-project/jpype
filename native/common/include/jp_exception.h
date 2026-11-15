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

/* Stack-trace bookkeeping infrastructure shared by the JPBaseError hierarchy
 * (see jp_error.h). This file used to also define JPypeException, the single
 * mono-class every exception crossing the Java/Python/C++ boundary used to
 * be carried as - that class has been fully replaced by
 * JPJavaError/JPPythonError/JPInternalError and is gone; only the pieces
 * they still depend on remain here.
 */
#include <stdexcept>
#ifndef __FUNCTION_NAME__
#ifdef WIN32   //WINDOWS
#define __FUNCTION_NAME__   __FUNCTION__
#else		  //*NIX
#define __FUNCTION_NAME__   __func__
#endif
#endif

// Create a stackinfo for a particular location in the code that can then
// be passed to the handler routine for auditing.
#define JP_STACKINFO() JPStackInfo(__FUNCTION_NAME__, __FILE__, __LINE__)



// Macro to use when hardening code
//   Most of these will be removed after core is debugged, but
//   a few are necessary to handle off normal conditions.

// The helper macros required to force the preprocessor to stringify the expansion, 
// rather than stringifying the tokens "__LINE__" or "__FILE__" directly.
#define STRINGIFY_HELPER(x) #x
#define TO_STRING(x) STRINGIFY_HELPER(x)

// Now you can use standard string compile-time merging!
#define ASSERT_NOT_NULL(X, Location) \
	do { \
		if ((X) == nullptr) { \
			PyErr_SetString(PyExc_RuntimeError, \
				"Null Pointer Exception at " Location " (" #X " is null) [" __FILE__ ":" TO_STRING(__LINE__) "]"); \
			JP_RAISE_PYTHON(); \
		} \
	} while (0)

// Macro to add stack trace info when multiple paths lead to the same trouble spot
#define JP_CATCH catch (JPBaseError& ex) { ex.from(JP_STACKINFO()); throw; }

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

#endif
