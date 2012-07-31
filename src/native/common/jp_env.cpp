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

HostEnvironment* JPEnv::s_Host = NULL;
JPJavaEnv*       JPEnv::s_Java = NULL;

void JPEnv::init(HostEnvironment* hostEnv)
{
	s_Host = hostEnv;
	
	JPTypeName::init();  
}
	
void JPEnv::loadJVM(const string& vmPath, char ignoreUnrecognized, const StringVector& args)
{
	TRACE_IN("JPEnv::loadJVM");
	
	JavaVMInitArgs jniArgs;
	jniArgs.options = NULL;
	
	JPJavaEnv::load(vmPath);

	// prepare this ...
	jniArgs.version = JNI_VERSION_1_4;
	jniArgs.ignoreUnrecognized = ignoreUnrecognized;
		
	jniArgs.nOptions = (jint)args.size();
	jniArgs.options = (JavaVMOption*)malloc(sizeof(JavaVMOption)*jniArgs.nOptions);
	memset(jniArgs.options, 0, sizeof(JavaVMOption)*jniArgs.nOptions);
	
	for (int i = 0; i < jniArgs.nOptions; i++)
	{
		jniArgs.options[i].optionString = (char*)args[i].c_str();
	}

	s_Java = JPJavaEnv::CreateJavaVM((void*)&jniArgs);  
    
	if (s_Java == NULL) {
		RAISE(JPypeException, "Unable to start JVM");
	}

	JPTypeManager::init();
	JPJni::init();
	JPProxy::init();

	TRACE_OUT;
}

void JPEnv::attachJVM(const string& vmPath)
{
	TRACE_IN("JPEnv::attachJVM");

	JPJavaEnv::load(vmPath);

	s_Java = JPJavaEnv::GetCreatedJavaVM(); 
	
	if (s_Java == NULL) {
		RAISE(JPypeException, "Unable to attach to JVM");
	}

	JPTypeManager::init();
	JPJni::init();
	JPProxy::init();

	TRACE_OUT;
	
}

void JPEnv::attachCurrentThread()
{
	s_Java->AttachCurrentThread();
}

void JPEnv::attachCurrentThreadAsDaemon()  
{
	s_Java->AttachCurrentThreadAsDaemon();
}

bool JPEnv::isThreadAttached()
{
	return s_Java->isThreadAttached();
}


void JPEnv::registerRef(HostRef* ref, HostRef* targetRef)
{
	TRACE_IN("JPEnv::registerRef");
	JPObject* objRef = s_Host->asObject(ref);
	JPCleaner cleaner;
	TRACE1("A");
	jobject srcObject = objRef->getObject();
	cleaner.addLocal(srcObject);
	JPJni::registerRef(s_Java->getReferenceQueue(), srcObject, (jlong)targetRef->copy());
	TRACE_OUT;
	TRACE1("B");
}


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

JPCleaner::JPCleaner()
{
}

JPCleaner::~JPCleaner()
{
//AT's comments on porting:
// A variety of Unix compilers do not allow redifinition of the same variable in "for" cycless
	vector<jobject>::iterator cur;
	for (cur = m_GlobalJavaObjects.begin(); cur != m_GlobalJavaObjects.end(); cur++)
	{
		JPEnv::getJava()->DeleteGlobalRef(*cur);
	}
	
	for (cur = m_LocalJavaObjects.begin(); cur != m_LocalJavaObjects.end(); cur++)
	{
		JPEnv::getJava()->DeleteLocalRef(*cur);
	}

	for (vector<HostRef*>::iterator cur2 = m_HostObjects.begin(); cur2 != m_HostObjects.end(); cur2++)
	{
		(*cur2)->release();
	}
}

void JPCleaner::addGlobal(jobject obj)
{
	m_GlobalJavaObjects.push_back(obj);
}

void JPCleaner::addLocal(jobject obj)
{
	m_LocalJavaObjects.push_back(obj);
}

void JPCleaner::add(HostRef* obj)
{
	m_HostObjects.push_back(obj);
}

void JPCleaner::removeGlobal(jobject obj)
{
	for (vector<jobject>::iterator cur = m_GlobalJavaObjects.begin(); cur != m_GlobalJavaObjects.end(); cur++)
	{
		if (*cur == obj)
		{
			m_GlobalJavaObjects.erase(cur);
			return;
		}
	}
}

