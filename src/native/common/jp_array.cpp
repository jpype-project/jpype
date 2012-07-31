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
#include <jpype.h>

JPArray::JPArray(JPTypeName name, jarray inst) 
{
	m_Class = JPTypeManager::findArrayClass(name);
	m_Object = (jarray)JPEnv::getJava()->NewGlobalRef(inst);
}

JPArray::~JPArray()
{
	JPEnv::getJava()->DeleteGlobalRef(m_Object);
}

int JPArray::getLength()
{
	return JPEnv::getJava()->GetArrayLength(m_Object);
}

vector<HostRef*> JPArray::getRange(int start, int stop)
{
	TRACE_IN("JPArray::getRange");
	JPType* compType = m_Class->getComponentType();
	TRACE2("Compoennt type", compType->getName().getSimpleName());
	
	vector<HostRef*> res = compType->getArrayRange(m_Object, start, stop-start);
	
	return res;
	TRACE_OUT;
}	

void JPArray::setRange(int start, int stop, vector<HostRef*>& val)
{
	JPCleaner cleaner;
	
	JPType* compType = m_Class->getComponentType();
	
	int len = stop-start;
	size_t plength = val.size();
	
	if (len != plength)
	{
		std::stringstream out;
		out << "Slice assignment must be of equal lengths : " << len << " != " << plength;
		RAISE(JPypeException, out.str());
	}

	for (size_t i = 0; i < plength; i++)
	{
		HostRef* v = val[i];
		if ( compType->canConvertToJava(v)<= _explicit)
		{
			RAISE(JPypeException, "Unable to convert.");
		}
	}	
			
	compType->setArrayRange(m_Object, start, stop-start, val);
}	

void JPArray::setItem(int ndx, HostRef* val)
{
	JPType* compType = m_Class->getComponentType();
	if (compType->canConvertToJava(val) <= _explicit)
	{
		RAISE(JPypeException, "Unable to convert.");
	}	
	
	compType->setArrayItem(m_Object, ndx, val);
}

HostRef* JPArray::getItem(int ndx)
{
	JPType* compType = m_Class->getComponentType();

	return compType->getArrayItem(m_Object, ndx);
}

JPType* JPArray::getType()
{
	return m_Class;
}

jvalue  JPArray::getValue()
{
	jvalue val;
	val.l = JPEnv::getJava()->NewLocalRef(m_Object);
	return val;
}
JCharString JPArray::toString()
{
	static const char* value = "Array wrapper";
	jchar res[14];
	res[13] = 0;
	for (int i = 0; value[i] != 0; i++)
	{
		res[i] = value[i];
	}
	
	return res;
}
