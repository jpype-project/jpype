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
#ifndef _JPYPE_UTILITY_H_
#define _JPYPE_UTILITY_H_

#define RAISE(exClass, msg) { throw new exClass(msg, __FILE__, __LINE__); }

/** Support Exception for JPype-generated exception */
class JPypeException
{
public :
	JPypeException(const char* msn, const char* f, int l) 
	{
		file=f, line=l;
		std::stringstream str;
		str << msn << " at " << f << ":" << l;
		this->msg = str.str();
	}

	JPypeException(const string& msn, const char* f, int l)
	{
		file=f, line=l;
		std::stringstream str;
		str << msn << " at " << f << ":" << l;
		this->msg = str.str();
	}

	JPypeException(const JPypeException& ex) : file(ex.file), line(ex.line) { this->msg = ex.msg;}

	virtual ~JPypeException() {}

	const char* getMsg()
	{
		return msg.c_str();
	}

	const char* file;
	int line;

private :
	string msg;
};


/*
#define STANDARD_CATCH \
catch(JavaException EXCEPTION_PTR ex) \
{ \
	JPEnv::getJava()->ExceptionDescribe(); \
	JPEnv::getJava()->ExceptionClear(); \
	stringstream msg; \
	msg << "Java Exception Occured at " <<  ex EXCEPTION_DEREF file << ":" << ex EXCEPTION_DEREF line << ":" << ex EXCEPTION_DEREF message; \
	JPEnv::getHost()->setRuntimeException(msg.str().c_str());\
	EXCEPTION_CLEANUP(ex); \
}\
catch(JPypeException EXCEPTION_PTR ex)\
{\
	JPEnv::getHost()->setRuntimeException(ex EXCEPTION_DEREF getMsg()); \
	EXCEPTION_CLEANUP(ex); \
}\
catch(...) \
{\
	JPEnv::getHost()->setRuntimeException("Unknown Exception"); \
} \
*/
	
#define RETHROW_CATCH(cleanup) \
catch(...) \
{ \
	cleanup ; \
	throw; \
} 

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

#endif // _JPYPE_UTILITY_H_
