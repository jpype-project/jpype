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
#include "jp_doubletype.h"

JPDoubleType::JPDoubleType()
: JPPrimitiveType("double")
{
}

JPClass* JPDoubleType::getBoxedClass(JPJavaFrame& frame) const
{
	return frame.getContext()->_java_lang_Double;
}

JPPyObject JPDoubleType::convertToPythonObject(JPJavaFrame& frame, jvalue value, bool cast)
{
	PyTypeObject * wrapper = getHost();
	JPPyObject obj = JPPyObject::call(wrapper->tp_alloc(wrapper, 0));
	((PyFloatObject*) obj.get())->ob_fval = value.d;
	PyJPValue_assignJavaSlot(frame, obj.get(), JPValue(this, value));
	return obj;
}

JPValue JPDoubleType::getValueFromObject(JPJavaFrame& frame, const JPValue& obj)
{
	jvalue v;
	jobject jo = obj.getValue().l;
	auto* jb = dynamic_cast<JPBoxedType*>( frame.findClassForObject(jo));
	field(v) = (type_t) frame.CallDoubleMethodA(jo, jb->m_DoubleValueID, nullptr);
	return JPValue(this, v);
}

static JPConversionAsFloat<JPDoubleType> asDoubleConversion;
static JPConversionLongAsFloat<JPDoubleType> asDoubleLongConversion;
static JPConversionFloatWiden<JPDoubleType> doubleWidenConversion;

class JPConversionAsDoubleExact : public JPConversionAsFloat<JPDoubleType>
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (!PyFloat_CheckExact(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_exact;
	}

} asDoubleExactConversion;

class JPConversionAsJDouble : public JPConversionJavaValue
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

		// Consider widening
		JPClass *cls2 = value->getClass();
		if (cls2->isPrimitive())
		{
			// https://docs.oracle.com/javase/specs/jls/se7/html/jls-5.html#jls-5.1.2
			auto *prim = dynamic_cast<JPPrimitiveType*>( cls2);
			switch (prim->getTypeCode())
			{
				case 'B':
				case 'S':
				case 'C':
				case 'I':
				case 'J':
				case 'F':
					match.conversion = &doubleWidenConversion;
					return match.type = JPMatch::_implicit;
				default:
					break;
			}
		}

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return JPMatch::_implicit;

	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		JPContext *context = JPContext_global;
		PyList_Append(info.exact, (PyObject*) context->_double->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_byte->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_char->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_short->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_int->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_long->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_float->getHost());
		unboxConversion->getInfo(cls, info);
	}
} asJDoubleConversion;

JPMatch::Type JPDoubleType::findJavaConversion(JPMatch &match)
{
	JP_TRACE_IN("JPDoubleType::findJavaConversion");

	if (match.object == Py_None)
		return match.type = JPMatch::_none;

	if (asJDoubleConversion.matches(this, match)
			|| asDoubleExactConversion.matches(this, match)
			|| asDoubleLongConversion.matches(this, match)
			|| asDoubleConversion.matches(this, match))
		return match.type;

	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPDoubleType::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	asJDoubleConversion.getInfo(this, info);
	asDoubleExactConversion.getInfo(this, info);
	asDoubleLongConversion.getInfo(this, info);
	asDoubleConversion.getInfo(this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}

jarray JPDoubleType::newArrayOf(JPJavaFrame& frame, jsize sz)
{
	return frame.NewDoubleArray(sz);
}

JPPyObject JPDoubleType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticDoubleField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPDoubleType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetDoubleField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPDoubleType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticDoubleMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPDoubleType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == nullptr)
			field(v) = frame.CallDoubleMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualDoubleMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

void JPDoubleType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java double");
	type_t val = field(match.convert());
	frame.SetStaticDoubleField(c, fid, val);
}

void JPDoubleType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java double");
	type_t val = field(match.convert());
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
	JPPySequence seq = JPPySequence::use(sequence);
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
	auto array = (array_t) a;
	type_t val;
	frame.GetDoubleArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(frame, v, false);
}

void JPDoubleType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java double");
	type_t val = field(match.convert());
	frame.SetDoubleArrayRegion((array_t) a, ndx, 1, &val);
}

void JPDoubleType::getView(JPArrayView& view)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	view.m_Memory = (void*) frame.GetDoubleArrayElements(
			(jdoubleArray) view.m_Array->getJava(), &view.m_IsCopy);
	view.m_Buffer.format = "d";
	view.m_Buffer.itemsize = sizeof (jdouble);
}

void JPDoubleType::releaseView(JPArrayView& view)
{
	try
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		frame.ReleaseDoubleArrayElements((jdoubleArray) view.m_Array->getJava(),
				(jdouble*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (JPypeException&)
	{
		// This is called as part of the cleanup routine and exceptions
		// are not permitted
	}
}

const char* JPDoubleType::getBufferFormat()
{
	return "d";
}

Py_ssize_t JPDoubleType::getItemSize()
{
	return sizeof (jdouble);
}

void JPDoubleType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	auto* b = (jdouble*) ((char*) memory + offset);
	frame.GetDoubleArrayRegion((jdoubleArray) a, start, len, b);
}

static void pack(jdouble* d, jvalue v)
{
	*d = v.d;
}

PyObject *JPDoubleType::newMultiArray(JPJavaFrame &frame, JPPyBuffer &buffer, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPDoubleType::newMultiArray");
	return convertMultiArray<type_t>(
			frame, this, &pack, "d",
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}
