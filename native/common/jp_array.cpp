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

JPArray::JPArray(const JPTypeName& name, jarray inst)
{
	JPJavaFrame frame;
	TRACE_IN("JPArray::JPArray");
	TRACE1(name.getSimpleName());
	m_Class = JPTypeManager::findArrayClass(name);
	m_Object = (jarray)frame.NewGlobalRef(inst);
	TRACE2("len=",getLength());
	TRACE_OUT;
}

JPArray::~JPArray()
{
	JPJavaFrame::ReleaseGlobalRef(m_Object);
}

int JPArray::getLength()
{
	JPJavaFrame frame;
	return frame.GetArrayLength(m_Object);
}

vector<HostRef*> JPArray::getRange(int start, int stop)
{
	JPJavaFrame frame;
	TRACE_IN("JPArray::getRange");
	JPType* compType = m_Class->getComponentType();
	TRACE2("Compoennt type", compType->getName().getSimpleName());
	
	vector<HostRef*> res = compType->getArrayRange(frame, m_Object, start, stop-start);
	
	return res;
	TRACE_OUT;
}	

PyObject* JPArray::getSequenceFromRange(int start, int stop)
{
	JPJavaFrame frame;
//	TRACE_IN("JPArray::getSequenceFromRange");
	JPType* compType = m_Class->getComponentType();
//	TRACE2("Component type", compType->getName().getSimpleName());

	return compType->getArrayRangeToSequence(frame, m_Object, start, stop);
//  TRACE_OUT
}

void JPArray::setRange(int start, int stop, vector<HostRef*>& val)
{
	JPJavaFrame frame;
	JPType* compType = m_Class->getComponentType();
	
	unsigned int len = stop-start;
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
			
	compType->setArrayRange(frame, m_Object, start, stop-start, val);
}

void JPArray::setRange(int start, int stop, PyObject* sequence)
{
	JPJavaFrame frame;
	JPType* compType = m_Class->getComponentType();
	unsigned int len = stop-start;
	// check bounds of sequence which is to be assigned
	HostRef h(sequence);
	unsigned int plength = JPEnv::getHost()->getSequenceLength(&h);

	if (len != plength)
	{
		std::stringstream out;
		out << "Slice assignment must be of equal lengths : " << len << " != " << plength;
		RAISE(JPypeException, out.str());
	}

	compType->setArrayRange(frame, m_Object, start, len, sequence);
}

void JPArray::setItem(int ndx, HostRef* val)
{
	JPJavaFrame frame;
	JPType* compType = m_Class->getComponentType();
	if (compType->canConvertToJava(val) <= _explicit)
	{
		RAISE(JPypeException, "Unable to convert.");
	}	
	
	compType->setArrayItem(frame, m_Object, ndx, val);
}

HostRef* JPArray::getItem(int ndx)
{
	JPJavaFrame frame;
	JPType* compType = m_Class->getComponentType();

	return compType->getArrayItem(frame, m_Object, ndx);
}

JPType* JPArray::getType()
{
	return m_Class;
}

jvalue  JPArray::getValue()
{
	jvalue val;
	val.l = m_Object;
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
