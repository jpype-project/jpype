/*****************************************************************************
   Copyright 2004-2008 Steve Menard

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
#include <jpype.h>

static int jpype_traceLevel = 0;

#ifdef JPYPE_TRACING_INTERNAL
	static const bool trace_internal = true;  
#else
	static const bool trace_internal = false;
#endif


void JPypeTracer::traceIn(const char* msg)
{
	if (trace_internal)
	{
		for (int i = 0; i < jpype_traceLevel; i++)
		{
			JPYPE_TRACING_OUTPUT << "  ";
		}
		JPYPE_TRACING_OUTPUT << "<B msg=\"" << msg << "\" >" << endl;
		JPYPE_TRACING_OUTPUT.flush();
		jpype_traceLevel ++;
	}
}

void JPypeTracer::traceOut(const char* msg, bool error)
{
	if (trace_internal)
	{
		jpype_traceLevel --;
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
}  

void JPypeTracer::trace1(const char* name, const string& msg)  
{
	if (trace_internal)
	{
		for (int i = 0; i < jpype_traceLevel; i++)
		{
			JPYPE_TRACING_OUTPUT << "  ";
		}
		JPYPE_TRACING_OUTPUT << "<M>" << name << " : " << msg << "</M>" << endl;
		JPYPE_TRACING_OUTPUT.flush();
	}
}

