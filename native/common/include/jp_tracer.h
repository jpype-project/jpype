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


#ifdef JP_TRACING_ENABLE
#define JP_TRACE_IN(n) JPypeTracer _trace(n); try {
#define JP_TRACE_OUT } catch(...) { _trace.gotError(JP_STACKINFO()); throw; }
#define JP_TRACE(...) JPypeTracer::trace(__VA_ARGS__)
#define JP_TRACE_PY(m, obj) JPypeTracer::tracePythonObject(m, obj)
#else
#define JP_TRACE_IN(n)  try {
#define JP_TRACE_OUT } catch (JPypeException &ex) { ex.from(JP_STACKINFO()); throw; }
#define JP_TRACE(...)
#define JP_TRACE_PY(m, obj) 
#endif

// Enable this option to get all the py referencing information
//#define JP_ENABLE_TRACE_PY

class JPypeTracer
{
private:
	string m_Name;
	bool m_Error;
	JPypeTracer* m_Last;

public:

	JPypeTracer(const char* name);
	~JPypeTracer();

	void gotError(const JPStackInfo& info)
	{
		m_Error = true;
		try
		{
			throw;
		} catch (JPypeException& ex)
		{
			ex.from(info);
			throw;
		}
	}

	template <class T>
	static void trace(const T& msg)
	{
		stringstream str;
		str << msg;
		trace1(str.str());
	}

	template <class T1, class T2>
	static void trace(const T1& msg1, const T2& msg2)
	{
		stringstream str;
		str << msg1 << " " << msg2;
		trace1(str.str());
	}

	template <class T1, class T2, class T3>
	static void trace(const T1& msg1, const T2& msg2, const T3& msg3)
	{
		stringstream str;
		str << msg1 << " " << msg2 << " " << msg3;
		trace1(str.str());
	}

	template <class T1, class T2, class T3, class T4>
	static void trace(const T1& msg1, const T2& msg2, const T3& msg3, const T4& msg4)
	{
		stringstream str;
		str << msg1 << " " << msg2 << " " << msg3 << " " << msg4;
		trace1(str.str());
	}

	template <class T1, class T2, class T3, class T4, class T5>
	static void trace(const T1& msg1, const T2& msg2, const T3& msg3, const T4& msg4, const T5& msg5)
	{
		stringstream str;
		str << msg1 << " " << msg2 << " " << msg3 << " " << msg4 << " " << msg5;
		trace1(str.str());
	}

	static void tracePythonObject(const char* msg, PyObject* ref);

private:
	static void traceIn(const char* msg);
	static void traceOut(const char* msg, bool error);
	static void trace1(const string& msg);
} ;

#endif // _JP_TRACER_H__
