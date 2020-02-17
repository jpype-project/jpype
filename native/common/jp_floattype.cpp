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
#include "jp_floattype.h"

JPFloatType::JPFloatType() : JPPrimitiveType(JPTypeManager::_java_lang_Float)
{
}

JPFloatType::~JPFloatType()
{
}

bool JPFloatType::isSubTypeOf(JPClass* other) const
{
	return other == JPTypeManager::_float
			|| other == JPTypeManager::_double;
}

JPPyObject JPFloatType::convertToPythonObject(jvalue val)
{
	return JPPyFloat::fromFloat(field(val));
}

JPValue JPFloatType::getValueFromObject(jobject obj)
{
	jvalue v;
	field(v) = (type_t) JPJni::doubleValue(obj);
	return JPValue(this, v);
}

JPMatch::Type JPFloatType::canConvertToJava(PyObject* obj)
{
	ASSERT_NOT_NULL(obj);
	if (JPPyObject::isNone(obj))
	{
		return JPMatch::_none;
	}

	JPValue* value = PyJPValue_getJavaSlot(obj);
	if (value != NULL)
	{
		if (value->getClass() == this)
		{
			return JPMatch::_exact;
		}

		if (value->getClass() == m_BoxedClass)
		{
			return JPMatch::_implicit;
		}

		// Java does not permit boxed to boxed conversions.
		return JPMatch::_none;
	}

	if (PyFloat_Check(obj))
	{
		// This next line is a puzzle.  It seems like it should be JPMatch::_exact.
		return JPMatch::_implicit;
	}

	// Java allows conversion to any type with a longer range even if lossy
	if (JPPyFloat::checkConvertable(obj))
	{
		return JPMatch::_implicit;
	}

	return JPMatch::_none;
}

jvalue JPFloatType::convertToJava(PyObject* obj)
{
	JP_TRACE_IN("JPFloatType::convertToJava");
	jvalue res;
	JPValue* value = PyJPValue_getJavaSlot(obj);
	if (value != NULL)
	{
		if (value->getClass() == this)
		{
			return value->getValue();
		}

		if (value->getClass() == m_BoxedClass)
		{
			return getValueFromObject(value->getJavaObject());
		}

		JP_RAISE(PyExc_OverflowError, "Cannot convert value to Java float");
	} else if (JPPyFloat::checkConvertable(obj))
	{
		double l = JPPyFloat::asDouble(obj);
		// FIXME the check for s_minFloat seems wrong.
		// Java would trim to 0 rather than giving an error.
		if (l >= 0 && l > JPJni::s_Float_Max)
		{
			JP_RAISE(PyExc_OverflowError, "Cannot convert value to Java float");
		} else if (l < 0 && l < -JPJni::s_Float_Max)
		{
			JP_RAISE(PyExc_OverflowError, "Cannot convert value to Java float");
		}
		res.f = (jfloat) l;
		return res;
	}// We should never reach here as an int because we should
		// have hit the float conversion.  But we are leaving it for the odd
		// duck with __int__ but no __float__
	else if (JPPyLong::checkConvertable(obj))
	{
		field(res) = (type_t) JPPyLong::asLong(obj);
		return res;
	}

	JP_RAISE(PyExc_TypeError, "Cannot convert value to Java float");
	return res;
	JP_TRACE_OUT;
}

jarray JPFloatType::newArrayInstance(JPJavaFrame& frame, jsize sz)
{
	return frame.NewFloatArray(sz);
}

JPPyObject JPFloatType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticFloatField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPFloatType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetFloatField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPFloatType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticFloatMethodA(claz, mth, val);
	}
	return convertToPythonObject(v);
}

JPPyObject JPFloatType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == NULL)
			field(v) = frame.CallFloatMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualFloatMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(v);
}

void JPFloatType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticFloatField(c, fid, val);
}

void JPFloatType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetFloatField(c, fid, val);
}

void JPFloatType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPFloatType::setArrayRange");
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetFloatArrayElements, &JPJavaFrame::ReleaseFloatArrayElements);

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
			jconverter conv = getConverter(view.format, view.itemsize, "f");
			for (Py_ssize_t i = 0; i < length; ++i, index += step)
			{
				jvalue r = conv(memory);
				val[index] = r.f;
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
		double v =  PyFloat_AsDouble(seq[i].get());
		if (v == -1. && JPPyErr::occurred())
		{
			JP_RAISE_PYTHON("JPFloatType::setArrayRange");
		}
		val[index] = (type_t) assertRange(v);
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPFloatType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetFloatArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(v);
}

void JPFloatType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	array_t array = (array_t) a;
	type_t val = field(convertToJava(obj));
	frame.SetFloatArrayRegion(array, ndx, 1, &val);
}

void JPFloatType::getView(JPArrayView& view)
{
	JPJavaFrame frame;
	view.memory = (void*) frame.GetFloatArrayElements(
			(jfloatArray) view.array->getJava(), &view.isCopy);
	view.buffer.format = "f";
	view.buffer.itemsize = sizeof (jfloat);
}

void JPFloatType::releaseView(JPArrayView& view)
{
	JPJavaFrame frame;
	frame.ReleaseFloatArrayElements((jfloatArray) view.array->getJava(),
			(jfloat*) view.memory, view.buffer.readonly ? JNI_ABORT : 0);
}

const char* JPFloatType::getBufferFormat()
{
	return "f";
}

ssize_t JPFloatType::getItemSize()
{
	return sizeof (jfloat);
}

void JPFloatType::copyElements(JPJavaFrame &frame, jarray a, void* memory, int offset)
{
	int len = frame.GetArrayLength(a);
	jfloat* b = (jfloat*) ((char*) memory + offset);
	frame.GetFloatArrayRegion((jfloatArray) a, 0, len, b);
}

