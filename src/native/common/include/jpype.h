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

// Define this to make the trace calls do their output. If you change this only the core.cpp needs to be recompiled
//#define JPYPE_TRACING_INTERNAL
#define JPYPE_TRACING_OUTPUT cerr

#define TRACE_IN(n) JPypeTracer _trace(n); try {
#define TRACE_OUT } catch(...) { _trace.gotError(); throw; }
#define TRACE1(m) _trace.trace(m)
#define TRACE2(m,n) _trace.trace(m,n)
#define TRACE3(m,n,o) _trace.trace(m,n,o)

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

#include <jni.h>

#ifdef WIN32
	#ifdef __GNUC__
		// JNICALL causes problem for funtions prototypes .. since I am nto defining any JNI methods there isno need for it
		#undef JNICALL
		#define JNICALL
	#endif
#endif


#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string.h>

using namespace std;

/** The following functions are delcared here but actually defined in the platform_specific file */
void longToHexString(long value, char* outStr);

/** Definition of commonly used template types */
typedef vector<string> StringVector;

/** Use this class instewad of basic_string<jchar> because compiler support is not great cross-platform */
class JCharString
{
public :
	JCharString(const jchar*);
	JCharString(const JCharString&);
	JCharString(size_t);
	virtual ~JCharString();
	
	const jchar* c_str();
	
	size_t length() { return m_Length; }
	
	jchar& operator[](size_t ndx) { return m_Value[ndx]; }
	
private :
	jchar* m_Value;
	size_t m_Length;
};


// Base utility headers
#include "jp_typename.h"
#include "jp_utility.h"
#include "jp_javaenv.h"
#include "jp_jniutil.h"

#include "jp_hostenv.h"
#include "jp_env.h"

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
