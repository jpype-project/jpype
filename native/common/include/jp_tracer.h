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
#ifndef _JP_TRACER_H__
#define _JP_TRACER_H__

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

class JPypeTracer
{
private :
	string m_Name;	
	bool m_Error;
	
public :
	JPypeTracer(const char* name) : m_Name(name)
	{
		traceIn(name);
		m_Error = false;
	}
	
	virtual ~JPypeTracer()
	{
		traceOut(m_Name.c_str(), m_Error);
	}
	
	void gotError()
	{
		m_Error = true;
	}
	
	template <class T>
	void trace(T msg)
	{
#ifdef TRACING
		stringstream str;
		str << msg;
		trace1(m_Name.c_str(), str.str());
#endif
	}
	
	template <class T, class U>
	void trace(T msg1, U msg2)
	{
#ifdef TRACING
		stringstream str;
		str << msg1 << " " << msg2;
		trace1(m_Name.c_str(), str.str());
#endif
	}

	template <class T, class U, class V>
	void trace(T msg1, U msg2, V msg3)
	{
#ifdef TRACING
		stringstream str;
		str << msg1 << " " << msg2 << " " << msg3;
		trace1(m_Name.c_str(), str.str());
#endif
	}
	
private :
	static void traceIn(const char* msg);
	static void traceOut(const char* msg, bool error);
	static void trace1(const char* name, const string& msg);
};

#endif // _JP_TRACER_H__
