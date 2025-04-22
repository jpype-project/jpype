/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#include "jpype.h"
#include "pyjp.h"
#include "jp_array.h"
#include "jp_arrayclass.h"
#include "jp_primitive_accessor.h"

// Note: java represents arrays of zero length as null, thus we
// need to be careful to handle these properly.  We need to
// carry them around so that we can match types.

JPArray::JPArray(const JPValue &value)
: m_Object((jarray) value.getValue().l)
{
	m_Class = dynamic_cast<JPArrayClass*>( value.getClass());
	JPJavaFrame frame = JPJavaFrame::outer();
	JP_TRACE_IN("JPArray::JPArray");
	ASSERT_NOT_NULL(m_Class);
	JP_TRACE(m_Class->toString());

	// We will use this during range checks, so cache it
	if (m_Object.get() == nullptr)
		m_Length = 0;  // GCOVR_EXCL_LINE
	else
		m_Length = frame.GetArrayLength(m_Object.get());

	m_Step = 1;
	m_Start = 0;
	m_Slice = false;

	JP_TRACE_OUT;
}

JPArray::JPArray(JPArray* instance, jsize start, jsize stop, jsize step)
: m_Object((jarray) instance->getJava())
{
	JP_TRACE_IN("JPArray::JPArraySlice");
	m_Class = instance->m_Class;
	m_Step = step * instance->m_Step;
	m_Start = instance->m_Start + instance->m_Step*start;
	if (step > 0)
		m_Length =  (stop - start - 1 + step) / step;
	else
		m_Length =  (stop - start + 1 + step) / step;
	if (m_Length < 0)
		m_Length = 0;  // GCOVR_EXCL_LINE
	m_Slice = true;
	JP_TRACE_OUT;
}

JPArray::~JPArray()
= default;

jsize JPArray::getLength() const
{
	return m_Length;
}

void JPArray::setRange(jsize start, jsize length, jsize step, PyObject* val)
{
	JP_TRACE_IN("JPArray::setRange");

	// Make sure it is an iterable before we start
	if (!PySequence_Check(val))
		JP_RAISE(PyExc_TypeError, "can only assign a sequence");

	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass* compType = m_Class->getComponentType();
	JPPySequence seq = JPPySequence::use(val);
	long plength = (long) seq.size();

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
	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass* compType = m_Class->getComponentType();

	if (ndx < 0)
		ndx += m_Length;

	if (ndx >= m_Length || ndx < 0)
		JP_RAISE(PyExc_IndexError, "java array assignment out of bounds");

	compType->setArrayItem(frame, m_Object.get(), m_Start + ndx*m_Step, val);
}

JPPyObject JPArray::getItem(jsize ndx)
{
	JPJavaFrame frame = JPJavaFrame::outer();
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
	JPValue value = m_Class->newArray(frame, m_Length);
	JPClass* compType = m_Class->getComponentType();
	auto out = (jarray) value.getValue().l;
	compType->setArrayRange(frame, out, 0, m_Length, 1, obj);
	return out;
}

JPArrayView::JPArrayView(JPArray* array)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	m_Array = array;
	m_RefCount = 0;
	m_Buffer.obj = nullptr;
	m_Buffer.ndim = 1;
	m_Buffer.suboffsets = nullptr;
	auto *type = dynamic_cast<JPPrimitiveType*>( array->getClass()->getComponentType());
	type->getView(*this);
	m_Strides[0] = m_Buffer.itemsize * array->m_Step;
	m_Shape[0] = array->m_Length;
	m_Buffer.buf = (char*) m_Memory + m_Buffer.itemsize * array->m_Start;
	m_Buffer.len = array->m_Length * m_Buffer.itemsize;
	m_Buffer.shape = m_Shape;
	m_Buffer.strides = m_Strides;
	m_Buffer.readonly = 1;
	m_Owned = false;
}

JPArrayView::JPArrayView(JPArray* array, jobject collection)
{
	JP_TRACE_IN("JPArrayView::JPArrayView");
	// All of the work has already been done by org.jpype.Utilities
	JPJavaFrame frame = JPJavaFrame::outer();
	m_Array = array;

	jint len = frame.GetArrayLength((jarray) collection);
	jobject item0 = frame.GetObjectArrayElement((jobjectArray) collection, 0);
	jobject item1 = frame.GetObjectArrayElement((jobjectArray) collection, 1);

	// First element is the primitive type that we are packing the array from
	auto *componentType = dynamic_cast<JPPrimitiveType*>(
			frame.findClass((jclass) item0));

	// Second element is the shape of the array from which we compute the
	// memory size, the shape, and strides
	int dims;
	Py_ssize_t itemsize;
	Py_ssize_t sz;
	{
		JPPrimitiveArrayAccessor<jintArray, jint*> accessor(frame, (jintArray) item1,
				&JPJavaFrame::GetIntArrayElements, &JPJavaFrame::ReleaseIntArrayElements);
		jint* shape2 = accessor.get();
		dims = frame.GetArrayLength((jarray) item1);
		itemsize = componentType->getItemSize();
		sz = itemsize;
		for (int i = 0; i < dims; ++i)
		{
			m_Shape[i] = shape2[i];
			sz *= m_Shape[i];
		}
		accessor.abort();
	}
	Py_ssize_t stride = itemsize;
	for (int i = 0; i < dims; ++i)
	{
		int n = dims - 1 - i;
		m_Strides[n] = stride;
		stride *= m_Shape[n];
	}

	m_RefCount = 0;
	m_Memory = new char[sz];
	m_Owned = true;

	// All remaining elements are primitive arrays to be unpacked
	int offset = 0;
	Py_ssize_t last = m_Shape[dims - 1];
	for (Py_ssize_t i = 0; i < len - 2; i++)
	{
		auto a1 = (jarray) frame.GetObjectArrayElement((jobjectArray) collection, (jsize) i + 2);
		componentType->copyElements(frame, a1, 0, (jsize) last, m_Memory, offset);
		offset += (int) (itemsize * last);
		frame.DeleteLocalRef(a1);
	}

	// Copy values into Python buffer for consumption
	m_Buffer.obj = nullptr;
	m_Buffer.ndim = dims;
	m_Buffer.suboffsets = nullptr;
	m_Buffer.itemsize = itemsize;
	m_Buffer.format = const_cast<char*> (componentType->getBufferFormat());
	m_Buffer.buf = (char*) m_Memory + m_Buffer.itemsize * array->m_Start;
	m_Buffer.len = sz;
	m_Buffer.shape = m_Shape;
	m_Buffer.strides = m_Strides;
	m_Buffer.readonly = 1;
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}

JPArrayView::~JPArrayView()
{
	if (m_Owned)
		delete [] (char*) m_Memory;
}

void JPArrayView::reference()
{
	m_RefCount++;
}

bool JPArrayView::unreference()
{
	m_RefCount--;
	auto *type = dynamic_cast<JPPrimitiveType*>( m_Array->getClass()->getComponentType());
	if (m_RefCount == 0 && !m_Owned)
		type->releaseView(*this);
	return m_RefCount == 0;
}
