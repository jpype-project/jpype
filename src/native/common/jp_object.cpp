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

JPObject::JPObject(JPTypeName& c, jobject o)
{
	m_Class = JPTypeManager::findClass(c);
	m_Object = JPEnv::getJava()->NewGlobalRef(o);
}
  
JPObject::JPObject(JPClass* c, jobject o)
{
	m_Class = c;
	m_Object = JPEnv::getJava()->NewGlobalRef(o);
}

JPObject::~JPObject()
{
	JPEnv::getJava()->DeleteGlobalRef(m_Object);
}

JCharString JPObject::toString()
{
	if (m_Object == NULL)
	{
		static const char* value = "null";
		jchar res[5];
		res[4] = 0;
		for (int i = 0; value[i] != 0; i++)
		{
			res[i] = value[i];
		}
		return res;
	}

	JPCleaner cleaner;

	jstring jval = JPJni::toString(m_Object);
	cleaner.addLocal(jval);

	JCharString result = JPJni::unicodeFromJava(jval);

	return result;
	
}

HostRef* JPObject::getAttribute(string name)
{
	TRACE_IN("JPObject::getAttribute");
	TRACE1(name);

	JPCleaner cleaner;
	
	// instance fields ...
	JPField* fld = m_Class->getInstanceField(name);	
	if (fld != NULL)
	{
		return fld->getAttribute(m_Object);
	}
	
	// static fields ...
	fld = m_Class->getStaticField(name);
	if (fld != NULL)
	{
		return fld->getStaticAttribute();
	}
		
	JPEnv::getHost()->setAttributeError(name.c_str());
	JPEnv::getHost()->raise("getAttribute");

	return NULL; // never reached ...
	
	TRACE_OUT;
}

void JPObject::setAttribute(string name, HostRef* val)
{
	// instance fields ...
	JPField* fld = m_Class->getInstanceField(name);	
	if (fld != NULL)
	{
		fld->setAttribute(m_Object, val);
		return;
	}
	
	// static fields ...
	fld = m_Class->getStaticField(name);
	if (fld != NULL)
	{
		fld->setStaticAttribute(val);
		return;
	}
	
	
	JPEnv::getHost()->setAttributeError(name.c_str());
	JPEnv::getHost()->raise("setAttribute");
}
