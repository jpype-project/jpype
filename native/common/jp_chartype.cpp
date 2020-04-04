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
#include "pyjp.h"

JPCharType::JPCharType()
: JPPrimitiveType("char")
{
}

JPCharType::~JPCharType()
{
}

JPPyObject JPCharType::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast)
{
	if (!cast)
	{
		return JPPyString::fromCharUTF16(val.c);
	}
	JPPyObject tmp = JPPyObject(JPPyRef::_call, PyLong_FromLong(field(val)));
	JPPyObject out = JPPyObject(JPPyRef::_call, convertLong(getHost(), (PyLongObject*) tmp.get()));
	PyJPValue_assignJavaSlot(frame, out.get(), JPValue(this, val));
	return out;
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
	typedef JPCharType base_t;
public:

	JPMatch::Type matches(JPMatch &match, JPClass *cls)  override
	{
		JP_TRACE_IN("JPConversionAsChar::matches");
		if (!JPPyString::checkCharUTF16(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		jvalue res;
		res.c = JPPyString::asCharUTF16(match.object);
		return res;
	}
} asCharConversion;

JPMatch::Type JPCharType::findJavaConversion(JPMatch &match)
{
	JP_TRACE_IN("JPCharType::getJavaConversion");

	if (match.object == Py_None)
		return match.type = JPMatch::_none;

	JPValue *value = match.getJavaSlot();
	if (value != NULL)
	{
		JPClass *cls = value->getClass();
		if (cls == this)
		{
			match.conversion = javaValueConversion;
			return match.type = JPMatch::_exact;
		}

		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (unboxConversion->matches(match, this))
			return match.type;

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return match.type = JPMatch::_none;
	}

	if (asCharConversion.matches(match, this))
		return match.type;

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
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPCharType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetCharField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPCharType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticCharMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
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
	return convertToPythonObject(frame, v, false);
}

void JPCharType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java char");
	type_t val = field(match.convert());
	frame.SetStaticCharField(c, fid, val);
}

void JPCharType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java char");
	type_t val = field(match.convert());
	frame.SetCharField(c, fid, val);
}

void JPCharType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPCharType::setArrayRange");

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetCharArrayElements, &JPJavaFrame::ReleaseCharArrayElements);

	type_t* val = accessor.get();
	JPPySequence seq(JPPyRef::_use, sequence);
	jsize index = start;
	for (Py_ssize_t i = 0; i < length; ++i, index += step)
	{
		jchar v = JPPyString::asCharUTF16(seq[i].get());
		JP_PY_CHECK();
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
	return convertToPythonObject(frame, v, false);
}

void JPCharType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java char");
	type_t val = field(match.convert());
	frame.SetCharArrayRegion((array_t) a, ndx, 1, &val);
}

void JPCharType::getView(JPArrayView& view)
{
	JPJavaFrame frame(view.getContext());
	view.m_Memory = (void*) frame.GetCharArrayElements(
			(jcharArray) view.m_Array->getJava(), &view.m_IsCopy);
	view.m_Buffer.format = "H";
	view.m_Buffer.itemsize = sizeof (jchar);
}

void JPCharType::releaseView(JPArrayView& view)
{
	try
	{
		JPJavaFrame frame(view.getContext());
		frame.ReleaseCharArrayElements((jcharArray) view.m_Array->getJava(),
				(jchar*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (JPypeException&)
	{
		// This is called as part of the cleanup routine and exceptions
		// are not permitted
	}
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

static void pack(jchar* d, jvalue v)
{
	*d = v.c;
}

PyObject *JPCharType::newMultiArray(JPJavaFrame &frame, JPPyBuffer &buffer, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPCharType::newMultiArray");
	return convertMultiArray<type_t>(
			frame, this, &pack, "c",
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}
