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
#include <jp_primitive_common.h>

JPLongType::JPLongType() : JPPrimitiveType(JPTypeManager::_java_lang_Long)
{
}

JPLongType::~JPLongType()
{
}

bool JPLongType::isSubTypeOf(JPClass* other) const
{
	return other == JPTypeManager::_long
			|| other == JPTypeManager::_float
			|| other == JPTypeManager::_double;
}

JPPyObject JPLongType::convertToPythonObject(jvalue val)
{
	return JPPyLong::fromLong(field(val));
}

JPValue JPLongType::getValueFromObject(jobject obj)
{
	jvalue v;
	field(v) = JPJni::longValue(obj);
	return JPValue(this, v);
}

JPMatch::Type JPLongType::canConvertToJava(PyObject* obj)
{
	ASSERT_NOT_NULL(obj);
	if (JPPyObject::isNone(obj))
	{
		return JPMatch::_none;
	}

	JPValue* value = JPPythonEnv::getJavaValue(obj);
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

	// FIXME this logic is screwy as it implies that 
	// only python 3 can hit an exact.  Thus either all
	// integer types should be exact or none of them.
	if (JPPyInt::check(obj))
	{
		return JPMatch::_implicit;
	}

	if (JPPyLong::check(obj))
	{
		return JPMatch::_exact;
	}

	return JPMatch::_none;
}

jvalue JPLongType::convertToJava(PyObject* obj)
{
	jvalue res;
	field(res) = 0;
	JPValue* value = JPPythonEnv::getJavaValue(obj);
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
		JP_RAISE_TYPE_ERROR("Cannot convert value to Java long");
	}
	else if (JPPyInt::check(obj))
	{
		field(res) = (type_t) JPPyInt::asInt(obj);
		return res;
	}
	else if (JPPyLong::check(obj))
	{
		field(res) = (type_t) JPPyLong::asLong(obj);
		return res;
	}

	JP_RAISE_TYPE_ERROR("Cannot convert value to Java long");
	return res; // never reached
}

jarray JPLongType::newArrayInstance(JPJavaFrame& frame, jsize sz)
{
	return frame.NewLongArray(sz);
}

JPPyObject JPLongType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticLongField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPLongType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetLongField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPLongType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticLongMethodA(claz, mth, val);
	}
	return convertToPythonObject(v);
}

JPPyObject JPLongType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallNonvirtualLongMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(v);
}

void JPLongType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticLongField(c, fid, val);
}

void JPLongType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetLongField(c, fid, val);
}

JPPyObject JPLongType::getArrayRange(JPJavaFrame& frame, jarray a, jsize lo, jsize hi)
{
	return getSlice<type_t>(frame, a, lo, lo + hi, NPY_INT64, PyLong_FromLong);
}

void JPLongType::setArrayRange(JPJavaFrame& frame, jarray a, jsize start, jsize length, PyObject* sequence)
{
	JP_TRACE_IN("JPLongType::setArrayRange");
	if (setRangeViaBuffer<array_t, type_t>(frame, a, start, length, sequence,
			&JPJavaFrame::SetLongArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);

	type_t* val = accessor.get();
	JPPySequence seq(JPPyRef::_use, sequence);
	for (Py_ssize_t i = 0; i < length; ++i)
	{
		type_t v = (type_t) PyInt_AsLong(seq[i].get());
		if (v == -1 && JPPyErr::occurred())
		{
			JP_RAISE_PYTHON("JPLongType::setArrayRange");
		}
		val[start + i] = v;
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPLongType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetLongArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(v);
}

void JPLongType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	array_t array = (array_t) a;
	type_t val = field(convertToJava(obj));
	frame.SetLongArrayRegion(array, ndx, 1, &val);
}

