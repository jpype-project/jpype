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
#include "jpype.h"
#include "jp_arrayclass.h"

// Note: java represents arrays of zero length as null, thus we
// need to be careful to handle these properly.  We need to
// carry them around so that we can match types.

JPArray::JPArray(const JPValue& val) : m_Object((jarray) val.getValue().l)
{
	JPJavaFrame frame;
	JP_TRACE_IN("JPArray::JPArray");
	m_Class = (JPArrayClass*) val.getClass();
	ASSERT_NOT_NULL(m_Class);
	JP_TRACE(m_Class->toString());

	// We will use this during range checks, so cache it
	if (m_Object.get() == NULL)
		m_Length = 0;
	else
		m_Length = frame.GetArrayLength(m_Object.get());

	m_Step = 1;
	m_Start = 0;
	m_Slice = false;

	JP_TRACE_OUT;
}

JPArray::JPArray(JPArray* instance, jsize start, jsize stop, jsize step)
: m_Object(instance->getJava())
{
	JP_TRACE_IN("JPArray::JPArray");
	m_Class = instance->m_Class;
	m_Step = step * instance->m_Step;
	m_Start = instance->m_Start + instance->m_Step*start;
	if (step > 0)
		m_Length =  (stop - start - 1 + step) / step;
	else
		m_Length =  (stop - start + 1 + step) / step;
	if (m_Length < 0)
		m_Length = 0;
	m_Slice = true;
	JP_TRACE_OUT;
}

JPArray::~JPArray()
{
}

jsize JPArray::getLength()
{
	return m_Length;
}

void JPArray::setRange(jsize start, jsize length, jsize step, PyObject* val)
{
	JP_TRACE_IN("JPArray::setRange");

	// Make sure it is an iterable before we start
	if (!PySequence_Check(val))
		JP_RAISE(PyExc_TypeError, "can only assign a sequence");

	JPJavaFrame frame;
	JPClass* compType = m_Class->getComponentType();
	JPPySequence seq(JPPyRef::_use, val);
	long plength = seq.size();

	JP_TRACE("Verify lengths", length, plength);
	if ((long) length != plength)
	{
		// Python would allow mismatching size by growing or shrinking
		// the length of the array.  But java arrays are immutable in length.
		std::stringstream out;
		out << "Slice assignment must be of equal lengths : " << length << " != " << plength;
		JP_RAISE(PyExc_ValueError, out.str());
	}

	JP_TRACE("Call component set range");
	jsize i0 = m_Start + m_Step*start;
	compType->setArrayRange(frame, m_Object.get(), i0, length, m_Step*step, val);
	JP_TRACE_OUT;
}

void JPArray::setItem(jsize ndx, PyObject* val)
{
	JPJavaFrame frame;
	JPClass* compType = m_Class->getComponentType();

	if (ndx < 0)
		ndx += m_Length;

	if (ndx >= m_Length || ndx < 0)
		JP_RAISE(PyExc_IndexError, "java array assignment out of bounds");

	if (compType->canConvertToJava(val) <= JPMatch::_explicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert.");

	compType->setArrayItem(frame, m_Object.get(), m_Start + ndx*m_Step, val);
}

JPPyObject JPArray::getItem(jsize ndx)
{
	JPJavaFrame frame;
	JPClass* compType = m_Class->getComponentType();

	if (ndx < 0)
		ndx += m_Length;

	if (ndx >= m_Length || ndx < 0)
	{
		JP_RAISE(PyExc_IndexError, "array index out of bounds");
	}

	return compType->getArrayItem(frame, m_Object.get(), m_Start + ndx * m_Step);
}

jarray JPArray::clone(JPJavaFrame& frame, PyObject* obj)
{
	JPValue value = m_Class->newInstance(frame, m_Length);
	JPClass* compType = m_Class->getComponentType();
	jarray out = (jarray) value.getValue().l;
	compType->setArrayRange(frame, out, 0, m_Length, 1, obj);
	return out;
}

JPArrayView::JPArrayView(JPArray* array)
{
	JPJavaFrame frame;
	this->array = array;
	refcount = 0;
	buffer.obj = NULL;
	buffer.ndim = 1;
	buffer.suboffsets = NULL;
	array->getClass()->getComponentType()->getView(*this);
	strides[0] = buffer.itemsize * array->m_Step;
	shape[0] = array->m_Length;
	buffer.buf = (char*) memory + buffer.itemsize * array->m_Start;
	buffer.len = array->m_Length * buffer.itemsize;
	buffer.shape = shape;
	buffer.strides = strides;
	buffer.readonly = 1;
}

void JPArrayView::reference()
{
	refcount++;
}

bool JPArrayView::unreference()
{
	refcount--;
	JP_TRACE("REF COUNT", refcount);
	array->getClass()->getComponentType()->releaseView(*this, refcount == 0);
	return refcount == 0;
}
