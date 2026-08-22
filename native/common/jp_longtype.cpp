// --- file: common/jp_longtype.cpp ---
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
#include "jp_longtype.h"

JPLongType::JPLongType(JPJavaFrame& frame, jclass cls)
: JPPrimitiveType(frame, cls, "long")
{
}

JPLongType::~JPLongType()
= default;

JPClass* JPLongType::getBoxedClass(JPJavaFrame& frame) const
{
	return frame.getContext()->_java_lang_Long;
}

JPPyObject JPLongType::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast)
{
	JPPyObject out = JPPyObject::call(convertLong(getHost(), field(val)));
	PyJPValue_assignJavaSlot(frame, out.get(), JPValue(this, val));
	return out;
}

JPValue JPLongType::getValueFromObject(JPJavaFrame& frame, const JPValue& obj)
{
	jvalue v;
	jobject jo = obj.getJavaObject(frame);
	auto* jb = dynamic_cast<JPBoxedType*>( frame.findClassForObject(jo));
	field(v) = (type_t) frame.CallLongMethodA(jo, jb->m_LongValueID, nullptr);
	return JPValue(this, v);
}

JPConversionLong<JPLongType> longConversion;
JPConversionLongNumber<JPLongType> longNumberConversion;
JPConversionLongWiden<JPLongType> longWidenConversion;

class JPConversionJLong : public JPConversionJavaValue
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (match.getJPClass() == nullptr)
			return match.type = JPMatch::_none;

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
				case 'I':
				case 'C':
				case 'S':
				case 'B':
					match.conversion = &longWidenConversion;
					return match.type = JPMatch::_implicit;
				default:
					break;
			}
		}

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		match.type = JPMatch::_none;
		return JPMatch::_implicit;
	}

	void getInfo(JPJavaFrame& frame, JPClass *cls, JPConversionInfo &info) override
	{
		JPContext *context = frame.getContext();
		PyList_Append(info.exact, (PyObject*) context->_long->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_byte->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_char->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_short->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_int->getHost());
		unboxConversion->getInfo(frame, cls, info);
	}
} jlongConversion;

JPMatch::Type JPLongType::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPLongType::findJavaConversion");

	if (match.object == Py_None)
		return match.type = JPMatch::_none;


	if (jlongConversion.matches(this, match)
			|| longConversion.matches(this, match)
			|| longNumberConversion.matches(this, match))
		return match.type;

	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPLongType::getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info)
{
	jlongConversion.getInfo(frame, this, info);
	longConversion.getInfo(frame, this, info);
	longNumberConversion.getInfo(frame, this, info);
	PyList_Append(info.ret, (PyObject*) frame.getContext()->_long->getHost());
}

jarray JPLongType::newArrayOf(JPJavaFrame& frame, jsize sz)
{
	return frame.NewLongArray(sz);
}

JPPyObject JPLongType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticLongField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPLongType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetLongField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPLongType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticLongMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPLongType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == nullptr)
			field(v) = frame.CallLongMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualLongMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

void JPLongType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match(frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java int");
	type_t val = field(match.convert());
	frame.SetStaticLongField(c, fid, val);
}

void JPLongType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match(frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java int");
	type_t val = field(match.convert());
	frame.SetLongField(c, fid, val);
}

void JPLongType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPLongType::setArrayRange");
	if (tryFastBufferPush(frame, this, a, start, step, length, sequence))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);

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
			jconverter conv = getConverter(view.format, (int) view.itemsize, "j");
			for (Py_ssize_t i = 0; i < length; ++i, index += step)
			{
				jvalue r = conv(memory);
				val[index] = r.j;
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
	// tuple vs. general sequence, resolved here). The value conversion
	// itself (PyLong_AsLongLong) is identical whether the item is an
	// exact int or not -- unlike byte/short/int, there's no narrower
	// PyLong_AsLong to prefer, since jlong is already the widest integer
	// type PyLong_As* offers. So the only per-element saving available is
	// skipping PyIndex_Check's generic-protocol probe for the (common)
	// exact-int case, done inline below rather than as a separate branch
	// that would otherwise demote every element after the first non-exact
	// one to the general PySequence_GetItem path.
	if (PyList_CheckExact(sequence))
	{
		for (Py_ssize_t i = 0; i < length; ++i, index += step)
		{
			PyObject *item = PyList_GET_ITEM(sequence, i);
			if (!PyLong_CheckExact(item) && !PyIndex_Check(item))
			{
				PyErr_Format(PyExc_TypeError, "Unable to implicitly convert '%s' to long", Py_TYPE(item)->tp_name);
				JP_RAISE_PYTHON();
			}
			jlong v = PyLong_AsLongLong(item);
			if (v == -1)
				JP_PY_CHECK();
			val[index] = (type_t) v;
		}
	} else if (PyTuple_CheckExact(sequence))
	{
		for (Py_ssize_t i = 0; i < length; ++i, index += step)
		{
			PyObject *item = PyTuple_GET_ITEM(sequence, i);
			if (!PyLong_CheckExact(item) && !PyIndex_Check(item))
			{
				PyErr_Format(PyExc_TypeError, "Unable to implicitly convert '%s' to long", Py_TYPE(item)->tp_name);
				JP_RAISE_PYTHON();
			}
			jlong v = PyLong_AsLongLong(item);
			if (v == -1)
				JP_PY_CHECK();
			val[index] = (type_t) v;
		}
	} else
	{
		JPPySequence seq = JPPySequence::use(sequence);
		for (Py_ssize_t i = 0; i < length; ++i, index += step)
		{
			PyObject *item = seq[i].get();
			if (!PyIndex_Check(item))
			{
				PyErr_Format(PyExc_TypeError, "Unable to implicitly convert '%s' to long", Py_TYPE(item)->tp_name);
				JP_RAISE_PYTHON();
			}
			jlong v = PyLong_AsLongLong(item);
			if (v == -1)
				JP_PY_CHECK();
			val[index] = (type_t) v;
		}
	}
	accessor.commit();
	JP_TRACE_OUT;
}

