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

// Define this to generate the trace calls

// Define this to make the trace calls do their output. 
//    use "setup.py --enable-tracing build" to enable 
//#define JP_TRACING_ENABLE

#ifdef WIN32
#define JPYPE_WIN32

#ifndef __GNUC__ // Then this must mean a variant of GCC on win32 ...
#define JPYPE_WIN32_VCPP
#pragma warning (disable:4786)
#else
#endif

#if defined(__CYGWIN__)
// jni_md.h does not work for cygwin.  Use this instead.
#elif defined(__GNUC__)
// JNICALL causes problem for function prototypes .. since I am not defining any JNI methods there is no need for it
#undef JNICALL
#define JNICALL
#endif

#else
#define JPYPE_LINUX
#endif

#include <jni.h>

#if PY_MAJOR_VERSION >= 3
// Python 3
#define PyInt_FromLong PyLong_FromLong
#define PyInt_AsLong PyLong_AsLong
#define PyInt_AS_LONG PyLong_AS_LONG
#define PyInt_Check PyLong_Check
#define PyInt_FromSsize_t PyLong_FromSsize_t
#else
#undef PyUnicode_FromFormat
#define PyUnicode_FromFormat PyString_FromFormat
#endif

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

/** Definition of commonly used template types */
typedef vector<string> StringVector;

// Types
class JPClass;
class JPArrayClass;
class JPValue;
class JPProxy;
class JPArray;
class JPBoxedType;
class JPVoidType;
class JPBooleanType;
class JPByteType;
class JPCharType;
class JPShortType;
class JPIntType;
class JPLongType;
class JPFloatType;
class JPDoubleType;
class JPStringClass;

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

// Base utility headers
#include "jp_javaframe.h"
#include "jp_context.h"
#include "jp_exception.h"
#include "jp_env.h"
#include "jp_pythontypes.h"
#include "jp_pythonenv.h"
#include "jp_tracer.h"
#include "jp_typename.h"
#include "jp_typemanager.h"
#include "jp_encoding.h"
#include "jp_modifier.h"

// Other header files
#include "jp_method.h"
#include "jp_value.h"

// Primitives classes
#include "jp_primitivetype.h"
#include "jp_voidtype.h"
#include "jp_booleantype.h"
#include "jp_bytetype.h"
#include "jp_chartype.h"
#include "jp_shorttype.h"
#include "jp_inttype.h"
#include "jp_longtype.h"
#include "jp_floattype.h"
#include "jp_doubletype.h"

// Accessors
#include "jp_field.h"
#include "jp_methoddispatch.h"
#include "jp_array.h"
#include "jp_class.h"

// Object classes
#include "jp_arrayclass.h"
#include "jp_stringclass.h"
#include "jp_objecttype.h"
#include "jp_classtype.h"
#include "jp_boxedclasses.h"

// Services
#include "jp_reference_queue.h"
#include "jp_classloader.h"
#include "jp_proxy.h"
#include "jp_monitor.h"

#endif // _JPYPE_H_
