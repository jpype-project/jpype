/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#include "jpype.h"
#include "pyjp.h"
#include "jp_array.h"
#include "jp_primitive_accessor.h"
#include "jp_booleantype.h"
#include "jp_boxedtype.h"

JPBooleanType::JPBooleanType()
: JPPrimitiveType("boolean")
{
}

JPBooleanType::~JPBooleanType()
= default;

JPClass* JPBooleanType::getBoxedClass(JPJavaFrame& frame) const
{
	return frame.getContext()->_java_lang_Boolean;
}

JPPyObject JPBooleanType::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast)
{
	return JPPyObject::call(PyBool_FromLong(val.z));
}

JPValue JPBooleanType::getValueFromObject(JPJavaFrame& frame, const JPValue& obj)
{
	jvalue v;
	field(v) = frame.CallBooleanMethodA(obj.getValue().l, frame.getContext()->_java_lang_Boolean->m_BooleanValueID, nullptr) != 0;
	return JPValue(this, v);
}

class JPConversionAsBoolean : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (!PyBool_Check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_exact;
	}

	void getInfo(JPClass * cls, JPConversionInfo &info) override
	{
		PyList_Append(info.exact, (PyObject*) & PyBool_Type);
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		jlong v = PyObject_IsTrue(match.object);
		if (v == -1)
			JP_PY_CHECK();
		res.z = v != 0;
		return res;
	}
} asBooleanExact;

class JPConversionAsBooleanJBool : public JPConversionJavaValue
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{

		JPValue *value = match.getJavaSlot();
		if (value == nullptr)
			return match.type = JPMatch::_none;
		match.type = JPMatch::_none;
		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (javaValueConversion->matches(cls, match)
				|| unboxConversion->matches(cls, match))
			return match.type;

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return JPMatch::_implicit; // search no further.
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		JPContext *context = JPContext_global;
		PyList_Append(info.exact, (PyObject*) context->_boolean->getHost());
		unboxConversion->getInfo(cls, info);
	}

} asBooleanJBool;

class JPConversionAsBooleanLong : public JPConversionAsBoolean
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (!PyLong_CheckExact(match.object)
				&& !PyIndex_Check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyObject *typing = PyImport_AddModule("jpype.protocol");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "SupportsIndex"));
		PyList_Append(info.expl, proto.get());
	}

} asBooleanLong;

class JPConversionAsBooleanNumber : public JPConversionAsBoolean
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (!PyNumber_Check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_explicit;
	}

	void getInfo(JPClass * cls, JPConversionInfo &info) override
	{
		PyObject *typing = PyImport_AddModule("jpype.protocol");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "SupportsFloat"));
		PyList_Append(info.expl, proto.get());
	}

} asBooleanNumber;

JPMatch::Type JPBooleanType::findJavaConversion(JPMatch &match)
{
	JP_TRACE_IN("JPBooleanType::findJavaConversion", this);

	if (match.object ==  Py_None)
		return match.type = JPMatch::_none;

	if (asBooleanExact.matches(this, match)
			|| asBooleanJBool.matches(this, match)
			|| asBooleanLong.matches(this, match)
			|| asBooleanNumber.matches(this, match)
			)
		return match.type;
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPBooleanType::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	asBooleanExact.getInfo(this, info);
	asBooleanJBool.getInfo(this, info);
	asBooleanLong.getInfo(this, info);
	asBooleanNumber.getInfo(this, info);
	PyList_Append(info.ret, (PyObject*) & PyBool_Type);
}

jarray JPBooleanType::newArrayOf(JPJavaFrame& frame, jsize sz)
{
	return frame.NewBooleanArray(sz);
}

JPPyObject JPBooleanType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticBooleanField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPBooleanType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetBooleanField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPBooleanType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticBooleanMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPBooleanType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == nullptr)
			field(v) = frame.CallBooleanMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualBooleanMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

void JPBooleanType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java boolean");
	type_t val = field(match.convert());
	frame.SetStaticBooleanField(c, fid, val);
}

void JPBooleanType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java boolean");
	type_t val = field(match.convert());
	frame.SetBooleanField(c, fid, val);
}

void JPBooleanType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPBooleanType::setArrayRange");
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetBooleanArrayElements, &JPJavaFrame::ReleaseBooleanArrayElements);

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
			jconverter conv = getConverter(view.format, (int) view.itemsize, "z");
			for (Py_ssize_t i = 0; i < length; ++i, index += step)
			{
				jvalue r = conv(memory);
				val[index] = r.z;
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
	JPPySequence seq = JPPySequence::use(sequence);
	jsize index = start;
	for (Py_ssize_t i = 0; i < length; ++i, index += step)
	{
		int v = PyObject_IsTrue(seq[i].get());
		if (v == -1)
			JP_PY_CHECK();
		val[index] = v;
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPBooleanType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	auto array = (array_t) a;
	type_t val;
	frame.GetBooleanArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(frame, v, false);
}

void JPBooleanType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java boolean");
	type_t val = field(match.convert());
	frame.SetBooleanArrayRegion((array_t) a, ndx, 1, &val);
}

void JPBooleanType::getView(JPArrayView& view)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	view.m_Memory = (void*) frame.GetBooleanArrayElements(
			(jbooleanArray) view.m_Array->getJava(), &view.m_IsCopy);
	view.m_Buffer.format = "?";
	view.m_Buffer.itemsize = sizeof (jboolean);
}

void JPBooleanType::releaseView(JPArrayView& view)
{
	try
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		frame.ReleaseBooleanArrayElements((jbooleanArray) view.m_Array->getJava(),
				(jboolean*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (JPypeException&)
	{
		// This is called as part of the cleanup routine and exceptions
		// are not permitted
	}
}

const char* JPBooleanType::getBufferFormat()
{
	return "?";
}

Py_ssize_t JPBooleanType::getItemSize()
{
	return sizeof (jboolean);
}

void JPBooleanType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	auto* b = (jboolean*) ((char*) memory + offset);
	frame.GetBooleanArrayRegion((jbooleanArray) a, start, len, b);
}

static void pack(jboolean* d, jvalue v)
{
	*d = v.z;
}

PyObject *JPBooleanType::newMultiArray(JPJavaFrame &frame, JPPyBuffer &buffer, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPBooleanType::newMultiArray");
	return convertMultiArray<type_t>(
			frame, this, &pack, "z",
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}