void JPLongType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match(frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java int");
	type_t val = field(match.convert());
	frame.SetLongArrayRegion((array_t) a, ndx, 1, &val);
}

JPPyObject JPLongType::getFastArrayItem(JPJavaAccess& frame, jarray a, jsize ndx)
{
	// See JPIntType::getFastArrayItem: inlines convertToPythonObject
	// directly -- PyJPValue_assignJavaSlot is a guaranteed no-op for this
	// family, so no frame is ever genuinely needed here.
	auto array = (array_t) a;
	type_t val;
	frame.GetLongArrayRegion(array, ndx, 1, &val);
	return JPPyObject::call(convertLong(getHost(), val));
}

JPArray* JPLongType::createArrayWrapper(const JPValue& value)
{
	return new JPArrayLong(value);
}

JPArrayClass* JPLongType::createArrayClass(JPJavaFrame& frame, jclass cls,
		const string& name, JPClass* superClass, jint modifiers)
{
	return new JPArrayClassLong(frame, cls, name, superClass, this, modifiers);
}

JPMatch::Type JPArrayClassLong::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPArrayClassLong::findJavaConversion");
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

void JPArrayClassLong::getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info)
{
	objectConversion->getInfo(frame, this, info);
	bufferConversion->getInfo(frame, this, info);
	sequenceConversion->getInfo(frame, this, info);
	hintsConversion->getInfo(frame, this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}

JPArrayLong::JPArrayLong(const JPValue& array)
: JPArray(array), m_CompType(dynamic_cast<JPLongType*>(m_Class->getComponentType()))
{
}

JPArrayLong::JPArrayLong(JPArrayLong* src, jsize start, jsize stop, jsize step)
: JPArray(src, start, stop, step), m_CompType(src->m_CompType)
{
}

JPPyObject JPArrayLong::getItem(jsize ndx)
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

JPPyObject JPArrayLong::getItem(jsize ndx, jobject resolved)
{
	JPJavaAccess frame(m_Context);
	return m_CompType->getFastArrayItem(frame, (jarray) resolved, m_Start + ndx * m_Step);
}

JPArray* JPArrayLong::slice(jsize start, jsize stop, jsize step)
{
	return new JPArrayLong(this, start, stop, step);
}

void JPLongType::getView(JPJavaFrame& frame, JPArrayView& view)
{
	view.m_Memory = (void*) frame.GetLongArrayElements(
			(jlongArray) view.m_Array->getJava(frame), &view.m_IsCopy);
	view.m_Buffer.format = "=q";
	view.m_Buffer.itemsize = sizeof (jlong);
}

void JPLongType::releaseView(JPJavaFrame& frame, JPArrayView& view)
{
	try
	{
		frame.ReleaseLongArrayElements((jlongArray) view.m_Array->getJava(frame),
				(jlong*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (...)
	{
		// This is called as part of the cleanup routine and exceptions
		// are not permitted
	}
}

const char* JPLongType::getBufferFormat()
{
	return "=q";
}

Py_ssize_t JPLongType::getItemSize()
{
	return sizeof (jlong);
}

void JPLongType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	auto* b = (jlong*) ((char*) memory + offset);
	frame.GetLongArrayRegion((jlongArray) a, start, len, b);
}

void JPLongType::setElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		const void* memory, int offset)
{
	auto* b = (jlong*) ((const char*) memory + offset);
	frame.SetLongArrayRegion((jlongArray) a, start, len, const_cast<jlong*>(b));
}

static void pack(jlong* d, jvalue v)
{
	*d = v.j;
}

PyObject *JPLongType::newMultiArray(JPJavaFrame &frame, JPPyBuffer &buffer, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPLongType::newMultiArray");
	return convertMultiArray<type_t>(
			frame, this, &pack, "j",
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}

jobject JPLongType::newMultiArrayObject(JPJavaFrame &frame, JPPyBuffer &buffer, jconverter converter, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPLongType::newMultiArrayObject");
	return convertMultiArrayObject<type_t>(
			frame, this, &pack, converter,
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}
