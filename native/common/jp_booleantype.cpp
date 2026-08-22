// --- file: common/jp_booleantype.cpp ---
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
#include "jp_booleantype.h"
#include "jp_boxedtype.h"

JPBooleanType::JPBooleanType(JPJavaFrame& frame, jclass cls)
: JPPrimitiveType(frame, cls, "boolean")
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
	field(v) = frame.CallBooleanMethodA(obj.getJavaObject(frame), frame.getContext()->_java_lang_Boolean->m_BooleanValueID, nullptr) != 0;
	return JPValue(this, v);
}

class JPConversionAsBoolean : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		PyObject* obj = match.object;
		PyJPModuleState* st = match.frame->getContext()->modulestate;
		if (PyBool_Check(obj) || PyJP_IsInstanceSingle(obj, (PyTypeObject*) st->numpy_bool_type))
		{
			match.conversion = this;
			return match.type = JPMatch::_exact;
		}

		return match.type = JPMatch::_none;
	}

	void getInfo(JPJavaFrame& frame, JPClass * cls, JPConversionInfo &info) override
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

		if (match.getJPClass() == nullptr)
			return match.type = JPMatch::_none;
		match.type = JPMatch::_none;
		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (javaValueConversion->matches(cls, match)
				|| unboxConversion->matches(cls, match))
			return match.type;

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return JPMatch::_implicit; // search no further.
	}

	void getInfo(JPJavaFrame& frame, JPClass *cls, JPConversionInfo &info) override
	{
		JPContext *context = frame.getContext();
		PyList_Append(info.exact, (PyObject*) context->_boolean->getHost());
		unboxConversion->getInfo(frame, cls, info);
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

	void getInfo(JPJavaFrame& frame, JPClass *cls, JPConversionInfo &info) override
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

	void getInfo(JPJavaFrame& frame, JPClass * cls, JPConversionInfo &info) override
	{
		PyObject *typing = PyImport_AddModule("jpype.protocol");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "SupportsFloat"));
		PyList_Append(info.expl, proto.get());
	}

} asBooleanNumber;

JPMatch::Type JPBooleanType::findJavaConversionImpl(JPMatch &match)
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

void JPBooleanType::getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info)
{
	asBooleanExact.getInfo(frame, this, info);
	asBooleanJBool.getInfo(frame, this, info);
	asBooleanLong.getInfo(frame, this, info);
	asBooleanNumber.getInfo(frame, this, info);
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
	JPMatch match(frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java boolean");
	type_t val = field(match.convert());
	frame.SetStaticBooleanField(c, fid, val);
}

void JPBooleanType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match(frame, obj);
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
	if (tryFastBufferPush(frame, this, a, start, step, length, sequence))
		return;

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

	jsize index = start;

	// Container-kind dispatch happens once, not per element (list vs.
	// tuple vs. general sequence, resolved here); within each loop, the
	// exact-bool-or-not check IS per element, deliberately -- a single
	// cheap PyBool_Check, the same cost sequenceCheckStep (jp_class.cpp)
	// already pays per element during matches(). A single non-bool item
	// anywhere in the sequence no longer demotes every element after it
	// to the generic PySequence_GetItem + PyObject_IsTrue path -- only
	// that one element pays the general truthiness call.
	if (PyList_CheckExact(sequence))
	{
		for (Py_ssize_t i = 0; i < length; ++i, index += step)
		{
			PyObject *item = PyList_GET_ITEM(sequence, i);
			if (PyBool_Check(item))
				val[index] = (type_t) (item == Py_True);
			else
			{
				int v = PyObject_IsTrue(item);
				if (v == -1)
					JP_PY_CHECK();
				val[index] = (type_t) v;
			}
		}
	} else if (PyTuple_CheckExact(sequence))
	{
		for (Py_ssize_t i = 0; i < length; ++i, index += step)
		{
			PyObject *item = PyTuple_GET_ITEM(sequence, i);
			if (PyBool_Check(item))
				val[index] = (type_t) (item == Py_True);
			else
			{
				int v = PyObject_IsTrue(item);
				if (v == -1)
					JP_PY_CHECK();
				val[index] = (type_t) v;
			}
		}
	} else
	{
		JPPySequence seq = JPPySequence::use(sequence);
		for (Py_ssize_t i = 0; i < length; ++i, index += step)
		{
			int v = PyObject_IsTrue(seq[i].get());
			if (v == -1)
				JP_PY_CHECK();
			val[index] = (type_t) v;
		}
	}
	accessor.commit();
	JP_TRACE_OUT;
}

