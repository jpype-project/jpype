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
#include "jp_chartype.h"

JPCharType::JPCharType() : JPPrimitiveType(JPTypeManager::_java_lang_Character)
{
}

JPCharType::~JPCharType()
{
}

bool JPCharType::isSubTypeOf(JPClass* other) const
{
	return other == JPTypeManager::_char;
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

JPMatch::Type JPCharType::canConvertToJava(PyObject* obj)
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

	if (JPPyString::checkCharUTF16(obj))
	{
		return JPMatch::_implicit;
	}

	return JPMatch::_none;
}

jvalue JPCharType::convertToJava(PyObject* obj)
{
	JP_TRACE_IN("JPCharType::convertToJava");
	JP_TRACE(JPPyObject::getTypeName(obj));
	jvalue res;
	JPValue* value = PyJPValue_getJavaSlot(obj);
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
		JP_RAISE(PyExc_TypeError, "Cannot convert value to Java char");
	} else if (JPPyString::checkCharUTF16(obj))
	{
		res.c = JPPyString::asCharUTF16(obj);
		return res;
	}

	JP_RAISE(PyExc_TypeError, "Cannot convert value to Java char");
	return res;
	JP_TRACE_OUT;
}

jarray JPCharType::newArrayInstance(JPJavaFrame& frame, jsize sz)
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
		if (clazz == NULL)
			field(v) = frame.CallCharMethodA(obj, mth, val);
		else
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

//JPPyObject JPCharType::getArrayRange(JPJavaFrame& frame, jarray a,
//		jsize start, jsize length, jsize step)
//{
//	JP_TRACE_IN("JPCharType::getArrayRange");
//	// FIXME this section is not exception safe.
//	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
//			&JPJavaFrame::GetCharArrayElements, &JPJavaFrame::ReleaseCharArrayElements);
//
//	type_t* val = accessor.get();
//	// FIXME this is an error, the encoding used by JAVA does not match standard UTF16.
//	// We need to handle this by each code point.
//	JPPyObject res(JPPyRef::_call, PyUnicode_FromUnicode(NULL, length));
//	Py_UNICODE *pchars = PyUnicode_AS_UNICODE(res.get());
//
//	for (Py_ssize_t i = start; i < length; i++)
//		pchars[i] = (Py_UNICODE) val[i];
//	return res;
//	JP_TRACE_OUT;
//}

void JPCharType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPCharType::setArrayRange");

	// FIXME need different rules than integer/float types
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetCharArrayElements, &JPJavaFrame::ReleaseCharArrayElements);

	type_t* val = accessor.get();
	JPPySequence seq(JPPyRef::_use, sequence);
	jsize index = start;
	for (Py_ssize_t i = 0; i < length; ++i, index += step)
	{
		jchar v = JPPyString::asCharUTF16(seq[i].get());
		if (JPPyErr::occurred())
		{
			JP_RAISE_PYTHON("JPCharType::setArrayRange");
		}
		val[index] = (type_t) v;
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPCharType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetCharArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(v);
}

void JPCharType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	array_t array = (array_t) a;
	type_t val = field(convertToJava(obj));
	frame.SetCharArrayRegion(array, ndx, 1, &val);
}

void JPCharType::getView(JPArrayView& view)
{
	JPJavaFrame frame;
	view.memory = (void*) frame.GetCharArrayElements(
			(jcharArray) view.array->getJava(), &view.isCopy);
	view.buffer.format = "H";
	view.buffer.itemsize = sizeof (jchar);
}

void JPCharType::releaseView(JPArrayView& view)
{
	JPJavaFrame frame;
	frame.ReleaseCharArrayElements((jcharArray) view.array->getJava(),
			(jchar*) view.memory, view.buffer.readonly ? JNI_ABORT : 0);
}

const char* JPCharType::getBufferFormat()
{
	return "H";
}

ssize_t JPCharType::getItemSize()
{
	return sizeof (jchar);
}

void JPCharType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	jchar* b = (jchar*) ((char*) memory + offset);
	frame.GetCharArrayRegion((jcharArray) a, start, len, b);
}
