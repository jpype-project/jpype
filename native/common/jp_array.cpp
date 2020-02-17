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
	{
		PyErr_Format(PyExc_TypeError,
				"Java array assignments must be sequences, not '%s'", Py_TYPE(val)->tp_name);
		JP_RAISE_PYTHON("fail");
	}

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
	{
		PyErr_Format(PyExc_TypeError, "Unable to convert '%s' int Java '%s'",
				Py_TYPE(val)->tp_name,
				this->getClass()->getComponentType()->getCanonicalName().c_str());
		JP_RAISE_PYTHON("fail");
	}

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

int JPArray::checkIsPrimitive(int &dims)
{
	dims = 0;
	JPClass* cls = this->getClass();
	while (dynamic_cast<JPArrayClass*> (cls) != 0)
	{
		dims++;
		cls = ((JPArrayClass*) cls)->getComponentType();
	}
	if (!cls->isPrimitive())
		return -1;
	return 0;
}

int JPArray::checkRectangular(int &dimsize0, int &dimsize1)
{
	JPJavaFrame frame;
	// Get the first dimension
	dimsize0 = getLength();
	if (dimsize0 == 0)
		return -1;

	// Get the second dimension
	jobjectArray a = (jobjectArray) getJava();
	jobject u = frame.GetObjectArrayElement(a, m_Start);
	if (u == 0)
		return -1;
	dimsize1 = frame.GetArrayLength((jarray) u);
	frame.DeleteLocalRef(u);

	for (int i = 1; i < dimsize0; ++i)
	{
		int j = m_Start + i*m_Step;
		jobject u2 = frame.GetObjectArrayElement(a, j);
		if (u2 == 0)
			return -1;
		jint s = frame.GetArrayLength((jarray) u2);
		if (s != dimsize1)
			return -1;
		frame.DeleteLocalRef(u2);
	}
	return 0;
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
	JPPrimitiveType *type = (JPPrimitiveType*) array->getClass()->getComponentType();
	type->getView(*this);
	strides[0] = buffer.itemsize * array->m_Step;
	shape[0] = array->m_Length;
	buffer.buf = (char*) memory + buffer.itemsize * array->m_Start;
	buffer.len = array->m_Length * buffer.itemsize;
	buffer.shape = shape;
	buffer.strides = strides;
	buffer.readonly = 1;
	owned = false;
}

JPArrayView::JPArrayView(JPArray* array, int dimsize0, int dimsize1)
{
	JPJavaFrame frame;
	this->array = array;
	refcount = 0;
	buffer.obj = NULL;
	buffer.ndim = 2;
	buffer.suboffsets = NULL;
	JPArrayClass *cls = array->m_Class;
	cls = (JPArrayClass*) cls->getComponentType();
	JPPrimitiveType *cls2 = (JPPrimitiveType*) cls->getComponentType();
	ssize_t itemsize = cls2->getItemSize();
	memory = new char[dimsize0 * dimsize1 * itemsize];
	int offset = 0;
	jobjectArray a = (jobjectArray) array->getJava();
	for (int i = 0; i < dimsize0; i++)
	{
		int j = array->m_Start + i * array->m_Step;
		jarray a1 = (jarray) frame.GetObjectArrayElement(a, j);
		cls2->copyElements(frame, a1, memory, offset);
		offset += itemsize*dimsize1;
		frame.DeleteLocalRef(a1);
	}
	buffer.itemsize = itemsize;
	strides[0] = buffer.itemsize * dimsize1;
	strides[1] = buffer.itemsize;
	shape[0] = dimsize0;
	shape[1] = dimsize1;
	buffer.format = const_cast<char*> (cls2->getBufferFormat());
	buffer.buf = (char*) memory + buffer.itemsize * array->m_Start;
	buffer.len = dimsize0 * dimsize1 * buffer.itemsize;
	buffer.shape = shape;
	buffer.strides = strides;
	buffer.readonly = 1;
	owned = true;
}

JPArrayView::~JPArrayView()
{
	if (owned)
		delete (char*) memory;
}

void JPArrayView::reference()
{
	refcount++;
}

bool JPArrayView::unreference()
{
	refcount--;
	JPPrimitiveType *type = (JPPrimitiveType*) array->getClass()->getComponentType();
	if (refcount == 0 && !owned)
		type->releaseView(*this);
	return refcount == 0;
}
