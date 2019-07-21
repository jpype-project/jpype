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

JPDoubleType::JPDoubleType(JPContext* context, jclass clss,
		const string& name,
		JPBoxedType* boxedClass,
		jint modifiers)
: JPPrimitiveType(context, clss, name, boxedClass, modifiers)
{
	JPJavaFrame frame(context);
	_DoubleValueID = frame.GetMethodID(boxedClass->getJavaClass(), "doubleValue", "()D");
}

JPDoubleType::~JPDoubleType()
{
}

JPPyObject JPDoubleType::convertToPythonObject(jvalue val)
{
	return JPPyFloat::fromDouble(field(val));
}

JPValue JPDoubleType::getValueFromObject(jobject obj)
{
	JPJavaFrame frame(m_Context);
	jvalue v;
	field(v) = frame.CallDoubleMethodA(obj, _DoubleValueID, 0);
	return JPValue(this, v);
}

class JPConversionAsDouble : public JPConversion
{
	typedef JPDoubleType base_t;
public:

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		jvalue res;
		base_t::field(res) = (base_t::type_t) JPPyFloat::asDouble(pyobj);
		return res;
	}
} asDoubleConversion;

class JPConversionAsDoubleLong : public JPConversion
{
	typedef JPDoubleType base_t;
public:

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		jvalue res;
		base_t::field(res) = (base_t::type_t) JPPyLong::asLong(pyobj);
		return res;
	}
} asDoubleLongConversion;

class JPConversionDoubleWidenInt : public JPConversion
{
	typedef JPDoubleType base_t;
public:

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		JPValue* value = JPPythonEnv::getJavaValue(pyobj);
		jvalue ret;
		ret.d = (jdouble) ((JPPrimitiveType*)value->getClass())->getAsLong(value->getValue());
		return ret;
	}
} doubleIntWidenConversion;

class JPConversionDoubleWidenFloat : public JPConversion
{
	typedef JPDoubleType base_t;
public:

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		JPValue* value = JPPythonEnv::getJavaValue(pyobj);
		jvalue ret;
		ret.d = (jdouble) ((JPPrimitiveType*)value->getClass())->getAsDouble(value->getValue());
		return ret;
	}
} doubleFloatWidenConversion;

JPMatch::Type JPDoubleType::getJavaConversion(JPMatch& match, JPJavaFrame& frame, PyObject* pyobj)
{
	JP_TRACE_IN("JPDoubleType::getJavaConversion");
	if (JPPyObject::isNone(pyobj))
		return match.type = JPMatch::_none;

	JPValue *value = JPPythonEnv::getJavaValue(pyobj);
	if (value != NULL)
	{
		JPClass *cls = value->getClass();
		if (cls == this)
		{
			match.conversion = javaValueConversion;
			return match.type = JPMatch::_exact;
		}

		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (cls == m_BoxedClass)
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
				case 'B':
				case 'S':
				case 'C':
				case 'I':
				case 'J':
					match.conversion = &doubleIntWidenConversion;
					return match.type = JPMatch::_implicit;
				case 'F':
					match.conversion = &doubleFloatWidenConversion;
					return match.type = JPMatch::_implicit;
				default:
					return match.type = JPMatch::_none;
			}
		}

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8) 
		return match.type = JPMatch::_none;
	}

	if (JPPyFloat::check(pyobj))
	{
		match.conversion = &asDoubleConversion;
		return match.type = JPMatch::_exact;
	}

	if (JPPyFloat::checkConvertable(pyobj))
	{
		match.conversion = &asDoubleConversion;
		return match.type = JPMatch::_implicit;
	}

	if (JPPyLong::checkConvertable(pyobj))
	{
		match.conversion = &asDoubleLongConversion;
		return match.type = JPMatch::_implicit;
	}

	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

jarray JPDoubleType::newArrayInstance(JPJavaFrame& frame, jsize sz)
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
		if (clazz == NULL)
			field(v) = frame.CallDoubleMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualDoubleMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(v);
}

void JPDoubleType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(match, frame, obj) < JPMatch::_implicit)
		JP_RAISE_TYPE_ERROR("Unable to convert to Java double");
	type_t val = field(match.conversion->convert(frame, this, obj));
	frame.SetStaticDoubleField(c, fid, val);
}

void JPDoubleType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(match, frame, obj) < JPMatch::_implicit)
		JP_RAISE_TYPE_ERROR("Unable to convert to Java double");
	type_t val = field(match.conversion->convert(frame, this, obj));
	frame.SetDoubleField(c, fid, val);
}

JPPyObject JPDoubleType::getArrayRange(JPJavaFrame& frame, jarray a, jsize lo, jsize hi)
{
	return getSlice<type_t>(frame, a, lo, lo + hi, NPY_FLOAT64, PyFloat_FromDouble);
}

void JPDoubleType::setArrayRange(JPJavaFrame& frame, jarray a, jsize start, jsize length, PyObject* sequence)
{
	JP_TRACE_IN("JPDoubleType::setArrayRange");
	if (setRangeViaBuffer<array_t, type_t>(frame, a, start, length, sequence, NPY_FLOAT64,
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

JPPyObject JPDoubleType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetDoubleArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(v);
}

void JPDoubleType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(match, frame, obj) < JPMatch::_implicit)
		JP_RAISE_TYPE_ERROR("Unable to convert to Java double");
	type_t val = field(match.conversion->convert(frame, this, obj));
	frame.SetDoubleArrayRegion((array_t) a, ndx, 1, &val);
}

