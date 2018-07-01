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

JPCharType::JPCharType() : JPPrimitiveType(JPTypeManager::_java_lang_Char)
{
}

JPCharType::~JPCharType()
{
}

bool JPCharType::isSubTypeOf(JPClass* other) const
{
	return other == JPTypeManager::_char;
	// FIXME this wss wrong.  Java char is not an integer type.
	//			|| other == JPTypeManager::_int
	//			|| other == JPTypeManager::_long
	//			|| other == JPTypeManager::_float
	//			|| other == JPTypeManager::_double;
}

JPPyObject JPCharType::convertToPythonObject(jvalue val)
{
	return JPPyString::fromCharUTF16(val.c);
}

JPValue JPCharType::getValueFromObject(jobject obj)
{
	jvalue v;
	field(v) = JPJni::charValue(obj);
	return JPValue(this, v);
}

EMatchType JPCharType::canConvertToJava(PyObject* obj)
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

	if (JPPyString::checkCharUTF16(obj))
	{
		return _implicit;
	}

	return _none;
}

jvalue JPCharType::convertToJava(PyObject* obj)
{
	JP_TRACE_IN("JPCharType::convertToJava");
	JP_TRACE(JPPyObject::getTypeName(obj));
	jvalue res;
	JPValue* value = JPPythonEnv::getJavaValue(obj);
	if (value != NULL)
	{
		JP_TRACE("Java Value");
		if (value->getClass() == this)
		{
			return *value;
		}
		if (value->getClass() == m_BoxedClass)
		{
			return getValueFromObject(value->getJavaObject());
		}
		JP_RAISE_TYPE_ERROR("Cannot convert value to Java char");
	}
	else if (JPPyString::checkCharUTF16(obj))
	{
		res.c = JPPyString::asCharUTF16(obj);
		return res;
	}

	JP_RAISE_TYPE_ERROR("Cannot convert value to Java char");
	return res;
	JP_TRACE_OUT;
}

jarray JPCharType::newArrayInstance(JPJavaFrame& frame, int sz)
{
	return frame.NewCharArray(sz);
}

JPPyObject JPCharType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticCharField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPCharType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetCharField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPCharType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticCharMethodA(claz, mth, val);
	}
	return convertToPythonObject(v);
}

JPPyObject JPCharType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallNonvirtualCharMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(v);
}

void JPCharType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticCharField(c, fid, val);
}

void JPCharType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetCharField(c, fid, val);
}

JPPyObject JPCharType::getArrayRange(JPJavaFrame& frame, jarray a, int start, int length)
{
	JP_TRACE_IN("JPCharType::getArrayRange");
	// FIXME this section is not exception safe.
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetCharArrayElements, &JPJavaFrame::ReleaseCharArrayElements);

	type_t* val = accessor.get();
	// FIXME this is an error, the encoding used by JAVA does not match standard UTF16.
	// We need to handle this by each code point.
	JPPyObject res(JPPyRef::_call, PyUnicode_FromUnicode(NULL, length));
	Py_UNICODE *pchars = PyUnicode_AS_UNICODE(res.get());

	for (Py_ssize_t i = start; i < length; i++)
		pchars[i] = (Py_UNICODE) val[i];
	return res;
	JP_TRACE_OUT;
}

void JPCharType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, PyObject* sequence)
{
	JP_TRACE_IN("JPCharType::setArrayRange");
	if (setRangeViaBuffer<array_t, type_t>(frame, a, start, length, sequence,
			&JPJavaFrame::SetCharArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetCharArrayElements, &JPJavaFrame::ReleaseCharArrayElements);

	type_t* val = accessor.get();
	JPPySequence seq(JPPyRef::_use, sequence);
	for (Py_ssize_t i = 0; i < length; ++i)
	{
		jchar v = JPPyString::asCharUTF16(seq[i].get());
		if (JPPyErr::occurred())
		{
			JP_RAISE_PYTHON("JPCharType::setArrayRange");
		}
		val[start + i] = (type_t) v;
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPCharType::getArrayItem(JPJavaFrame& frame, jarray a, int ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetCharArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(v);
}

void JPCharType::setArrayItem(JPJavaFrame& frame, jarray a, int ndx, PyObject* obj)
{
	array_t array = (array_t) a;
	type_t val = field(convertToJava(obj));
	frame.SetCharArrayRegion(array, ndx, 1, &val);
}
