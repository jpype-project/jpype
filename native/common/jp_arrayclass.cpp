/*****************************************************************************
   Copyright 2004 Steve M�nard

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



JPArrayClass::JPArrayClass(const JPTypeName& tname, jclass c) :
	JPClassBase(tname, c)
{
	JPTypeName compname = m_Name.getComponentName();
	m_ComponentType = JPTypeManager::getType(compname);
}

JPArrayClass::~JPArrayClass()
{
}

EMatchType JPArrayClass::canConvertToJava(HostRef* o)
{
	TRACE_IN("JPArrayClass::canConvertToJava");
	JPLocalFrame frame;
	
	if (JPEnv::getHost()->isNone(o))
	{
		return _implicit;
	}
	
	if (JPEnv::getHost()->isArray(o))
	{
		TRACE1("array");
		JPArray* a = JPEnv::getHost()->asArray(o);
		
		JPArrayClass* ca = a->getClass();
		
		if (ca == this)
		{
			return _exact;
		}
		
		if (JPEnv::getJava()->IsAssignableFrom(ca->m_Class, m_Class))
		{
			return _implicit;
		}
	}
	else if (JPEnv::getHost()->isUnicodeString(o) && m_ComponentType->getName().getType() ==JPTypeName::_char)
	{
		TRACE1("char[]");
		// Strings are also char[]
		return _implicit;
	}
	else if (JPEnv::getHost()->isByteString(o) && m_ComponentType->getName().getType() ==JPTypeName::_byte)
	{
		TRACE1("char[]");
		// Strings are also char[]
		return _implicit;
	}
	else if (JPEnv::getHost()->isSequence(o) && !JPEnv::getHost()->isObject(o))
	{
		TRACE1("Sequence");
		EMatchType match = _implicit;
		int length = JPEnv::getHost()->getSequenceLength(o);
		for (int i = 0; i < length && match > _none; i++)
		{
			HostRef* obj = JPEnv::getHost()->getSequenceItem(o, i);
			EMatchType newMatch = m_ComponentType->canConvertToJava(obj);
			if (newMatch < match)
			{
				match = newMatch;
			}
			delete obj;
		}
		return match;
	}
	
	return _none;
	TRACE_OUT;
}

HostRef* JPArrayClass::asHostObject(jvalue val)
{
	TRACE_IN("JPArrayClass::asHostObject")
	if (val.l == NULL)
	{
		return JPEnv::getHost()->getNone();
	}
	return JPEnv::getHost()->newArray(new JPArray(m_Name, (jarray)val.l));
	TRACE_OUT;
}

jvalue JPArrayClass::convertToJava(HostRef* obj)
{	
	TRACE_IN("JPArrayClass::convertToJava");
	JPLocalFrame frame;
	jvalue res;
	res.l = NULL;
	
	if (JPEnv::getHost()->isArray(obj))
	{
		TRACE1("direct");
		JPArray* a = JPEnv::getHost()->asArray(obj);
		res = a->getValue();		
	}
	else if (JPEnv::getHost()->isByteString(obj) && m_ComponentType->getName().getType() == JPTypeName::_byte && sizeof(char) == sizeof(jbyte))
	{
		TRACE1("char[]");
		char* rawData;
		long size;
		JPEnv::getHost()->getRawByteString(obj, &rawData, size);
		
		jbyteArray array = JPEnv::getJava()->NewByteArray(size);

		jboolean isCopy;
		jbyte* contents = JPEnv::getJava()->GetByteArrayElements(array, &isCopy);
		memcpy(contents, rawData, size*sizeof(jbyte));
		JPEnv::getJava()->ReleaseByteArrayElements(array, contents, 0);
		
		res.l = array;
	}
	else if (JPEnv::getHost()->isUnicodeString(obj) && m_ComponentType->getName().getType() == JPTypeName::_char && JPEnv::getHost()->getUnicodeSize() == sizeof(jchar))
	{
		TRACE1("uchar[]");
		jchar* rawData;
		long size;
		JPEnv::getHost()->getRawUnicodeString(obj, &rawData, size);
		
		jcharArray array = JPEnv::getJava()->NewCharArray(size);

		jboolean isCopy;
		jchar* contents = JPEnv::getJava()->GetCharArrayElements(array, &isCopy);
		memcpy(contents, rawData, size*sizeof(jchar));
		JPEnv::getJava()->ReleaseCharArrayElements(array, contents, 0);
		
		res.l = array;
	}
	else if (JPEnv::getHost()->isSequence(obj))
	{
		TRACE1("sequence");
		int length = JPEnv::getHost()->getSequenceLength(obj);
		
		jarray array = m_ComponentType->newArrayInstance(length);
		
		for (int i = 0; i < length ; i++)
		{
			HostRef* obj2 = JPEnv::getHost()->getSequenceItem(obj, i);
			m_ComponentType->setArrayItem(array, i, obj2);
			delete obj2;
		}
		res.l = array;
	}

	res.l = frame.keep(res.l);
	return res;
	TRACE_OUT;
}

jvalue JPArrayClass::convertToJavaVector(vector<HostRef*>& refs, size_t start, size_t end)
{
	JPLocalFrame frame;
	TRACE_IN("JPArrayClass::convertToJavaVector");
	int length = end-start;

	jarray array = m_ComponentType->newArrayInstance(length);
	jvalue res;
		
	for (size_t i = start; i < end ; i++)
	{
		m_ComponentType->setArrayItem(array, i-start, refs[i]);
	}
	res.l = frame.keep(array);
	return res;
	TRACE_OUT;
}

JPArray* JPArrayClass::newInstance(int length)
{	
	JPLocalFrame frame;
	jarray array = m_ComponentType->newArrayInstance(length);
	return  new JPArray(getName(), array);
}

