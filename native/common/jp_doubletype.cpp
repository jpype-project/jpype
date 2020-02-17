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
#include "jp_doubletype.h"

JPDoubleType::JPDoubleType() : JPPrimitiveType(JPTypeManager::_java_lang_Double)
{
}

JPDoubleType::~JPDoubleType()
{
}

bool JPDoubleType::isSubTypeOf(JPClass* other) const
{
	return other == JPTypeManager::_double;
}

JPPyObject JPDoubleType::convertToPythonObject(jvalue val)
{
	return JPPyFloat::fromDouble(field(val));
}

JPValue JPDoubleType::getValueFromObject(jobject obj)
{
	jvalue v;
	field(v) = JPJni::doubleValue(obj);
	return JPValue(this, v);
}

JPMatch::Type JPDoubleType::canConvertToJava(PyObject* obj)
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
		return JPMatch::_exact;
	}

	// Java allows conversion to any type with a longer range even if lossy
	// Does it quack?
	if (JPPyFloat::checkConvertable(obj))
	{
		return JPMatch::_implicit;
	}

	return JPMatch::_none;
}

jvalue JPDoubleType::convertToJava(PyObject* obj)
{
	JP_TRACE_IN("JPDoubleType::convertToJava");
	jvalue res;
	field(res) = 0;
	JPValue* value = PyJPValue_getJavaSlot(obj);
	if (value != NULL)
	{
		if (value->getClass() == this)
		{
			return *value;
		}
		if (value->getClass() == m_BoxedClass)
		{
			return getValueFromObject(value->getJavaObject());
		}
		JP_RAISE(PyExc_TypeError, "Cannot convert value to Java double");
	} else if (PyFloat_Check(obj))
	{
		field(res) = (type_t) JPPyFloat::asDouble(obj);
		return res;
	} else if (JPPyLong::check(obj))
	{
		field(res) = (type_t) JPPyLong::asLong(obj);
		return res;
	} else if (PyObject_HasAttrString(obj, "__float__"))
	{
		field(res) = (type_t) JPPyFloat::asDouble(obj);
		return res;
	}

	JP_RAISE(PyExc_TypeError, "Cannot convert value to Java double");
	return res;
	JP_TRACE_OUT;
}

jarray JPDoubleType::newArrayInstance(JPJavaFrame& frame, jsize sz)
{
	return frame.NewDoubleArray(sz);
}

JPPyObject JPDoubleType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticDoubleField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPDoubleType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetDoubleField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPDoubleType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticDoubleMethodA(claz, mth, val);
	}
	return convertToPythonObject(v);
}

JPPyObject JPDoubleType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == NULL)
			field(v) = frame.CallDoubleMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualDoubleMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(v);
}

void JPDoubleType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticDoubleField(c, fid, val);
}

void JPDoubleType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetDoubleField(c, fid, val);
}

void JPDoubleType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPDoubleType::setArrayRange");
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetDoubleArrayElements, &JPJavaFrame::ReleaseDoubleArrayElements);

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
			jconverter conv = getConverter(view.format, view.itemsize, "d");
			for (Py_ssize_t i = 0; i < length; ++i, index += step)
			{
				jvalue r = conv(memory);
				val[index] = r.d;
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
		type_t v = (type_t) PyFloat_AsDouble(seq[i].get());
		if (v == -1. && JPPyErr::occurred())
		{
			JP_RAISE_PYTHON("JPDoubleType::setArrayRange");
		}
		val[index] = v;
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPDoubleType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetDoubleArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(v);
}

void JPDoubleType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	array_t array = (array_t) a;
	type_t val = field(convertToJava(obj));
	frame.SetDoubleArrayRegion(array, ndx, 1, &val);
}

void JPDoubleType::getView(JPArrayView& view)
{
	JPJavaFrame frame;
	view.memory = (void*) frame.GetDoubleArrayElements(
			(jdoubleArray) view.array->getJava(), &view.isCopy);
	view.buffer.format = "d";
	view.buffer.itemsize = sizeof (jdouble);
}

void JPDoubleType::releaseView(JPArrayView& view)
{
	JPJavaFrame frame;
	frame.ReleaseDoubleArrayElements((jdoubleArray) view.array->getJava(),
			(jdouble*) view.memory, view.buffer.readonly ? JNI_ABORT : 0);
}

const char* JPDoubleType::getBufferFormat()
{
	return "d";
}

ssize_t JPDoubleType::getItemSize()
{
	return sizeof (jdouble);
}

void JPDoubleType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	jdouble* b = (jdouble*) ((char*) memory + offset);
	frame.GetDoubleArrayRegion((jdoubleArray) a, start, len, b);
}
