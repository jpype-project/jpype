/*****************************************************************************
   Copyright 2004-2008 Steve Ménard

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
#include "jp_primitive_accessor.h"
#include "jp_bytetype.h"
#include "pyjp.h"

JPByteType::JPByteType()
: JPPrimitiveType("byte")
{
}

JPByteType::~JPByteType()
{
}

JPPyObject JPByteType::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast)
{
	JPPyObject tmp = JPPyObject(JPPyRef::_call, PyLong_FromLong(field(val)));
	JPPyObject out = JPPyObject(JPPyRef::_call, convertLong(getHost(), (PyLongObject*) tmp.get()));
	PyJPValue_assignJavaSlot(frame, out.get(), JPValue(this, val));
	return out;
}

JPValue JPByteType::getValueFromObject(const JPValue& obj)
{
	JPContext *context = obj.getClass()->getContext();
	JPJavaFrame frame(context);
	jvalue v;
	field(v) = frame.CallByteMethodA(obj.getValue().l, context->m_ByteValueID, 0);
	return JPValue(this, v);
}

JPConversionLong<JPByteType> byteConversion;
JPConversionLongNumber<JPByteType> byteNumberConversion;

JPMatch::Type JPByteType::findJavaConversion(JPMatch &match)
{
	JP_TRACE_IN("JPByteType::getJavaConversion");

	if (match.object == Py_None)
		return match.type = JPMatch::_none;

	JPValue *value = match.getJavaSlot();
	if (value != NULL)
	{
		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (javaValueConversion->matches(match, this)
				|| unboxConversion->matches(match, this))
			return match.type;

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return match.type = JPMatch::_none;
	}

	if (byteConversion.matches(match, this)
			|| byteNumberConversion.matches(match, this))
		return match.type;

	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

jarray JPByteType::newArrayInstance(JPJavaFrame& frame, jsize sz)
{
	return frame.NewByteArray(sz);
}

JPPyObject JPByteType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticByteField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPByteType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetByteField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPByteType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticByteMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPByteType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == NULL)
			field(v) = frame.CallByteMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualByteMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

void JPByteType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java byte");
	type_t val = field(match.convert());
	frame.SetStaticByteField(c, fid, val);
}

void JPByteType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java byte");
	type_t val = field(match.convert());
	frame.SetByteField(c, fid, val);
}

void JPByteType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step, PyObject* sequence)
{
	JP_TRACE_IN("JPByteType::setArrayRange");
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetByteArrayElements, &JPJavaFrame::ReleaseByteArrayElements);

	type_t* val = accessor.get();
	// First check if assigning sequence supports buffer API
	if (PyObject_CheckBuffer(sequence))
	{
		JPPyBuffer buffer(sequence, PyBUF_FULL_RO);
		if (buffer.valid())
		{
			Py_buffer& view = buffer.getView();
			if (view.ndim != 1)
				JP_RAISE(PyExc_TypeError, "buffer dims incorrect");
			Py_ssize_t vshape = view.shape[0];
			Py_ssize_t vstep = view.strides[0];
			if (vshape != length)
				JP_RAISE(PyExc_ValueError, "mismatched size");

			char* memory = (char*) view.buf;
			if (view.suboffsets && view.suboffsets[0] >= 0)
				memory = *((char**) memory) + view.suboffsets[0];
			jsize index = start;
			jconverter conv = getConverter(view.format, (int) view.itemsize, "b");
			for (Py_ssize_t i = 0; i < length; ++i, index += step)
			{
				jvalue r = conv(memory);
				val[index] = r.b;
				memory += vstep;
			}
			accessor.commit();
			return;
		} else
		{
			PyErr_Clear();
		}
	}

	// Use sequence API
	JPPySequence seq(JPPyRef::_use, sequence);
	jsize index = start;
	for (Py_ssize_t i = 0; i < length; ++i, index += step)
	{
		jlong v = PyLong_AsLongLong(seq[i].get());
		if (v == -1)
			JP_PY_CHECK();
		val[index] = (type_t) assertRange(v);
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPByteType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetByteArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(frame, v, false);
}

void JPByteType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java byte");
	type_t val = field(match.convert());
	frame.SetByteArrayRegion((array_t) a, ndx, 1, &val);
}

void JPByteType::getView(JPArrayView& view)
{
	JPJavaFrame frame(view.getContext());
	view.m_Memory = (void*) frame.GetByteArrayElements(
			(jbyteArray) view.m_Array->getJava(), &view.m_IsCopy);
	view.m_Buffer.format = "b";
	view.m_Buffer.itemsize = sizeof (jbyte);
}

void JPByteType::releaseView(JPArrayView& view)
{
	try
	{
		JPJavaFrame frame(view.getContext());
		frame.ReleaseByteArrayElements((jbyteArray) view.m_Array->getJava(),
				(jbyte*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (JPypeException&)
	{
		// This is called as part of the cleanup routine and exceptions
		// are not permitted
	}
}

const char* JPByteType::getBufferFormat()
{
	return "b";
}

ssize_t JPByteType::getItemSize()
{
	return sizeof (jbyte);
}

void JPByteType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	jbyte* b = (jbyte*) ((char*) memory + offset);
	frame.GetByteArrayRegion((jbyteArray) a, start, len, b);
}

static void pack(jbyte* d, jvalue v)
{
	*d = v.b;
}

PyObject *JPByteType::newMultiArray(JPJavaFrame &frame, JPPyBuffer &buffer, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPByteType::newMultiArray");
	return convertMultiArray<type_t>(
			frame, this, &pack, "b",
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}
