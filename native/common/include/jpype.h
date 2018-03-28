/*****************************************************************************
   Copyright 2004 Steve Menard

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
//#define TRACING

// Define this to make the trace calls for referencing.
//#define MTRACING

#ifdef TRACING
  #define JPYPE_TRACING_INTERNAL
#endif
#define JPYPE_TRACING_OUTPUT cerr

#ifdef TRACING
  #define TRACE_IN(n) JPypeTracer _trace(n); try {
  #define TRACE_OUT } catch(...) { _trace.gotError(); throw; }
  #define TRACE1(m) _trace.trace(m)
  #define TRACE2(m,n) _trace.trace(m,n)
  #define TRACE3(m,n,o) _trace.trace(m,n,o)
#else
  #define TRACE_IN(n)
  #define TRACE_OUT
  #define TRACE1(m)
  #define TRACE2(m,n)
  #define TRACE3(m,n,o)
#endif

#ifdef MTRACING
  #define MTRACE_IN(n) JPypeTracer _trace(n); try {
  #define MTRACE_OUT } catch(...) { _trace.gotError(); throw; }
  #define MTRACE1(m) _trace.trace(m)
  #define MTRACE2(m,n) _trace.trace(m,n)
  #define MTRACE3(m,n,o) _trace.trace(m,n,o)
#else
  #define MTRACE_IN(n)
  #define MTRACE_OUT
  #define MTRACE1(m)
  #define MTRACE2(m,n)
  #define MTRACE3(m,n,o)
#endif

#ifdef WIN32
	#define JPYPE_WIN32

	#ifndef __GNUC__ // Then this must mean a variant of GCC on win32 ...
		#define JPYPE_WIN32_VCPP
		#pragma warning (disable:4786)
	#else
	#endif
	
#else
	#define JPYPE_LINUX
#endif

#ifdef WIN32
	#if defined(__CYGWIN__)
		// jni_md.h does not work for cygwin.  Use this instead.
		//#define _JAVASOFT_JNI_MD_H_
		//#define JNIEXPORT __declspec(dllexport)
		//#define JNIIMPORT __declspec(dllimport)
	#elif defined(__GNUC__)
		// JNICALL causes problem for function prototypes .. since I am not defining any JNI methods there is no need for it
		#undef JNICALL
		#define JNICALL
	#endif
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
    #define PyUnicode_FromFormat PyString_FromFormat
#endif

// Define this and use to allow destructors to throw in C++11 or later
#if __cplusplus >= 201103L
#define NO_EXCEPT_FALSE noexcept(false)
#else
#define NO_EXCEPT_FALSE
#endif

#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>

using std::map;
using std::string;
using std::stringstream;
using std::cout; using std::cerr; using std::endl;
using std::vector;

/** The following functions are delcared here but actually defined in the platform_specific file */
void longToHexString(long value, char* outStr);

/** Definition of commonly used template types */
typedef vector<string> StringVector;


// Base utility headers
#include "jp_typename.h"
#include "jp_utility.h"
#include "jp_javaenv.h"
#include "jp_hostenv.h"
#include "jp_env.h"
#include "jp_jniutil.h"


// Other header files
#include "jp_type.h"
#include "jp_primitivetypes.h"
#include "jp_objecttypes.h"

#include "jp_field.h"
#include "jp_methodoverload.h"
#include "jp_method.h"
#include "jp_classbase.h"
#include "jp_class.h"
#include "jp_arrayclass.h"
#include "jp_typemanager.h"

#include "jp_objectbase.h"
#include "jp_object.h"
#include "jp_array.h"

#include "jp_invocationhandler.h"
#include "jp_reference.h"
#include "jp_referencequeue.h"
#include "jp_proxy.h"

#include "jp_monitor.h"
#include "jp_env.h"

#endif // _JPYPE_H_
