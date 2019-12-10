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

JPShortType::JPShortType()
: JPPrimitiveType("short")
{
}

JPShortType::~JPShortType()
{
}

JPPyObject JPShortType::convertToPythonObject(JPJavaFrame& frame, jvalue val)
{
	return JPPyInt::fromInt(field(val));
}

JPValue JPShortType::getValueFromObject(const JPValue& obj)
{
	JPContext *context = obj.getClass()->getContext();
	JPJavaFrame frame(context);
	jvalue v;
	field(v) = frame.CallShortMethodA(obj.getJavaObject(), context->m_ShortValueID, 0);
	return JPValue(this, v);
}

class JPConversionAsShort : public JPConversion
{
	typedef JPShortType base_t;
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue res;
		base_t::field(res) = base_t::assertRange(JPPyLong::asLong(pyobj));
		return res;
	}
} asShortConversion;

class JPConversionShortWiden : public JPConversion
{
	typedef JPShortType base_t;
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass* cls, PyObject* pyobj) override
	{
		JPValue* value = JPPythonEnv::getJavaValue(pyobj);
		jvalue ret;
		ret.s = (jshort) ((JPPrimitiveType*) value->getClass())->getAsLong(value->getValue());
		return ret;
	}
} shortWidenConversion;

JPMatch::Type JPShortType::getJavaConversion(JPJavaFrame *frame, JPMatch &match, PyObject *pyobj)
{
	JP_TRACE_IN("JPShortType::getJavaConversion");
	JPContext *context = NULL;
	if (frame != NULL)
		context = frame->getContext();

	if (JPPyObject::isNone(pyobj))
		return match.type = JPMatch::_none;

	JPValue* value = JPPythonEnv::getJavaValue(pyobj);
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
		if (context != NULL && cls == context->_java_lang_Short)
		{
			match.conversion = unboxConversion;
			return match.type = JPMatch::_implicit;
		}

		// Consider widening
		if (cls->isPrimitive())
		{
			// https://docs.oracle.com/javase/specs/jls/se7/html/jls-5.html#jls-5.1.2
			JPPrimitiveType *prim = (JPPrimitiveType*) cls;
			switch (prim->getTypeCode())
			{
				case 'C':
				case 'B':
					match.conversion = &shortWidenConversion;
					return match.type = JPMatch::_implicit;
				default:
					return match.type = JPMatch::_none;
			}
		}

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return match.type = JPMatch::_none;
	}

	if (JPPyLong::check(pyobj))
	{
		match.conversion = &asShortConversion;
		return match.type = JPMatch::_implicit;
	}

	if (JPPyLong::checkConvertable(pyobj))
	{
		match.conversion = &asShortConversion;
		match.type = JPPyLong::checkIndexable(pyobj) ? JPMatch::_implicit : JPMatch::_explicit;
		return match.type;
	}

	return match.type =  JPMatch::_none;
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
	return convertToPythonObject(frame, v);
}

JPPyObject JPShortType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetShortField(c, fid);
	return convertToPythonObject(frame, v);
}

JPPyObject JPShortType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticShortMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v);
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
	return convertToPythonObject(frame, v);
}

void JPShortType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE_TYPE_ERROR("Unable to convert to Java short");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetStaticShortField(c, fid, val);
}

void JPShortType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE_TYPE_ERROR("Unable to convert to Java short");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetShortField(c, fid, val);
}

JPPyObject JPShortType::getArrayRange(JPJavaFrame& frame, jarray a, jsize lo, jsize hi)
{
	return getSlice<type_t>(frame, a, lo, lo + hi, NPY_SHORT, PyInt_FromLong);
}

void JPShortType::setArrayRange(JPJavaFrame& frame, jarray a, jsize start, jsize length, PyObject* sequence)
{
	JP_TRACE_IN("JPShortType::setArrayRange");
	if (setRangeViaBuffer<array_t, type_t>(frame, a, start, length, sequence, NPY_SHORT,
			&JPJavaFrame::SetShortArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetShortArrayElements, &JPJavaFrame::ReleaseShortArrayElements);

	type_t* val = accessor.get();
	JPPySequence seq(JPPyRef::_use, sequence);
	for (Py_ssize_t i = 0; i < length; ++i)
	{
		type_t v = (type_t) PyInt_AsLong(seq[i].get());
		if (v == -1 && JPPyErr::occurred())
		{
			JP_RAISE_PYTHON("JPShortType::setArrayRange");
		}
		val[start + i] = v;
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
	return convertToPythonObject(frame, v);
}

void JPShortType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE_TYPE_ERROR("Unable to convert to Java short");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetShortArrayRegion((array_t) a, ndx, 1, &val);
}

string JPShortType::asString(jvalue v)
{
	std::stringstream out;
	out << v.s;
	return out.str();
}
