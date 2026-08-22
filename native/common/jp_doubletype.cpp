// --- file: common/jp_doubletype.cpp ---
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
#include "jp_arrayclass.h"
#include "jp_classhints.h"
#include "jp_primitive_accessor.h"
#include "jp_doubletype.h"

JPDoubleType::JPDoubleType(JPJavaFrame& frame, jclass cls)
: JPPrimitiveType(frame, cls, "double")
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
	jobject jo = obj.getJavaObject(frame);
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
		if (match.getJPClass() == nullptr)
			return match.type = JPMatch::_none;
		match.type = JPMatch::_none;

		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (javaValueConversion->matches(cls, match)
				|| unboxConversion->matches(cls, match))
			return match.type;

		// Consider widening
		JPClass *cls2 = match.getJPClass();
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

	void getInfo(JPJavaFrame& frame, JPClass *cls, JPConversionInfo &info) override
	{
		JPContext *context = frame.getContext();
		PyList_Append(info.exact, (PyObject*) context->_double->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_byte->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_char->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_short->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_int->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_long->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_float->getHost());
		unboxConversion->getInfo(frame, cls, info);
	}
} asJDoubleConversion;

JPMatch::Type JPDoubleType::findJavaConversionImpl(JPMatch &match)
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

void JPDoubleType::getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info)
{
	asJDoubleConversion.getInfo(frame, this, info);
	asDoubleExactConversion.getInfo(frame, this, info);
	asDoubleLongConversion.getInfo(frame, this, info);
	asDoubleConversion.getInfo(frame, this, info);
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
	JPMatch match(frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java double");
	type_t val = field(match.convert());
	frame.SetStaticDoubleField(c, fid, val);
}

void JPDoubleType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match(frame, obj);
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
	if (tryFastBufferPush(frame, this, a, start, step, length, sequence))
		return;

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
			// This is PyBUF_FULL_RO, so suboffsets CAN legitimately be
			// non-null for a genuinely indirect exporter -- but every such
			// exporter found (CPython's own _testbuffer.ndarray, the only
			// one able to produce one at all; numpy/array/ctypes can't)
			// lacks __len__, and both call paths that reach here
			// (JPArray::setRange and JPConversionBuffer::matches) require
			// a working len() before ever getting this far. Kept as a
			// defensive fallback, not a provably-reachable path.
			if (view.suboffsets && view.suboffsets[0] >= 0)  // GCOVR_EXCL_LINE
				memory = *((char**) memory) + view.suboffsets[0];  // GCOVR_EXCL_LINE
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

	jsize index = start;

	// Container-kind dispatch happens once, not per element (list vs.
	// tuple vs. general sequence, resolved here); within each loop, the
	// exact-float/exact-int-or-neither check IS per element, deliberately
	// -- see JPFloatType::setArrayRange for the same pattern (this type
	// just stores the double directly, no narrowing cast needed). A
	// single item that's neither exact float nor exact int anywhere in
	// the sequence no longer demotes every element after it to the
	// generic PySequence_GetItem path.
	if (PyList_CheckExact(sequence))
	{
		for (Py_ssize_t i = 0; i < length; ++i, index += step)
		{
			PyObject *item = PyList_GET_ITEM(sequence, i);
			double v;
			if (PyFloat_CheckExact(item))
				v = PyFloat_AS_DOUBLE(item);
			else if (PyLong_CheckExact(item))
			{
				v = PyLong_AsDouble(item);
				if (v == -1.0 && PyErr_Occurred())
					JP_PY_CHECK();
			} else
			{
				v = PyFloat_AsDouble(item);
				if (v == -1.0 && PyErr_Occurred())
					JP_PY_CHECK();
			}
			val[index] = (type_t) v;
		}
	} else if (PyTuple_CheckExact(sequence))
	{
		for (Py_ssize_t i = 0; i < length; ++i, index += step)
		{
			PyObject *item = PyTuple_GET_ITEM(sequence, i);
			double v;
			if (PyFloat_CheckExact(item))
				v = PyFloat_AS_DOUBLE(item);
			else if (PyLong_CheckExact(item))
			{
				v = PyLong_AsDouble(item);
				if (v == -1.0 && PyErr_Occurred())
					JP_PY_CHECK();
			} else
			{
				v = PyFloat_AsDouble(item);
				if (v == -1.0 && PyErr_Occurred())
					JP_PY_CHECK();
			}
			val[index] = (type_t) v;
		}
	} else
	{
		JPPySequence seq = JPPySequence::use(sequence);
		for (Py_ssize_t i = 0; i < length; ++i, index += step)
		{
			type_t v = (type_t) PyFloat_AsDouble(seq[i].get());
			if (v == -1)
				JP_PY_CHECK();
			val[index] = v;
		}
	}
	accessor.commit();
	JP_TRACE_OUT;
}

void JPDoubleType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match(frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java double");
	type_t val = field(match.convert());
	frame.SetDoubleArrayRegion((array_t) a, ndx, 1, &val);
}

