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

JPShortType::JPShortType(JPContext* context, jclass clss,
		const string& name,
		JPBoxedType* boxedClass,
		jint modifiers)
: JPPrimitiveType(context, clss, name, boxedClass, modifiers)
{
	JPJavaFrame frame(context);
	jfieldID fid;
	fid = frame.GetStaticFieldID(boxedClass->getJavaClass(), "MIN_VALUE", "S");
	m_Short_Min = frame.GetStaticShortField(boxedClass->getJavaClass(), fid);
	fid = frame.GetStaticFieldID(boxedClass->getJavaClass(), "MAX_VALUE", "S");
	m_Short_Max = frame.GetStaticShortField(boxedClass->getJavaClass(), fid);
	m_ShortValueID = frame.GetMethodID(boxedClass->getJavaClass(), "shortValue", "()S");
}

JPShortType::~JPShortType()
{
}

bool JPShortType::isSubTypeOf(JPClass* other) const
{
	return other == m_Context->_short
			|| other == m_Context->_int
			|| other == m_Context->_long
			|| other == m_Context->_float
			|| other == m_Context->_double;
}

JPPyObject JPShortType::convertToPythonObject(jvalue val)
{
	return JPPyInt::fromInt(field(val));
}

JPValue JPShortType::getValueFromObject(jobject obj)
{
	JPJavaFrame frame(m_Context);
	jvalue v;
	field(v) = frame.CallShortMethodA(obj, m_ShortValueID, 0);
	return JPValue(this, v);
}

class JPConversionAsShort : public JPConversion
{
	typedef JPShortType base_t;
public:

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		jvalue res;
		base_t::field(res) = (base_t::type_t) ((base_t*) cls)->assertRange(JPPyLong::asLong(pyobj));
		return res;
	}
} asShortConversion;

JPMatch::Type JPShortType::getJavaConversion(JPMatch& match, JPJavaFrame& frame, PyObject* pyobj)
{
	JP_TRACE_IN("JPShortType::getJavaConversion");
	match.type = JPMatch::_none;
	if (JPPyObject::isNone(pyobj))
		return JPMatch::_none;

	JPValue* value = JPPythonEnv::getJavaValue(pyobj);
	if (value != NULL)
	{
		if (value->getClass() == this)
		{
			match.conversion = javaValueConversion;
			return match.type = JPMatch::_exact;
		}

		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (value->getClass() == m_BoxedClass)
		{
			match.conversion = unboxConversion;
			return match.type = JPMatch::_implicit;
		}

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8) 
		return match.type;
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

	return match.type;
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
	return convertToPythonObject(v);
}

JPPyObject JPShortType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetShortField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPShortType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticShortMethodA(claz, mth, val);
	}
	return convertToPythonObject(v);
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
	return convertToPythonObject(v);
}

void JPShortType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(match, frame, obj) < JPMatch::_implicit)
		JP_RAISE_TYPE_ERROR("Unable to convert to Java short");
	type_t val = field(match.conversion->convert(frame, this, obj));
	frame.SetStaticShortField(c, fid, val);
}

void JPShortType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(match, frame, obj) < JPMatch::_implicit)
		JP_RAISE_TYPE_ERROR("Unable to convert to Java short");
	type_t val = field(match.conversion->convert(frame, this, obj));
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
	return convertToPythonObject(v);
}

void JPShortType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(match, frame, obj) < JPMatch::_implicit)
		JP_RAISE_TYPE_ERROR("Unable to convert to Java short");
	type_t val = field(match.conversion->convert(frame, this, obj));
	frame.SetShortArrayRegion((array_t) a, ndx, 1, &val);
}

