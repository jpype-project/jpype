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

static int jpype_traceLevel = 0;
static JPypeTracer* jpype_tracer_last = NULL;

#define JPYPE_TRACING_OUTPUT cerr

//This code is not thread safe, thus tracing a multithreaded code is likely
// to result in crashes.

JPypeTracer::JPypeTracer(const char* name) : m_Name(name)
{
	traceIn(name);
	m_Error = false;
	m_Last = jpype_tracer_last;
	jpype_tracer_last = this;
}

JPypeTracer::~JPypeTracer()
{
	traceOut(m_Name.c_str(), m_Error);
	jpype_tracer_last = m_Last;
}

void JPypeTracer::traceIn(const char* msg)
{
	for (int i = 0; i < jpype_traceLevel; i++)
	{
		JPYPE_TRACING_OUTPUT << "  ";
	}
	JPYPE_TRACING_OUTPUT << "<B msg=\"" << msg << "\" >" << endl;
	JPYPE_TRACING_OUTPUT.flush();
	jpype_traceLevel++;
}

void JPypeTracer::traceOut(const char* msg, bool error)
{
	jpype_traceLevel--;
	for (int i = 0; i < jpype_traceLevel; i++)
	{
		JPYPE_TRACING_OUTPUT << "  ";
	}
	if (error)
	{
		JPYPE_TRACING_OUTPUT << "</B> <!-- !!!!!!!! EXCEPTION !!!!!! " << msg << " -->" << endl;
	}
	else
	{
		JPYPE_TRACING_OUTPUT << "</B> <!-- " << msg << " -->" << endl;
	}
	JPYPE_TRACING_OUTPUT.flush();
}

void JPypeTracer::tracePythonObject(const char* msg, PyObject* ref)
{
#ifdef JP_ENABLE_TRACE_PY
	stringstream ss;
	ss << msg << " " << (void*) ref;
	if (ref != NULL)
	{
		ss << " " << ref->ob_refcnt << " " << ref->ob_type->tp_name;
	}
	trace1(ss.str());
#endif
}

void JPypeTracer::trace1(const string& msg)
{
	string name = "unknown";

	if (jpype_tracer_last != NULL)
		name = jpype_tracer_last->m_Name;

	for (int i = 0; i < jpype_traceLevel; i++)
	{
		JPYPE_TRACING_OUTPUT << "  ";
	}
	JPYPE_TRACING_OUTPUT << "<M>" << name << " : " << msg << "</M>" << endl;
	JPYPE_TRACING_OUTPUT.flush();
}

