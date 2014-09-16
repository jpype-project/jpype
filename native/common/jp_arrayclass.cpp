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
	JPCleaner cleaner;
	
	if (JPEnv::getHost()->isNone(o))
	{
		return _implicit;
	}
	
	if (JPEnv::getHost()->isArray(o))
	{
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
		// Strings are also char[]
		return _implicit;
	}
	else if (JPEnv::getHost()->isByteString(o) && m_ComponentType->getName().getType() ==JPTypeName::_byte)
	{
		// Strings are also char[]
		return _implicit;
	}
	else if (JPEnv::getHost()->isSequence(o))
	{
		EMatchType match = _implicit;
		int length = JPEnv::getHost()->getSequenceLength(o);
		for (int i = 0; i < length && match > _none; i++)
		{
			HostRef* obj = JPEnv::getHost()->getSequenceItem(o, i);
			cleaner.add(obj);
			EMatchType newMatch = m_ComponentType->canConvertToJava(obj);
			if (newMatch < match)
			{
				match = newMatch;
			}
		}
		return match;
	}
	
	return _none;
}

HostRef* JPArrayClass::asHostObject(jvalue val)
{
	if (val.l == NULL)
	{
		return JPEnv::getHost()->getNone();
	}
	return JPEnv::getHost()->newArray(new JPArray(m_Name, (jarray)val.l));
}

jvalue JPArrayClass::convertToJava(HostRef* obj)
{	
	JPCleaner cleaner;
	jvalue res;
	res.l = NULL;
	
	if (JPEnv::getHost()->isArray(obj))
	{
		JPArray* a = JPEnv::getHost()->asArray(obj);
		res = a->getValue();		
	}
	else if (JPEnv::getHost()->isByteString(obj) && m_ComponentType->getName().getType() == JPTypeName::_byte && sizeof(char) == sizeof(jbyte))
	{
		char* rawData;
		long size;
		JPEnv::getHost()->getRawByteString(obj, &rawData, size);
		
		jbyteArray array = JPEnv::getJava()->NewByteArray(size);
		cleaner.addLocal(array);
		res.l = array;

		jboolean isCopy;
		jbyte* contents = JPEnv::getJava()->GetByteArrayElements(array, &isCopy);
		memcpy(contents, rawData, size*sizeof(jbyte));
		JPEnv::getJava()->ReleaseByteArrayElements(array, contents, 0);
		
		cleaner.removeLocal(array);
	}
	else if (JPEnv::getHost()->isUnicodeString(obj) && m_ComponentType->getName().getType() == JPTypeName::_char && JPEnv::getHost()->getUnicodeSize() == sizeof(jchar))
	{
		jchar* rawData;
		long size;
		JPEnv::getHost()->getRawUnicodeString(obj, &rawData, size);
		
		jcharArray array = JPEnv::getJava()->NewCharArray(size);
		cleaner.addLocal(array);
		res.l = array;

		jboolean isCopy;
		jchar* contents = JPEnv::getJava()->GetCharArrayElements(array, &isCopy);
		memcpy(contents, rawData, size*sizeof(jchar));
		JPEnv::getJava()->ReleaseCharArrayElements(array, contents, 0);
		
		cleaner.removeLocal(array);
	}
	else if (JPEnv::getHost()->isSequence(obj))
	{
		int length = JPEnv::getHost()->getSequenceLength(obj);
		
		jarray array = m_ComponentType->newArrayInstance(length);
		cleaner.addLocal(array);
		res.l = array;
		
		for (int i = 0; i < length ; i++)
		{
			HostRef* obj2 = JPEnv::getHost()->getSequenceItem(obj, i);
			cleaner.add(obj2);
			
			m_ComponentType->setArrayItem(array, i, obj2);
		}
		cleaner.removeLocal(array);
	}
	
	return res;
}

JPArray* JPArrayClass::newInstance(int length)
{	
	JPCleaner cleaner;
	
	jarray array = m_ComponentType->newArrayInstance(length);
	cleaner.addLocal(array);
	
	JPArray* res = new JPArray(getName(), array);
	
	return res;
}

