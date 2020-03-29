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
#include "jp_booleantype.h"
#include "pyjp.h"

JPBooleanType::JPBooleanType()
: JPPrimitiveType("boolean")
{
}

JPBooleanType::~JPBooleanType()
{
}

JPPyObject JPBooleanType::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast)
{
	return JPPyObject(JPPyRef::_call, PyBool_FromLong(val.z));
}

JPValue JPBooleanType::getValueFromObject(const JPValue& obj)
{
	JPContext *context = obj.getClass()->getContext();
	JPJavaFrame frame(context);
	jvalue v;
	field(v) = frame.CallBooleanMethodA(obj.getValue().l, context->m_BooleanValueID, 0) ? true : false;
	return JPValue(this, v);
}

class JPConversionAsBoolean : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls)
	{
		if (!PyLong_CheckExact(match.object)
				&& !PyIndex_Check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		jvalue res;
		jlong v = PyLong_AsLongLong(match.object);
		if (v == -1)
			JP_PY_CHECK();
		res.z = v != 0;
		return res;
	}
} asBooleanConversion;

JPMatch::Type JPBooleanType::findJavaConversion(JPMatch &match)
{
	JP_TRACE_IN("JPBooleanType::getJavaConversion", this);

	if (match.object ==  Py_None)
		return match.type = JPMatch::_none;

	if (PyBool_Check(match.object))
	{
		match.conversion = &asBooleanConversion;
		return match.type = JPMatch::_exact;
	}

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

	if (asBooleanConversion.matches(match, this))
		return match.type;

	if (PyNumber_Check(match.object))
	{
		match.conversion = &asBooleanConversion;
		return match.type = JPMatch::_explicit;
	}

	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

jarray JPBooleanType::newArrayInstance(JPJavaFrame& frame, jsize sz)
{
	return frame.NewBooleanArray(sz);
}

JPPyObject JPBooleanType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticBooleanField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPBooleanType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetBooleanField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPBooleanType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticBooleanMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPBooleanType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == NULL)
			field(v) = frame.CallBooleanMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualBooleanMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

void JPBooleanType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java boolean");
	type_t val = field(match.convert());
	frame.SetStaticBooleanField(c, fid, val);
}

void JPBooleanType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java boolean");
	type_t val = field(match.convert());
	frame.SetBooleanField(c, fid, val);
}

void JPBooleanType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPBooleanType::setArrayRange");
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetBooleanArrayElements, &JPJavaFrame::ReleaseBooleanArrayElements);

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
			jconverter conv = getConverter(view.format, (int) view.itemsize, "z");
			for (Py_ssize_t i = 0; i < length; ++i, index += step)
			{
				jvalue r = conv(memory);
				val[index] = r.z;
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
		int v = PyObject_IsTrue(seq[i].get());
		if (v == -1)
			JP_PY_CHECK();
		val[index] = v;
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPBooleanType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetBooleanArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(frame, v, false);
}

void JPBooleanType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java boolean");
	type_t val = field(match.convert());
	frame.SetBooleanArrayRegion((array_t) a, ndx, 1, &val);
}

void JPBooleanType::getView(JPArrayView& view)
{
	JPJavaFrame frame(view.getContext());
	view.m_Memory = (void*) frame.GetBooleanArrayElements(
			(jbooleanArray) view.m_Array->getJava(), &view.m_IsCopy);
	view.m_Buffer.format = "?";
	view.m_Buffer.itemsize = sizeof (jboolean);
}

void JPBooleanType::releaseView(JPArrayView& view)
{
	try
	{
		JPJavaFrame frame(view.getContext());
		frame.ReleaseBooleanArrayElements((jbooleanArray) view.m_Array->getJava(),
				(jboolean*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (JPypeException&)
	{
		// This is called as part of the cleanup routine and exceptions
		// are not permitted
	}
}

const char* JPBooleanType::getBufferFormat()
{
	return "?";
}

ssize_t JPBooleanType::getItemSize()
{
	return sizeof (jboolean);
}

void JPBooleanType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	jboolean* b = (jboolean*) ((char*) memory + offset);
	frame.GetBooleanArrayRegion((jbooleanArray) a, start, len, b);
}

static void pack(jboolean* d, jvalue v)
{
	*d = v.z;
}

PyObject *JPBooleanType::newMultiArray(JPJavaFrame &frame, JPPyBuffer &buffer, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPBooleanType::newMultiArray");
	return convertMultiArray<type_t>(
			frame, this, &pack, "z",
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}
