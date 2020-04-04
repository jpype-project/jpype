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
#ifndef _JPYPE_H_
#define _JPYPE_H_

#ifdef __GNUC__
// Python requires char* but C++ string constants are const char*
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#ifdef WIN32

#ifndef __GNUC__ // Then this must mean a variant of GCC on win32 ...
#pragma warning (disable:4786)
#endif

#if defined(__CYGWIN__)
// jni_md.h does not work for cygwin.  Use this instead.
#elif defined(__GNUC__)
// JNICALL causes problem for function prototypes .. since I am not defining any JNI methods there is no need for it
#undef JNICALL
#define JNICALL
#endif

#endif

#include <jni.h>

// Define this and use to allow destructors to throw in C++11 or later
#if defined(_MSC_VER)

// Visual Studio C++ does not seem have changed __cplusplus since 1997
#if (_MSVC_LAND >= 201402)
#define NO_EXCEPT_FALSE noexcept(false)
#else
#define NO_EXCEPT_FALSE throw(JPypeException)
#endif

#else

// For all the compilers than understand standards
#if (__cplusplus >= 201103L)
#define NO_EXCEPT_FALSE noexcept(false)
#else
#define NO_EXCEPT_FALSE throw(JPypeException)
#endif

#endif

#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <list>

using std::map;
using std::string;
using std::stringstream;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::list;

#ifdef JP_INSTRUMENTATION

template <size_t i>
constexpr uint32_t _hash(const char *q, uint32_t v)
{
	return _hash < i - 1 > (q + 1, v * 0x1a481023 + q[0]);
}

template <>
constexpr uint32_t _hash<0>(const char *q, uint32_t v)
{
	return v;
}
#define compile_hash(x) _hash<sizeof(x)-1>(x, 0)

extern void PyJPModuleFault_throw(uint32_t code);
extern int PyJPModuleFault_check(uint32_t code);
#define JP_TRACE_IN(X, ...) try { PyJPModuleFault_throw(compile_hash(X));
#define JP_FAULT_RETURN(X, Y)  if (PyJPModuleFault_check(compile_hash(X))) return Y
#else
#define JP_FAULT_RETURN(X, Y)  if (false)
#endif

/** Definition of commonly used template types */
typedef vector<string> StringVector;

/**
 * Converter are used for bulk byte transfers from Python to Java.
 */
typedef jvalue (*jconverter)(void*) ;

/**
 * Create a converter for a bulk byte transfer.
 *
 * Bulk transfers do not check for range and may be lossy.  These are only
 * triggered when a transfer either using memoryview or a slice operator
 * assignment from a buffer object (such as numpy.array).  Converters are
 * created once at the start of the transfer and used to convert each
 * byte by casting the memory and then assigning to the jvalue union with
 * the requested type.
 *
 * Byte order transfers are not supported by the Python buffer API and thus
 * have not been implemented.
 *
 * @param from is a Python struct designation
 * @param itemsize is the size of the Python item
 * @param to is the desired Java primitive type
 * @return a converter function to convert each member.
 */
extern jconverter getConverter(const char* from, int itemsize, const char* to);

extern bool _jp_cpp_exceptions;

// Types
class JPClass;
class JPValue;
class JPProxy;
class JPArray;
class JPArrayClass;
class JPArrayView;
class JPBoxedType;
class JPPrimitiveType;
class JPStringType;

// Members
class JPMethod;
class JPMethodDispatch;
class JPField;

// Services
class JPTypeFactory;
class JPTypeManager;
class JPClassLoader;
class JPReferenceQueue;
class JPProxyFactory;
class JPContext;

typedef vector<JPClass*> JPClassList;
typedef vector<JPField*> JPFieldList;
typedef vector<JPMethodDispatch*> JPMethodDispatchList;
typedef vector<JPMethod*> JPMethodList;

class JPResource
{
public:
	virtual ~JPResource() = 0;
} ;

// Macros for raising an exception with jpype
//   These must be macros so that we can update the pattern and
//   maintain the appropriate auditing information.  C++ does not
//   have a lot for facilities to make this easy.
#define JP_RAISE_PYTHON()                   { throw JPypeException(JPError::_python_error, NULL, JP_STACKINFO()); }
#define JP_RAISE_OS_ERROR_UNIX(err, msg)    { throw JPypeException(JPError::_os_error_unix,  msg, err, JP_STACKINFO()); }
#define JP_RAISE_OS_ERROR_WINDOWS(err, msg) { throw JPypeException(JPError::_os_error_windows,  msg, err, JP_STACKINFO()); }
#define JP_RAISE_METHOD_NOT_FOUND(msg)      { throw JPypeException(JPError::_method_not_found, NULL, msg, JP_STACKINFO()); }
#define JP_RAISE(type, msg)                 { throw JPypeException(JPError::_python_exc, type, msg, JP_STACKINFO()); }

#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

// Base utility headers
#include "jp_javaframe.h"
#include "jp_context.h"
#include "jp_exception.h"
#include "jp_pythontypes.h"
#include "jp_tracer.h"
#include "jp_typemanager.h"
#include "jp_encoding.h"
#include "jp_modifier.h"
#include "jp_match.h"

// Other header files
#include "jp_classhints.h"
#include "jp_method.h"
#include "jp_value.h"
#include "jp_class.h"

// Primitives classes
#include "jp_primitivetype.h"
#include "jp_typemanager.h"

// Accessors
#include "jp_array.h"
#include "jp_buffer.h"

// Services
#include "jp_proxy.h"

#endif // _JPYPE_H_