void JPBooleanType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match(frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java boolean");
	type_t val = field(match.convert());
	frame.SetBooleanArrayRegion((array_t) a, ndx, 1, &val);
}

JPPyObject JPBooleanType::getFastArrayItem(JPJavaAccess& frame, jarray a, jsize ndx)
{
	// Unlike the other seven primitives, convertToPythonObject here is
	// PyBool_FromLong(val.z) alone -- no getHost()/convertLong, no
	// PyJPValue_assignJavaSlot call at all -- so this is genuinely
	// frame-free, not just frame-unused: no JPJavaFrame is ever touched.
	auto array = (array_t) a;
	type_t val;
	frame.GetBooleanArrayRegion(array, ndx, 1, &val);
	return JPPyObject::call(PyBool_FromLong(val));
}

JPArray* JPBooleanType::createArrayWrapper(const JPValue& value)
{
	return new JPArrayBoolean(value);
}

JPArrayClass* JPBooleanType::createArrayClass(JPJavaFrame& frame, jclass cls,
		const string& name, JPClass* superClass, jint modifiers)
{
	return new JPArrayClassBoolean(frame, cls, name, superClass, this, modifiers);
}

JPMatch::Type JPArrayClassBoolean::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPArrayClassBoolean::findJavaConversion");
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

void JPArrayClassBoolean::getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info)
{
	objectConversion->getInfo(frame, this, info);
	bufferConversion->getInfo(frame, this, info);
	sequenceConversion->getInfo(frame, this, info);
	hintsConversion->getInfo(frame, this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}

JPArrayBoolean::JPArrayBoolean(const JPValue& array)
: JPArray(array), m_CompType(dynamic_cast<JPBooleanType*>(m_Class->getComponentType()))
{
}

JPArrayBoolean::JPArrayBoolean(JPArrayBoolean* src, jsize start, jsize stop, jsize step)
: JPArray(src, start, stop, step), m_CompType(src->m_CompType)
{
}

JPPyObject JPArrayBoolean::getItem(jsize ndx)
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

JPPyObject JPArrayBoolean::getItem(jsize ndx, jobject resolved)
{
	JPJavaAccess frame(m_Context);
	return m_CompType->getFastArrayItem(frame, (jarray) resolved, m_Start + ndx * m_Step);
}

JPArray* JPArrayBoolean::slice(jsize start, jsize stop, jsize step)
{
	return new JPArrayBoolean(this, start, stop, step);
}

void JPBooleanType::getView(JPJavaFrame& frame, JPArrayView& view)
{
	view.m_Memory = (void*) frame.GetBooleanArrayElements(
			(jbooleanArray) view.m_Array->getJava(frame), &view.m_IsCopy);
	view.m_Buffer.format = "?";
	view.m_Buffer.itemsize = sizeof (jboolean);
}

void JPBooleanType::releaseView(JPJavaFrame& frame, JPArrayView& view)
{
	try
	{
		frame.ReleaseBooleanArrayElements((jbooleanArray) view.m_Array->getJava(frame),
				(jboolean*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (...)
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

void JPBooleanType::setElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		const void* memory, int offset)
{
	auto* b = (jboolean*) ((const char*) memory + offset);
	frame.SetBooleanArrayRegion((jbooleanArray) a, start, len, const_cast<jboolean*>(b));
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

jobject JPBooleanType::newMultiArrayObject(JPJavaFrame &frame, JPPyBuffer &buffer, jconverter converter, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPBooleanType::newMultiArrayObject");
	return convertMultiArrayObject<type_t>(
			frame, this, &pack, converter,
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}