JPPyObject JPDoubleType::getFastArrayItem(JPJavaAccess& frame, jarray a, jsize ndx)
{
	// See JPFloatType::getFastArrayItem: inlines convertToPythonObject
	// directly, including its real (frame-free) slot write, rather than
	// calling it via a real JPJavaFrame& that would never actually be
	// used for anything JNI-related.
	auto array = (array_t) a;
	type_t val;
	frame.GetDoubleArrayRegion(array, ndx, 1, &val);
	PyTypeObject* wrapper = getHost();
	JPPyObject obj = JPPyObject::call(wrapper->tp_alloc(wrapper, 0));
	((PyFloatObject*) obj.get())->ob_fval = val;
	Py_ssize_t offset = PyJPValue_getJavaSlotOffset(obj.get());
	auto* slot = (jvalue*) (((char*) obj.get()) + offset);
	slot->d = val;
	return obj;
}

JPArray* JPDoubleType::createArrayWrapper(const JPValue& value)
{
	return new JPArrayDouble(value);
}

JPArrayClass* JPDoubleType::createArrayClass(JPJavaFrame& frame, jclass cls,
		const string& name, JPClass* superClass, jint modifiers)
{
	return new JPArrayClassDouble(frame, cls, name, superClass, this, modifiers);
}

JPMatch::Type JPArrayClassDouble::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPArrayClassDouble::findJavaConversion");
	if (nullConversion->matches(this, match)
			|| objectConversion->matches(this, match)
			|| bufferConversion->matches(this, match)
			|| listConversion->matches(this, match)
			|| tupleConversion->matches(this, match)
			|| sequenceConversion->matches(this, match)
			|| hintsConversion->matches(this, match)
			)
		return match.type;
	JP_TRACE("None");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPArrayClassDouble::getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info)
{
	objectConversion->getInfo(frame, this, info);
	bufferConversion->getInfo(frame, this, info);
	sequenceConversion->getInfo(frame, this, info);
	hintsConversion->getInfo(frame, this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}

JPArrayDouble::JPArrayDouble(const JPValue& array)
: JPArray(array), m_CompType(dynamic_cast<JPDoubleType*>(m_Class->getComponentType()))
{
}

JPArrayDouble::JPArrayDouble(JPArrayDouble* src, jsize start, jsize stop, jsize step)
: JPArray(src, start, stop, step), m_CompType(src->m_CompType)
{
}

JPPyObject JPArrayDouble::getItem(jsize ndx)
{
	ndx = checkIndex(ndx);
	JPJavaAccess frame(m_Context);
	JPJavaFrame jframe = JPJavaFrame::fast(frame.getEnv(), frame.getContext());
	// retrieveGlobal() is a JNI method call, so it mints a real local
	// reference -- fast() deliberately pushes no frame of its own (see its
	// ctor comment in jp_javaframe.cpp), and there is no enclosing real
	// frame on this single-element-access call path, so nothing else
	// reclaims it. JPLocalRef (RAII) releases it even if getItem(ndx,
	// resolved) below throws. Only paid here, on the single-index path
	// (ja[5]) -- PyJPArrayIter's per-element hot loop resolves the array
	// once, as a real global ref for the whole iterator, and calls
	// getItem(ndx, resolved) directly. See bugs/ArrayIterLocalRefLeak.md.
	JPLocalRef arr(jframe.getEnv(), jframe.retrieveGlobal(m_Object));
	return getItem(ndx, arr.get());
}

JPPyObject JPArrayDouble::getItem(jsize ndx, jobject resolved)
{
	JPJavaAccess frame(m_Context);
	return m_CompType->getFastArrayItem(frame, (jarray) resolved, m_Start + ndx * m_Step);
}

JPArray* JPArrayDouble::slice(jsize start, jsize stop, jsize step)
{
	return new JPArrayDouble(this, start, stop, step);
}

void JPDoubleType::getView(JPJavaFrame& frame, JPArrayView& view)
{
	view.m_Memory = (void*) frame.GetDoubleArrayElements(
			(jdoubleArray) view.m_Array->getJava(frame), &view.m_IsCopy);
	view.m_Buffer.format = "d";
	view.m_Buffer.itemsize = sizeof (jdouble);
}

void JPDoubleType::releaseView(JPJavaFrame& frame, JPArrayView& view)
{
	try
	{
		frame.ReleaseDoubleArrayElements((jdoubleArray) view.m_Array->getJava(frame),
				(jdouble*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (...)
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

void JPDoubleType::setElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		const void* memory, int offset)
{
	auto* b = (jdouble*) ((const char*) memory + offset);
	frame.SetDoubleArrayRegion((jdoubleArray) a, start, len, const_cast<jdouble*>(b));
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

jobject JPDoubleType::newMultiArrayObject(JPJavaFrame &frame, JPPyBuffer &buffer, jconverter converter, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPDoubleType::newMultiArrayObject");
	return convertMultiArrayObject<type_t>(
			frame, this, &pack, converter,
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}
