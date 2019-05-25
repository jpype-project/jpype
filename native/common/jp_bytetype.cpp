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

JPByteType::JPByteType() : JPPrimitiveType(JPTypeManager::_java_lang_Byte)
{
}

JPByteType::~JPByteType()
{
}

bool JPByteType::isSubTypeOf(JPClass* other) const
{
	return other == JPTypeManager::_byte
			|| other == JPTypeManager::_short
			|| other == JPTypeManager::_int
			|| other == JPTypeManager::_long
			|| other == JPTypeManager::_float
			|| other == JPTypeManager::_double;
}

jobject JPByteType::convertToDirectBuffer(PyObject* src)
{
	JPJavaFrame frame;
	JP_TRACE_IN("JPByteType::convertToDirectBuffer");

	if (JPPyMemoryView::check(src))
	{
		char* rawData;
		long size;

		JPPyMemoryView::getByteBufferSize(src, &rawData, size);
		return frame.keep(frame.NewDirectByteBuffer(rawData, size));
	}

	JP_RAISE_RUNTIME_ERROR("Unable to convert to Direct Buffer");
	JP_TRACE_OUT;
}

JPPyObject JPByteType::convertToPythonObject(jvalue val)
{
	return JPPyInt::fromInt(field(val));
}

JPValue JPByteType::getValueFromObject(jobject obj)
{
	jvalue v;
	field(v) = (type_t) JPJni::intValue(obj);
	return JPValue(this, v);
}

JPMatch::Type JPByteType::canConvertToJava(PyObject* obj)
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

jvalue JPByteType::convertToJava(PyObject* obj)
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
		JP_RAISE_TYPE_ERROR("Cannot convert value to Java byte");
	}
	else if (JPPyLong::checkConvertable(obj))
	{
		field(res) = (type_t) assertRange(JPPyLong::asLong(obj));
		return res;
	}

	JP_RAISE_TYPE_ERROR("Cannot convert value to Java byte");
	return res;
}

jarray JPByteType::newArrayInstance(JPJavaFrame& frame, jsize sz)
{
	return frame.NewByteArray(sz);
}

JPPyObject JPByteType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticByteField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPByteType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetByteField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPByteType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticByteMethodA(claz, mth, val);
	}
	return convertToPythonObject(v);
}

JPPyObject JPByteType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == NULL)
			field(v) = frame.CallByteMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualByteMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(v);
}

void JPByteType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticByteField(c, fid, val);
}

void JPByteType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetByteField(c, fid, val);
}

JPPyObject JPByteType::getArrayRange(JPJavaFrame& frame, jarray a, jsize lo, jsize hi)
{
	return getSlice<jbyte>(frame, a, lo, lo + hi, NPY_BYTE, PyInt_FromLong);
}

void JPByteType::setArrayRange(JPJavaFrame& frame, jarray a, jsize start, jsize length, PyObject* sequence)
{
	JP_TRACE_IN("JPByteType::setArrayRange");
	if (setRangeViaBuffer<array_t, type_t>(frame, a, start, length, sequence, NPY_BYTE,
			&JPJavaFrame::SetByteArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetByteArrayElements, &JPJavaFrame::ReleaseByteArrayElements);

	type_t* val = accessor.get();
	JPPySequence seq(JPPyRef::_use, sequence);
	for (Py_ssize_t i = 0; i < length; ++i)
	{
#if (PY_VERSION_HEX >= 0x02070000)
		type_t v = (type_t) PyInt_AsLong(seq[i].get());
		if (v == -1 && JPPyErr::occurred())
		{
			JP_RAISE_PYTHON("JPByteType::setArrayRange");
		}
#else
		type_t v = (type_t) PyInt_AS_LONG(seq[i].get());
#endif
		val[start + i] = v;
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPByteType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetByteArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(v);
}

void JPByteType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	array_t array = (array_t) a;
	type_t val = field(convertToJava(obj));
	frame.SetByteArrayRegion(array, ndx, 1, &val);
}

