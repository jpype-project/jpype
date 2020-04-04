/*****************************************************************************
   Copyright 2004-2008 Steve MÃ©nard

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
#include "jp_shorttype.h"
#include "pyjp.h"

JPShortType::JPShortType()
: JPPrimitiveType("short")
{
}

JPShortType::~JPShortType()
{
}

JPPyObject JPShortType::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast)
{
	JPPyObject tmp = JPPyObject(JPPyRef::_call, PyLong_FromLong(field(val)));
	JPPyObject out = JPPyObject(JPPyRef::_call, convertLong(getHost(), (PyLongObject*) tmp.get()));
	PyJPValue_assignJavaSlot(frame, out.get(), JPValue(this, val));
	return out;
}

JPValue JPShortType::getValueFromObject(const JPValue& obj)
{
	JPContext *context = obj.getClass()->getContext();
	JPJavaFrame frame(context);
	jvalue v;
	field(v) = frame.CallShortMethodA(obj.getValue().l, context->m_ShortValueID, 0);
	return JPValue(this, v);
}

JPConversionLong<JPShortType> shortConversion;
JPConversionLongNumber<JPShortType> shortNumberConversion;
JPConversionLongWiden<JPShortType> shortWidenConversion;

JPMatch::Type JPShortType::findJavaConversion(JPMatch &match)
{
	JP_TRACE_IN("JPShortType::getJavaConversion");

	if (match.object == Py_None)
		return match.type = JPMatch::_none;

	JPValue* value = match.getJavaSlot();
	if (value != NULL)
	{
		JPClass *cls = value->getClass();
		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (javaValueConversion->matches(match, this)
				|| unboxConversion->matches(match, this))
			return match.type;

		// Consider widening
		if (cls->isPrimitive())
		{
			// https://docs.oracle.com/javase/specs/jls/se7/html/jls-5.html#jls-5.1.2
			JPPrimitiveType *prim = (JPPrimitiveType*) cls;
			switch (prim->getTypeCode())
			{
				case 'C':
				case 'B':
					match.conversion = &shortWidenConversion;
					return match.type = JPMatch::_implicit;
				default:
					return match.type = JPMatch::_none;
			}
		}

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return match.type = JPMatch::_none;
	}

	if (shortConversion.matches(match, this)
			|| shortNumberConversion.matches(match, this))
		return match.type;

	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

jarray JPShortType::newArrayInstance(JPJavaFrame& frame, jsize sz)
{
	return frame.NewShortArray(sz);
}

JPPyObject JPShortType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticShortField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPShortType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetShortField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPShortType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticShortMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPShortType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == NULL)
			field(v) = frame.CallShortMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualShortMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

void JPShortType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java short");
	type_t val = field(match.convert());
	frame.SetStaticShortField(c, fid, val);
}

void JPShortType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java short");
	type_t val = field(match.convert());
	frame.SetShortField(c, fid, val);
}

void JPShortType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPShortType::setArrayRange");
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetShortArrayElements, &JPJavaFrame::ReleaseShortArrayElements);

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
			jconverter conv = getConverter(view.format, (int) view.itemsize, "s");
			for (Py_ssize_t i = 0; i < length; ++i, index += step)
			{
				jvalue r = conv(memory);
				val[index] = r.s;
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

JPPyObject JPShortType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetShortArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(frame, v, false);
}

void JPShortType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java short");
	type_t val = field(match.convert());
	frame.SetShortArrayRegion((array_t) a, ndx, 1, &val);
}

void JPShortType::getView(JPArrayView& view)
{
	JPJavaFrame frame(view.getContext());
	view.m_Memory = (void*) frame.GetShortArrayElements(
			(jshortArray) view.m_Array->getJava(), &view.m_IsCopy);
	view.m_Buffer.format = "h";
	view.m_Buffer.itemsize = sizeof (jshort);
}

void JPShortType::releaseView(JPArrayView& view)
{
	try
	{
		JPJavaFrame frame(view.getContext());
		frame.ReleaseShortArrayElements((jshortArray) view.m_Array->getJava(),
				(jshort*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (JPypeException&)
	{
		// This is called as part of the cleanup routine and exceptions
		// are not permitted
	}
}

const char* JPShortType::getBufferFormat()
{
	return "h";
}

ssize_t JPShortType::getItemSize()
{
	return sizeof (jshort);
}

void JPShortType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	jshort* b = (jshort*) ((char*) memory + offset);
	frame.GetShortArrayRegion((jshortArray) a, start, len, b);
}

static void pack(jshort* d, jvalue v)
{
	*d = v.s;
}

PyObject *JPShortType::newMultiArray(JPJavaFrame &frame, JPPyBuffer &buffer, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPShortType::newMultiArray");
	return convertMultiArray<type_t>(
			frame, this, &pack, "s",
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}