void JPCleaner::removeLocal(jobject obj)
{
	for (vector<jobject>::iterator cur = m_LocalJavaObjects.begin(); cur != m_LocalJavaObjects.end(); cur++)
	{
		if (*cur == obj)
		{
			m_LocalJavaObjects.erase(cur);
			return;
		}
	}
}

void JPCleaner::remove(HostRef* obj)
{
	for (vector<HostRef*>::iterator cur2 = m_HostObjects.begin(); cur2 != m_HostObjects.end(); cur2++)
	{
		if (*cur2 == obj)
		{
			m_HostObjects.erase(cur2);
			return;
		}
	}
}

void JPCleaner::addAllGlobal(vector<jobject>& r) 
{
	for (vector<jobject>::iterator cur = r.begin(); cur != r.end(); cur++)
	{
		addGlobal(*cur);
	}
}

void JPCleaner::addAllGlobal(vector<jclass>& r) 
{
	for (vector<jclass>::iterator cur = r.begin(); cur != r.end(); cur++)
	{
		addGlobal(*cur);
	}
}

void JPCleaner::addAllLocal(vector<jobject>& r) 
{
	for (vector<jobject>::iterator cur = r.begin(); cur != r.end(); cur++)
	{
		addLocal(*cur);
	}
}

void JPCleaner::addAllLocal(vector<jclass>& r) 
{
	for (vector<jclass>::iterator cur = r.begin(); cur != r.end(); cur++)
	{
		addLocal(*cur);
	}
}

void JPCleaner::addAll(vector<HostRef*>& r) 
{
	for (vector<HostRef*>::iterator cur = r.begin(); cur != r.end(); cur++)
	{
		add(*cur);
	}
}

void JPCleaner::removeAllGlobal(vector<jobject>& r)
{
	for (vector<jobject>::iterator cur = r.begin(); cur != r.end(); cur++)
	{
		removeGlobal(*cur);
	}
}

void JPCleaner::removeAllLocal(vector<jobject>& r)
{
	for (vector<jobject>::iterator cur = r.begin(); cur != r.end(); cur++)
	{
		removeLocal(*cur);
	}
}

void JPCleaner::removeAll(vector<HostRef*>& r)
{
	for (vector<HostRef*>::iterator cur = r.begin(); cur != r.end(); cur++)
	{
		remove(*cur);
	}
}

HostRef::HostRef(void* data, bool acquire)
{
	if (acquire)
	{
		m_HostData = JPEnv::getHost()->acquireRef(data);
	}
	else
	{
		m_HostData = data;
	}
}

HostRef::HostRef(void* data)
{
	m_HostData = JPEnv::getHost()->acquireRef(data);
}

HostRef::~HostRef()
{
	JPEnv::getHost()->releaseRef(m_HostData);
}
	
HostRef* HostRef::copy()
{
	return new HostRef(m_HostData);
}

void HostRef::release()
{
	delete this;
}

bool HostRef::isNull()
{
	return JPEnv::getHost()->isRefNull(m_HostData);
}

void* HostRef::data()
{
	return m_HostData;
}

JCharString::JCharString(const jchar* c)
{
	m_Length = 0;
	while (c[m_Length] != 0)
	{
		m_Length ++;
	}
	
	m_Value = new jchar[m_Length+1];
	m_Value[m_Length] = 0;
	for (unsigned int i = 0; i < m_Length; i++)
	{
		m_Value[i] = c[i];
	}
}

JCharString::JCharString(const JCharString& c)
{
	m_Length = c.m_Length;
	m_Value = new jchar[m_Length+1];
	m_Value[m_Length] = 0;
	for (unsigned int i = 0; i < m_Length; i++)
	{
		m_Value[i] = c.m_Value[i];
	}
	
}

JCharString::JCharString(size_t len)
{
	m_Length = len;
	m_Value = new jchar[len+1];
	for (size_t i = 0; i <= len; i++)
	{
		m_Value[i] = 0;
	}
}

JCharString::~JCharString()
{
	if (m_Value != NULL)
	{
		delete m_Value;
	}
}
	
const jchar* JCharString::c_str()
{
	return m_Value;
}
