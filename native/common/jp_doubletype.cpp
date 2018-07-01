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

EMatchType JPDoubleType::canConvertToJava(PyObject* obj)
{
	ASSERT_NOT_NULL(obj);
	if (JPPyObject::isNone(obj))
	{
		return _none;
	}

	JPValue* value = JPPythonEnv::getJavaValue(obj);
	if (value != NULL)
	{
		if (value->getClass() == this)
		{
			return _exact;
		}

		if (value->getClass() == m_BoxedClass)
		{
			return _implicit;
		}

		// Java does not permit boxed to boxed conversions.
		return _none;
	}

	if (JPPyFloat::check(obj))
	{
		return _exact;
	}

	// Java allows conversion to any type with a longer range even if lossy
	if (JPPyInt::check(obj) || JPPyLong::check(obj))
	{
		return _implicit;
	}

	return _none;
}

jvalue JPDoubleType::convertToJava(PyObject* obj)
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
		JP_RAISE_TYPE_ERROR("Cannot convert value to Java double");
	}
	else if (JPPyFloat::check(obj))
	{
		field(res) = (type_t) JPPyFloat::asDouble(obj);
		return res;
	}
	else if (JPPyInt::check(obj))
	{
		field(res) = JPPyInt::asInt(obj);
		return res;
	}
	else if (JPPyLong::check(obj))
	{
		field(res) = (type_t) JPPyLong::asLong(obj);
		return res;
	}

	JP_RAISE_TYPE_ERROR("Cannot convert value to Java double");
	return res;
}

jarray JPDoubleType::newArrayInstance(JPJavaFrame& frame, int sz)
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

JPPyObject JPDoubleType::getArrayRange(JPJavaFrame& frame, jarray a, int lo, int hi)
{
	return getSlice<type_t>(frame, a, lo, lo + hi, NPY_FLOAT64, PyFloat_FromDouble);
}

void JPDoubleType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, PyObject* sequence)
{
	JP_TRACE_IN("JPDoubleType::setArrayRange");
	if (setRangeViaBuffer<array_t, type_t>(frame, a, start, length, sequence,
			&JPJavaFrame::SetDoubleArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetDoubleArrayElements, &JPJavaFrame::ReleaseDoubleArrayElements);

	type_t* val = accessor.get();
	JPPySequence seq(JPPyRef::_use, sequence);
	for (Py_ssize_t i = 0; i < length; ++i)
	{
		type_t v = (type_t) PyFloat_AsDouble(seq[i].get());
		if (v == -1. && JPPyErr::occurred())
		{
			JP_RAISE_PYTHON("JPDoubleType::setArrayRange");
		}
		val[start + i] = v;
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPDoubleType::getArrayItem(JPJavaFrame& frame, jarray a, int ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetDoubleArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(v);
}

void JPDoubleType::setArrayItem(JPJavaFrame& frame, jarray a, int ndx, PyObject* obj)
{
	array_t array = (array_t) a;
	type_t val = field(convertToJava(obj));
	frame.SetDoubleArrayRegion(array, ndx, 1, &val);
}

