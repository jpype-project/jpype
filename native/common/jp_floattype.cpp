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
	field(v) = JPJni::doubleValue(obj);
	return JPValue(this, v);
}

EMatchType JPFloatType::canConvertToJava(PyObject* obj)
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
		// This next line is a puzzle.  It seems like it should be _exact.
		return _implicit;
	}

	// Java allows conversion to any type with a longer range even if lossy
	if (JPPyInt::check(obj) || JPPyLong::check(obj))
	{
		return _implicit;
	}

	return _none;
}

jvalue JPFloatType::convertToJava(PyObject* obj)
{
	jvalue res;
	JPValue* value = JPPythonEnv::getJavaValue(obj);
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

		JP_RAISE_OVERFLOW_ERROR("Cannot convert value to Java float");
	}
	else if (JPPyFloat::check(obj))
	{
		double l = JPPyFloat::asDouble(obj);
		// FIXME the check for s_minFloat seems wrong.
		// Java would trim to 0 rather than giving an error.
		if (l >= 0 && l > JPJni::s_Float_Max)
		{
			JP_RAISE_TYPE_ERROR("Cannot convert value to Java float");
		}
		else if (l < 0 && l < -JPJni::s_Float_Max)
		{
			JP_RAISE_TYPE_ERROR("Cannot convert value to Java float");
		}
		res.f = (jfloat) l;
		return res;
	}
	else if (JPPyInt::check(obj))
	{
		field(res) = JPPyInt::asInt(obj);
		return res;
	}
	else if (JPPyLong::check(obj))
	{
		field(res) = JPPyLong::asLong(obj);
		return res;
	}

	JP_RAISE_TYPE_ERROR("Cannot convert value to Java float");
	return res;
}

jarray JPFloatType::newArrayInstance(JPJavaFrame& frame, int sz)
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

JPPyObject JPFloatType::getArrayRange(JPJavaFrame& frame, jarray a, int lo, int hi)
{
	return getSlice<jfloat>(frame, a, lo, lo + hi, NPY_FLOAT32, PyFloat_FromDouble);
}

void JPFloatType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, PyObject* sequence)
{
	JP_TRACE_IN("JPFloatType::setArrayRange");
	if (setRangeViaBuffer<array_t, type_t>(frame, a, start, length, sequence,
			&JPJavaFrame::SetFloatArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetFloatArrayElements, &JPJavaFrame::ReleaseFloatArrayElements);

	type_t* val = accessor.get();
	JPPySequence seq(JPPyRef::_use, sequence);
	for (Py_ssize_t i = 0; i < length; ++i)
	{
		type_t v = (type_t) PyFloat_AsDouble(seq[i].get());
		if (v == -1. && JPPyErr::occurred())
		{
			JP_RAISE_PYTHON("JPFloatType::setArrayRange");
		}
		val[start + i] = v;
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPFloatType::getArrayItem(JPJavaFrame& frame, jarray a, int ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetFloatArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(v);
}

void JPFloatType::setArrayItem(JPJavaFrame& frame, jarray a, int ndx, PyObject* obj)
{
	array_t array = (array_t) a;
	type_t val = field(convertToJava(obj));
	frame.SetFloatArrayRegion(array, ndx, 1, &val);
}

