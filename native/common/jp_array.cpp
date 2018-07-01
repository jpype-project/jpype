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

// Note: java represents arrays of zero length as null, thus we
// need to be careful to handle these properly.  We need to 
// carry them around so that we can match types.

JPArray::JPArray(JPClass* cls, jarray inst) : m_Object(inst)
{
	JPJavaFrame frame;
	JP_TRACE_IN("JPArray::JPArray");
	m_Class = (JPArrayClass*) cls;
	ASSERT_NOT_NULL(m_Class);
	JP_TRACE(m_Class->toString());

	// We will use this during range checks, so cache it
	if (m_Object.get() == NULL)
		m_Length = 0;
	else
		m_Length = frame.GetArrayLength(m_Object.get());

	JP_TRACE_OUT;
}

JPArray::~JPArray()
{
}

int JPArray::getLength()
{
	return m_Length;
}

JPPyObject JPArray::getRange(size_t start, size_t stop)
{
	JPJavaFrame frame;
	JP_TRACE_IN("JPArray::getRange");
	JPClass* compType = m_Class->getComponentType();
	JP_TRACE("Component type", compType->getCanonicalName());
	JP_TRACE("Range", start, stop);

	// Python truncates the request to fit the array range
	if (stop > m_Length)
		stop = m_Length;

	if (m_Object.get() == NULL || start >= stop)
	{
		// Python behavior would be to return an empty list
		return JPPyList::newList(0);
	}

	return compType->getArrayRange(frame, m_Object.get(), start, stop - start);
	JP_TRACE_OUT;
}

void JPArray::setRange(size_t start, size_t stop, PyObject* val)
{
	JP_TRACE_IN("JPArray::setRange");
	JPJavaFrame frame;
	JPClass* compType = m_Class->getComponentType();
	unsigned int len = stop - start;
	JPPySequence seq(JPPyRef::_use, val);
	size_t plength = seq.size();

	JP_TRACE("Verify lengths", len, plength);
	if (len != plength)
	{
		// Python would allow mismatching size by growing or shrinking 
		// the length of the array.  But java arrays are immutable in length.
		std::stringstream out;
		out << "Slice assignment must be of equal lengths : " << len << " != " << plength;
		JP_RAISE_RUNTIME_ERROR(out.str());
	}

	JP_TRACE("Call component set range");
	compType->setArrayRange(frame, m_Object.get(), start, len, val);
	JP_TRACE_OUT;
}

void JPArray::setItem(size_t ndx, PyObject* val)
{
	JPJavaFrame frame;
	JPClass* compType = m_Class->getComponentType();
	if (ndx > m_Length)
	{
		// Python returns IndexError
		stringstream ss;
		ss << "java array assignment index out of range for size " << m_Length;
		JP_RAISE_INDEX_ERROR(ss.str());
	}

	if (compType->canConvertToJava(val) <= _explicit)
	{
		JP_RAISE_RUNTIME_ERROR("Unable to convert.");
	}

	compType->setArrayItem(frame, m_Object.get(), ndx, val);
}

JPPyObject JPArray::getItem(size_t ndx)
{
	JPJavaFrame frame;
	JPClass* compType = m_Class->getComponentType();

	if (ndx > m_Length)
	{
		// Python behavior is IndexError
		stringstream ss;
		ss << "index " << ndx << "is out of bounds for java array with size 0";
		JP_RAISE_INDEX_ERROR(ss.str());
	}

	return compType->getArrayItem(frame, m_Object.get(), ndx);
}

JPClass* JPArray::getType()
{
	return m_Class;
}

jvalue JPArray::getValue()
{
	jvalue val;
	val.l = m_Object.get();
	return val;
}
