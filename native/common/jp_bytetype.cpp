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

JPByteType::JPByteType()
: JPPrimitiveType("byte")
{
}

JPByteType::~JPByteType()
{
}

jobject JPByteType::convertToDirectBuffer(JPJavaFrame& frame, PyObject* src)
{
	JP_TRACE_IN("JPByteType::convertToDirectBuffer");

	if (JPPyMemoryView::check(src))
	{
		char* rawData;
		long size;

		JPPyMemoryView::getByteBufferSize(src, &rawData, size);
		return frame.keep(frame.NewDirectByteBuffer(rawData, size));
	}

	JP_RAISE_TYPE_ERROR("Unable to convert to Direct Buffer");
	JP_TRACE_OUT;
}

JPPyObject JPByteType::convertToPythonObject(JPJavaFrame& frame, jvalue val)
{
	return JPPyInt::fromInt(field(val));
}

JPValue JPByteType::getValueFromObject(const JPValue& obj)
{
	JPContext *context = obj.getClass()->getContext();
	JPJavaFrame frame(context);
	jvalue v;
	field(v) = frame.CallByteMethodA(obj.getJavaObject(), context->m_ByteValueID, 0);
	return JPValue(this, v);
}

class JPConversionAsByte : public JPConversion
{
	typedef JPByteType base_t;
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass* cls, PyObject* pyobj) override
	{

		jvalue res;
		base_t::field(res) = base_t::assertRange(JPPyLong::asLong(pyobj));
		return res;
	}
} asByteConversion;

JPMatch::Type JPByteType::getJavaConversion(JPJavaFrame *frame, JPMatch &match, PyObject *pyobj)
{
	JP_TRACE_IN("JPIntType::getJavaConversion");
	JPContext *context = NULL;
	if (frame != NULL)
		context = frame->getContext();

	if (JPPyObject::isNone(pyobj))
		return match.type = JPMatch::_none;

	JPValue *value = JPPythonEnv::getJavaValue(pyobj);
	if (value != NULL)
	{
		if (value->getClass() == this)
		{
			match.conversion = javaValueConversion;
			return match.type = JPMatch::_exact;
		}

		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (context != NULL && value->getClass() == context->_java_lang_Byte)
		{
			match.conversion = unboxConversion;
			return match.type = JPMatch::_implicit;
		}

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return match.type = JPMatch::_none;
	}

	if (JPPyLong::check(pyobj))
	{
		match.conversion = &asByteConversion;
		return match.type = JPMatch::_implicit;
	}

	if (JPPyLong::checkConvertable(pyobj))
	{
		match.conversion = &asByteConversion;
		return match.type = JPPyLong::checkIndexable(pyobj) ? JPMatch::_implicit : JPMatch::_explicit;
	}

	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

jarray JPByteType::newArrayInstance(JPJavaFrame& frame, jsize sz)
{
	return frame.NewByteArray(sz);
}

JPPyObject JPByteType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticByteField(c, fid);
	return convertToPythonObject(frame, v);
}

JPPyObject JPByteType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetByteField(c, fid);
	return convertToPythonObject(frame, v);
}

JPPyObject JPByteType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticByteMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v);
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
	return convertToPythonObject(frame, v);
}

void JPByteType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE_TYPE_ERROR("Unable to convert to Java byte");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetStaticByteField(c, fid, val);
}

void JPByteType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE_TYPE_ERROR("Unable to convert to Java byte");
	type_t val = field(match.conversion->convert(&frame, this, obj));
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
	return convertToPythonObject(frame, v);
}

void JPByteType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE_TYPE_ERROR("Unable to convert to Java byte");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetByteArrayRegion((array_t) a, ndx, 1, &val);
}

