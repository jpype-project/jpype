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
#include "jp_doubletype.h"
#include "pyjp.h"

JPDoubleType::JPDoubleType()
: JPPrimitiveType("double")
{
}

JPDoubleType::~JPDoubleType()
{
}

JPPyObject JPDoubleType::convertToPythonObject(JPJavaFrame& frame, jvalue val)
{
	return JPPyFloat::fromDouble(field(val));
}

JPValue JPDoubleType::getValueFromObject(const JPValue& obj)
{
	JPContext *context = obj.getClass()->getContext();
	JPJavaFrame frame(context);
	jvalue v;
	field(v) = frame.CallDoubleMethodA(obj.getValue().l, context->m_DoubleValueID, 0);
	return JPValue(this, v);
}

class JPConversionAsDouble : public JPConversion
{
	typedef JPDoubleType base_t;
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		jvalue res;
		double val = PyFloat_AsDouble(pyobj);
		if (val == -1.0)
			JP_PY_CHECK();
		base_t::field(res) = (base_t::type_t) val;
		return res;
	}
} asDoubleConversion;

class JPConversionAsDoubleLong : public JPConversion
{
	typedef JPDoubleType base_t;
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
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

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JPValue *value = PyJPValue_getJavaSlot(pyobj);
		jvalue ret;
		ret.d = (jdouble) ((JPPrimitiveType*) value->getClass())->getAsLong(value->getValue());
		return ret;
	}
} doubleIntWidenConversion;

class JPConversionDoubleWidenFloat : public JPConversion
{
	typedef JPDoubleType base_t;
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JPValue *value = PyJPValue_getJavaSlot(pyobj);
		jvalue ret;
		ret.d = (jdouble) ((JPPrimitiveType*) value->getClass())->getAsDouble(value->getValue());
		return ret;
	}
} doubleFloatWidenConversion;

JPMatch::Type JPDoubleType::getJavaConversion(JPJavaFrame *frame, JPMatch &match, PyObject *pyobj)
{
	JP_TRACE_IN("JPDoubleType::getJavaConversion");
	JPContext *context = NULL;
	if (frame != NULL)
		context = frame->getContext();

	if (JPPyObject::isNone(pyobj))
		return match.type = JPMatch::_none;

	JPValue *value = PyJPValue_getJavaSlot(pyobj);
	if (value != NULL)
	{
		JPClass *cls = value->getClass();
		if (cls == this)
		{
			match.conversion = javaValueConversion;
			return match.type = JPMatch::_exact;
		}

		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (context != NULL && cls == context->_java_lang_Double)
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

	if (PyFloat_CheckExact(pyobj))
	{
		match.conversion = &asDoubleConversion;
		return match.type = JPMatch::_exact;
	}

	if (PyFloat_Check(pyobj))
	{
		match.conversion = &asDoubleConversion;
		return match.type = JPMatch::_implicit;
	}

	if (PyLong_Check(pyobj) || PyIndex_Check(pyobj))
	{
		match.conversion = &asDoubleLongConversion;
		return match.type = JPMatch::_implicit;
	}

	if (PyNumber_Check(pyobj))
	{
		match.conversion = &asDoubleConversion;
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
	return convertToPythonObject(frame, v);
}

JPPyObject JPDoubleType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetDoubleField(c, fid);
	return convertToPythonObject(frame, v);
}

JPPyObject JPDoubleType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticDoubleMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v);
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
	return convertToPythonObject(frame, v);
}

void JPDoubleType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java double");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetStaticDoubleField(c, fid, val);
}

void JPDoubleType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java double");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetDoubleField(c, fid, val);
}

void JPDoubleType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPDoubleType::setArrayRange");
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetDoubleArrayElements, &JPJavaFrame::ReleaseDoubleArrayElements);

	type_t* val = accessor.get();
	// First check if assigning sequence supports buffer API
	if (PyObject_CheckBuffer(sequence))
	{
		JPPyBuffer buffer(sequence, PyBUF_FULL_RO);
		if (buffer.valid())
		{
			Py_buffer& view = buffer.getView();
			if (view.ndim != 1)
				JP_RAISE(PyExc_TypeError, "buffer dims incorrect");
			Py_ssize_t vshape = view.shape[0];
			Py_ssize_t vstep = view.strides[0];
			if (vshape != length)
				JP_RAISE(PyExc_ValueError, "mismatched size");

			char* memory = (char*) view.buf;
			if (view.suboffsets && view.suboffsets[0] >= 0)
				memory = *((char**) memory) + view.suboffsets[0];
			jsize index = start;
			jconverter conv = getConverter(view.format, (int) view.itemsize, "d");
			for (Py_ssize_t i = 0; i < length; ++i, index += step)
			{
				jvalue r = conv(memory);
				val[index] = r.d;
				memory += vstep;
			}
			accessor.commit();
			return;
		} else
		{
			PyErr_Clear();
		}
	}

	// Use sequence API
	JPPySequence seq(JPPyRef::_use, sequence);
	jsize index = start;
	for (Py_ssize_t i = 0; i < length; ++i, index += step)
	{
		type_t v = (type_t) PyFloat_AsDouble(seq[i].get());
		if (v == -1)
			JP_PY_CHECK();
		val[index] = v;
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
	return convertToPythonObject(frame, v);
}

void JPDoubleType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java double");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetDoubleArrayRegion((array_t) a, ndx, 1, &val);
}

void JPDoubleType::getView(JPArrayView& view)
{
	JPJavaFrame frame(view.getContext());
	view.m_Memory = (void*) frame.GetDoubleArrayElements(
			(jdoubleArray) view.m_Array->getJava(), &view.m_IsCopy);
	view.m_Buffer.format = "d";
	view.m_Buffer.itemsize = sizeof (jdouble);
}

void JPDoubleType::releaseView(JPArrayView& view)
{
	try
	{
		JPJavaFrame frame(view.getContext());
		frame.ReleaseDoubleArrayElements((jdoubleArray) view.m_Array->getJava(),
			(jdouble*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (JPypeException& ex)
	{
		// This is called as part of the cleanup routine and exceptions
		// are not permitted
	}
}

const char* JPDoubleType::getBufferFormat()
{
	return "d";
}

ssize_t JPDoubleType::getItemSize()
{
	return sizeof (jdouble);
}

void JPDoubleType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	jdouble* b = (jdouble*) ((char*) memory + offset);
	frame.GetDoubleArrayRegion((jdoubleArray) a, start, len, b);
}
