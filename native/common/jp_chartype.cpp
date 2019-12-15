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

JPCharType::JPCharType()
: JPPrimitiveType("char")
{
}

JPCharType::~JPCharType()
{
}

JPPyObject JPCharType::convertToPythonObject(JPJavaFrame& frame, jvalue val)
{
	return JPPyString::fromCharUTF16(val.c);
}

JPValue JPCharType::getValueFromObject(const JPValue& obj)
{
	JPContext *context = obj.getClass()->getContext();
	JPJavaFrame frame(context);
	jvalue v;
	field(v) = frame.CallCharMethodA(obj.getValue().l, context->m_CharValueID, 0);
	return JPValue(this, v);
}

class JPConversionAsChar : public JPConversion
{
	typedef JPIntType base_t;
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue res;
		res.c = JPPyString::asCharUTF16(pyobj);
		return res;
	}
} asCharConversion;

JPMatch::Type JPCharType::getJavaConversion(JPJavaFrame *frame, JPMatch &match, PyObject *pyobj)
{
	JP_TRACE_IN("JPCharType::getJavaConversion");
	JPContext *context = NULL;
	if (frame != NULL)
		context = frame->getContext();

	if (JPPyObject::isNone(pyobj))
		return match.type = JPMatch::_none;

	JPValue *value = JPPythonEnv::getJavaValue(pyobj);
	if (value != NULL)
	{
		JPClass *cls = value->getClass();
		if (cls == NULL)
			return match.type = JPMatch::_none;

		if (cls == this)
		{
			match.conversion = javaValueConversion;
			return match.type = JPMatch::_exact;
		}

		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (context != NULL && cls == context->_java_lang_Char)
		{
			match.conversion = unboxConversion;
			return match.type = JPMatch::_implicit;
		}

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return match.type = JPMatch::_none;
	}

	if (JPPyString::checkCharUTF16(pyobj))
	{
		match.conversion = &asCharConversion;
		return match.type = JPMatch::_implicit;
	}

	return match.type = JPMatch::_none;
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
	return convertToPythonObject(frame, v);
}

JPPyObject JPCharType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetCharField(c, fid);
	return convertToPythonObject(frame, v);
}

JPPyObject JPCharType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticCharMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v);
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
	return convertToPythonObject(frame, v);
}

void JPCharType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java char");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetStaticCharField(c, fid, val);
}

void JPCharType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java char");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetCharField(c, fid, val);
}

JPPyObject JPCharType::getArrayRange(JPJavaFrame& frame, jarray a, jsize start, jsize length)
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

void JPCharType::setArrayRange(JPJavaFrame& frame, jarray a, jsize start, jsize length, PyObject* sequence)
{
	JP_TRACE_IN("JPCharType::setArrayRange");
	if (setRangeViaBuffer<array_t, type_t>(frame, a, start, length, sequence, NPY_SHORT,
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

JPPyObject JPCharType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetCharArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(frame, v);
}

void JPCharType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java char");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetCharArrayRegion((array_t) a, ndx, 1, &val);
}

string JPCharType::asString(jvalue v)
{
	std::stringstream out;
	out << v.c;
	return out.str();
}
