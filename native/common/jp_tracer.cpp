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
#include "jpype.h"
#include "jp_tracer.h"

// GCOVR_EXCL_START

#if defined(_MSC_VER) && _MSC_VER<1700

// Welcome to the year 2008!
namespace std
{

class mutex
{
} ;

template <class T> class lock_guard
{
public:

	lock_guard(const T& mutex_type)
	{
	}
} ;
}

#else
#include <mutex>
#endif

static int jpype_traceLevel = 0;
static JPypeTracer* jpype_tracer_last = nullptr;

std::mutex trace_lock;

#define JPYPE_TRACING_OUTPUT std::cerr
static int INDENT_WIDTH = 2;
static const char *INDENT =
		"                                                                                ";

static void jpype_indent(int space)
{
	space *= INDENT_WIDTH;
	while (space > 80)
	{
		JPYPE_TRACING_OUTPUT << INDENT;
		space -= 80;
	}
	JPYPE_TRACING_OUTPUT << &INDENT[80 - space];
}

//This code is not thread safe, thus tracing a multithreaded code is likely
// to result in crashes.

JPypeTracer::JPypeTracer(const char* name, void* reference) : m_Name(name)
{
	m_Error = false;
	m_Last = jpype_tracer_last;
	jpype_tracer_last = this;
	traceIn(name, reference);
}

JPypeTracer::~JPypeTracer()
{
	traceOut(m_Name.c_str(), m_Error);
	jpype_tracer_last = m_Last;
}

void JPypeTracer::traceIn(const char* msg, void* ref)
{
	if (_PyJPModule_trace == 0)
		return;

	if (jpype_traceLevel < 0)
		jpype_traceLevel = 0;

	std::lock_guard<std::mutex> guard(trace_lock);
	jpype_indent(jpype_traceLevel);
	JPYPE_TRACING_OUTPUT << "> " << msg ;
	if (ref != nullptr)
		JPYPE_TRACING_OUTPUT << " id=\"" << ref << "\"";
	JPYPE_TRACING_OUTPUT << std::endl;
	JPYPE_TRACING_OUTPUT.flush();
	jpype_traceLevel++;
}

void JPypeTracer::traceOut(const char* msg, bool error)
{
	if (_PyJPModule_trace == 0)
		return;

	std::lock_guard<std::mutex> guard(trace_lock);
	jpype_traceLevel--;
	jpype_indent(jpype_traceLevel);
	if (error)
	{
		JPYPE_TRACING_OUTPUT << "EXCEPTION! " << msg << std::endl;
	} else
	{
		JPYPE_TRACING_OUTPUT << "< " << msg << std::endl;
	}
	JPYPE_TRACING_OUTPUT.flush();
}

void JPypeTracer::traceJavaObject(const char* msg, const void* ref)
{
	if ((_PyJPModule_trace & 4) == 0)
		return;
	if (ref == (void*) nullptr)
	{
		JPypeTracer::trace1("JNI", msg);
		return;
	}
	if (ref == (void*) - 1)
	{
		JPypeTracer::trace1("+ JNI", msg);
		jpype_traceLevel++;
		return;
	}
	if (ref == (void*) - 2)
	{
		jpype_traceLevel--;
		JPypeTracer::trace1("- JNI", msg);
		return;
	}
	std::stringstream str;
	str << msg << " " << (void*) ref ;
	JPypeTracer::trace1("JNI", str.str().c_str());
}

void JPypeTracer::tracePythonObject(const char* msg, PyObject* ref)
{
	if ((_PyJPModule_trace & 2) == 0)
		return;
	if (ref != nullptr)
	{
		std::stringstream str;
		str << msg << " " << (void*) ref << " "<<
    #ifndef Py_GIL_DISABLED
        Py_REFCNT(ref)
    #else
        -1
    #endif
        << " " << Py_TYPE(ref)->tp_name;
		JPypeTracer::trace1("PY", str.str().c_str());

	} else
	{
		std::stringstream str;
		str << msg << " " << (void*) ref;
		JPypeTracer::trace1("PY", str.str().c_str());
	}
}

void JPypeTracer::trace1(const char* source, const char* msg)
{
	if (_PyJPModule_trace == 0)
		return;

	std::lock_guard<std::mutex> guard(trace_lock);
	string name = "unknown";

	if (jpype_tracer_last != nullptr)
		name = jpype_tracer_last->m_Name;

	jpype_indent(jpype_traceLevel);
	if (source != nullptr)
		JPYPE_TRACING_OUTPUT << source << ": ";
	if (source == nullptr || (_PyJPModule_trace & 16) != 0)
		JPYPE_TRACING_OUTPUT << name << ": ";
	JPYPE_TRACING_OUTPUT << msg << std::endl;
	JPYPE_TRACING_OUTPUT.flush();
}

void JPypeTracer::trace2(const char* msg1, const char* msg2)
{
	if (_PyJPModule_trace == 0)
		return;

	std::lock_guard<std::mutex> guard(trace_lock);
	string name = "unknown";

	if (jpype_tracer_last != nullptr)
		name = jpype_tracer_last->m_Name;

	jpype_indent(jpype_traceLevel);
	JPYPE_TRACING_OUTPUT << name << ": " << msg1 << " " << msg2 << std::endl;
	JPYPE_TRACING_OUTPUT.flush();
}

void JPypeTracer::traceLocks(const string& msg, void* ref)
{
	std::lock_guard<std::mutex> guard(trace_lock);
	JPYPE_TRACING_OUTPUT << msg << ": " << ref  << std::endl;
	JPYPE_TRACING_OUTPUT.flush();
}

// GCOVR_EXCL_STOP
