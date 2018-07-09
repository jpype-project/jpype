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

JPBooleanType::JPBooleanType() : JPPrimitiveType(JPTypeManager::_java_lang_Boolean)
{
}

JPBooleanType::~JPBooleanType()
{
}

bool JPBooleanType::isSubTypeOf(JPClass* other) const
{
	return other == JPTypeManager::_boolean;
}

JPPyObject JPBooleanType::convertToPythonObject(jvalue val)
{
	return JPPyBool::fromLong(val.z);
}

JPValue JPBooleanType::getValueFromObject(jobject obj)
{
	jvalue v;
	field(v) = JPJni::booleanValue(obj);
	return JPValue(this, v);
}

JPMatch::Type JPBooleanType::canConvertToJava(PyObject* obj)
{
	ASSERT_NOT_NULL(obj);
	if (JPPyObject::isNone(obj))
	{
		return JPMatch::_none;
	}

	// Exact Python match
	if (JPPyBool::check(obj))
	{
		return JPMatch::_exact;
	}

	JPValue* value = JPPythonEnv::getJavaValue(obj);
	if (value != NULL)
	{
		// Wrapper
		if (value->getClass() == this)
		{
			return JPMatch::_exact;
		}
		// Implicit conversion from boxed to primitive (JLS 5.1.8)
		if (value->getClass() == this->m_BoxedClass)
		{
			return JPMatch::_implicit;
		}
		return JPMatch::_none;
	}

	// Java does not consider ints to be bools, but we may need
	// it when returning from a proxy.
	if (JPPyInt::check(obj) || JPPyLong::check(obj))
	{
		return JPMatch::_implicit; // FIXME this should be JPMatch::_explicit
	}

	return JPMatch::_none;
}

jvalue JPBooleanType::convertToJava(PyObject* obj)
{
	jvalue res;

	JPValue* value = JPPythonEnv::getJavaValue(obj);
	if (value != NULL)
	{
		if (value->getClass() == this)
			return *value;
		if (value->getClass() == m_BoxedClass)
		{
			if (value->getJavaObject() == NULL)
				JP_RAISE_RUNTIME_ERROR("Null pointer in implicit conversion from boxed.");
			return getValueFromObject(value->getJavaObject());
		}
	}
	else if (JPPyLong::check(obj))
	{
		res.z = (jboolean) JPPyLong::asLong(obj);
	}
	else
	{
		res.z = (jboolean) JPPyInt::asInt(obj);
	}
	return res;
}

jarray JPBooleanType::newArrayInstance(JPJavaFrame& frame, jsize sz)
{
	return frame.NewBooleanArray(sz);
}

JPPyObject JPBooleanType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticBooleanField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPBooleanType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetBooleanField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPBooleanType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticBooleanMethodA(claz, mth, val);
	}
	return convertToPythonObject(v);
}

JPPyObject JPBooleanType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallNonvirtualBooleanMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(v);
}

void JPBooleanType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticBooleanField(c, fid, val);
}

void JPBooleanType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetBooleanField(c, fid, val);
}

JPPyObject JPBooleanType::getArrayRange(JPJavaFrame& frame, jarray a, jsize start, jsize length)
{
	return getSlice<type_t>(frame, a, start, start + length, NPY_BOOL, PyBool_FromLong);
}

void JPBooleanType::setArrayRange(JPJavaFrame& frame, jarray a, jsize start, jsize length, PyObject* sequence)
{
	JP_TRACE_IN("JPBooleanType::setArrayRange");
	if (setRangeViaBuffer<array_t, type_t>(frame, a, start, length, sequence,
			&JPJavaFrame::SetBooleanArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetBooleanArrayElements, &JPJavaFrame::ReleaseBooleanArrayElements);

	type_t* val = accessor.get();
	JPPySequence seq(JPPyRef::_use, sequence);
	for (Py_ssize_t i = 0; i < length; ++i)
	{
		// FIXME this is suspect as we are supposed to be boolean, not integers
		long v = PyInt_AsLong(seq[i].get());
		if (v == -1 && JPPyErr::occurred())
		{
			JP_RAISE_PYTHON("JPBooleanType::setArrayRange");
		}
		val[start + i] = (type_t) v;
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
	return convertToPythonObject(v);
}

void JPBooleanType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	array_t array = (array_t) a;
	type_t val = field(convertToJava(obj));
	frame.SetBooleanArrayRegion(array, ndx, 1, &val);
}

