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
#include <jpype.h>

JPShortType::JPShortType()
: JPPrimitiveType(JPTypeManager::_java_lang_Short)
{
}

JPShortType::~JPShortType()
{
}

bool JPShortType::isSubTypeOf(JPClass* other) const
{
	return other == JPTypeManager::_short
			|| other == JPTypeManager::_int
			|| other == JPTypeManager::_long
			|| other == JPTypeManager::_float
			|| other == JPTypeManager::_double;
}

JPPyObject JPShortType::convertToPythonObject(jvalue val)
{
	return JPPyObject(JPPyRef::_call, PyLong_FromLong(field(val)));
}

JPValue JPShortType::getValueFromObject(jobject obj)
{
	jvalue v;
	field(v) = JPJni::intValue(obj);
	return JPValue(this, v);
}

JPMatch::Type JPShortType::canConvertToJava(PyObject* obj)
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

		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (value->getClass() == m_BoxedClass)
		{
			return JPMatch::_implicit;
		}

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return JPMatch::_none;
	}

	if (JPPyLong::check(obj))
	{
		return JPMatch::_implicit;
	}

	if (JPPyLong::checkConvertable(obj))
	{
		// If it has integer operations then we will call it an int
		if (JPPyLong::checkIndexable(obj))
			return JPMatch::_implicit;
		else
			return JPMatch::_explicit;
	}

	return JPMatch::_none;
}

jvalue JPShortType::convertToJava(PyObject* obj)
{
	JP_TRACE_IN("JPShortType::convertToJava");
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
		JP_RAISE(PyExc_TypeError, "Cannot convert value to Java short");
	} else if (JPPyLong::checkConvertable(obj))
	{
		field(res) = (type_t) assertRange(JPPyLong::asLong(obj));
		return res;
	}

	JP_RAISE(PyExc_TypeError, "Cannot convert value to Java short");
	return res; // never reached
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
	return convertToPythonObject(v);
}

JPPyObject JPShortType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetShortField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPShortType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticShortMethodA(claz, mth, val);
	}
	return convertToPythonObject(v);
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
	return convertToPythonObject(v);
}

void JPShortType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticShortField(c, fid, val);
}

void JPShortType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
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
			jconverter conv = getConverter(view.format, view.itemsize, "s");
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
		if (v == -1 && JPPyErr::occurred())
		{
			JP_RAISE_PYTHON("JPShortType::setArrayRange");
		}
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
	return convertToPythonObject(v);
}

void JPShortType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	array_t array = (array_t) a;
	type_t val = field(convertToJava(obj));
	frame.SetShortArrayRegion(array, ndx, 1, &val);
}

void JPShortType::getView(JPArrayView& view)
{
	JPJavaFrame frame;
	view.memory = (void*) frame.GetShortArrayElements(
			(jshortArray) view.array->getJava(), &view.isCopy);
	view.buffer.format = "h";
	view.buffer.itemsize = sizeof (jshort);
}

void JPShortType::releaseView(JPArrayView& view, bool complete)
{
	JPJavaFrame frame;
	if (complete)
	{
		frame.ReleaseShortArrayElements((jshortArray) view.array->getJava(),
				(jshort*) view.memory, view.buffer.readonly ? JNI_ABORT : 0);
	}
}
