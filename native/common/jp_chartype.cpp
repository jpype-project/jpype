// --- file: common/jp_chartype.cpp ---
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
#include "jp_chartype.h"
#include "jp_boxedtype.h"

JPCharType::JPCharType(JPJavaFrame& frame, jclass cls)
: JPPrimitiveType(frame, cls, "char")
{
}

JPCharType::~JPCharType()
= default;

JPClass* JPCharType::getBoxedClass(JPJavaFrame& frame) const
{
	return frame.getContext()->_java_lang_Character;
}
	
JPValue JPCharType::newInstance(JPJavaFrame& frame, JPPyObjectVector& args)
{
	// This is only callable from one location so error checking is minimal
	if (args.size() != 1 || !PyIndex_Check(args[0]))
		JP_RAISE(PyExc_TypeError, "bad args");  // GCOVR_EXCL_LINE
	jvalue jv;

	// This is a cast so we must not fail
	int overflow;
	jv.c = PyLong_AsLongAndOverflow(args[0], &overflow);
	return JPValue(this, jv);
}

JPPyObject JPCharType::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast)
{
	//	if (!cast)
	//	{
	JPPyObject out = JPPyObject::call(PyJPChar_Create((PyTypeObject*) frame.getContext()->modulestate->JChar, val.c));
	PyJPValue_assignJavaSlot(frame, out.get(), JPValue(this, val));
	return out;
	//	}
	//	JPPyObject tmp = JPPyObject::call(PyLong_FromLong(field(val)));
	//	JPPyObject out = JPPyObject::call(convertLong(getHost(), (PyLongObject*) tmp.get()));
	//	return out;
}

JPValue JPCharType::getValueFromObject(JPJavaFrame& frame, const JPValue& obj)
{
	jvalue v;
	field(v) = frame.CallCharMethodA(obj.getJavaObject(frame), frame.getContext()->_java_lang_Character->m_CharValueID, nullptr);
	return JPValue(this, v);
}

class JPConversionAsChar : public JPConversion
{
	using base_t = JPCharType;
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match)  override
	{
		JP_TRACE_IN("JPConversionAsChar::matches");
		// checkCharUTF16 requires str/bytes of length exactly 1 -- depends
		// on the object's content/length, not just its Py_TYPE (e.g.
		// bytes([1]) matches but bytes([1, 1]) does not, despite both being
		// `bytes`).
		match.cacheable = false;
		if (!JPPyString::checkCharUTF16(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;  // GCOVR_EXCL_LINE
	}

	void getInfo(JPJavaFrame& frame, JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.implicit, (PyObject*) & PyUnicode_Type);
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		res.c = JPPyString::asCharUTF16(match.object);
		return res;
	}
} asCharConversion;

class JPConversionAsJChar : public JPConversionJavaValue
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match)  override
	{
		if (match.getJPClass() == nullptr)
			return match.type = JPMatch::_none;
		match.type = JPMatch::_none;

		// Exact
		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (javaValueConversion->matches(cls, match)
				|| unboxConversion->matches(cls, match))
			return match.type;

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return JPMatch::_implicit; // stop the search
	}

	void getInfo(JPJavaFrame& frame, JPClass *cls, JPConversionInfo &info) override
	{
		JPContext *context = frame.getContext();
		PyList_Append(info.exact, (PyObject*) context->_char->getHost());
		unboxConversion->getInfo(frame, cls, info);
	}

} asJCharConversion;

JPMatch::Type JPCharType::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPCharType::findJavaConversion");

	if (match.object == Py_None)
		return match.type = JPMatch::_none;

	if (asJCharConversion.matches(this, match)
			|| asCharConversion.matches(this, match))
		return match.type;

	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPCharType::getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info)
{
	asJCharConversion.getInfo(frame, this, info);
	asCharConversion.getInfo(frame, this, info);
	PyList_Append(info.ret, (PyObject*) frame.getContext()->_char->getHost());
}

jarray JPCharType::newArrayOf(JPJavaFrame& frame, jsize sz)
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
		if (clazz == nullptr)
			field(v) = frame.CallCharMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualCharMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

void JPCharType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match(frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java char");
	type_t val = field(match.convert());
	frame.SetStaticCharField(c, fid, val);
}

void JPCharType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match(frame, obj);
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
	jsize index = start;

	// Fast path: a plain list/tuple, avoiding PySequence_GetItem's generic
	// protocol dispatch in favor of PyList_GET_ITEM/PyTuple_GET_ITEM. No
	// per-element type/length branch needed beyond what asCharUTF16
	// already does -- matches()/sequenceCheck already validated every
	// element converts (length-1 string or JChar), so this is purely a
	// container-access optimization, not a widened-acceptance one.
	if (PyList_CheckExact(sequence))
	{
		for (Py_ssize_t i = 0; i < length; ++i, index += step)
		{
			jchar v = JPPyString::asCharUTF16(PyList_GET_ITEM(sequence, i));
			JP_PY_CHECK();
			val[index] = (type_t) v;
		}
	} else if (PyTuple_CheckExact(sequence))
	{
		for (Py_ssize_t i = 0; i < length; ++i, index += step)
		{
			jchar v = JPPyString::asCharUTF16(PyTuple_GET_ITEM(sequence, i));
			JP_PY_CHECK();
			val[index] = (type_t) v;
		}
	} else
	{
		JPPySequence seq = JPPySequence::use(sequence);
		for (Py_ssize_t i = 0; i < length; ++i, index += step)
		{
			jchar v = JPPyString::asCharUTF16(seq[i].get());
			JP_PY_CHECK();
			val[index] = (type_t) v;
		}
	}
	accessor.commit();
	JP_TRACE_OUT;
}

void JPCharType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match(frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java char");
	type_t val = field(match.convert());
	frame.SetCharArrayRegion((array_t) a, ndx, 1, &val);
}

JPPyObject JPCharType::getFastArrayItem(JPJavaAccess& frame, jarray a, jsize ndx)
{
	// See JPIntType::getFastArrayItem: inlines convertToPythonObject
	// directly -- PyJPValue_assignJavaSlot is a guaranteed no-op for this
	// family, so no frame is ever genuinely needed here.
	auto array = (array_t) a;
	type_t val;
	frame.GetCharArrayRegion(array, ndx, 1, &val);
	// _JChar (the bare process-wide global) is declared but never
	// assigned anywhere in the tree -- the live per-interpreter value is
	// st->JChar (see JPCharType::convertToPythonObject just above). Using
	// the always-null bare global here crashed PyJPChar_Create with a
	// null type pointer. frame.getContext() is this JPJavaAccess's own
	// captured context (see the caller, JPArrayChar::getItem()), not an
	// ambient global -- correct under multiple sub-interpreters.
	return JPPyObject::call(PyJPChar_Create(
			(PyTypeObject*) frame.getContext()->modulestate->JChar, val));
}

JPArray* JPCharType::createArrayWrapper(const JPValue& value)
{
	return new JPArrayChar(value);
}

JPArrayClass* JPCharType::createArrayClass(JPJavaFrame& frame, jclass cls,
		const string& name, JPClass* superClass, jint modifiers)
{
	return new JPArrayClassChar(frame, cls, name, superClass, this, modifiers);
}

JPMatch::Type JPArrayClassChar::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPArrayClassChar::findJavaConversion");
	if (nullConversion->matches(this, match)
			|| objectConversion->matches(this, match)
			|| charArrayConversion->matches(this, match)
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

void JPArrayClassChar::getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info)
{
	objectConversion->getInfo(frame, this, info);
	charArrayConversion->getInfo(frame, this, info);
	bufferConversion->getInfo(frame, this, info);
	sequenceConversion->getInfo(frame, this, info);
	hintsConversion->getInfo(frame, this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}

JPArrayChar::JPArrayChar(const JPValue& array)
: JPArray(array), m_CompType(dynamic_cast<JPCharType*>(m_Class->getComponentType()))
{
}

JPArrayChar::JPArrayChar(JPArrayChar* src, jsize start, jsize stop, jsize step)
: JPArray(src, start, stop, step), m_CompType(src->m_CompType)
{
}

JPPyObject JPArrayChar::getItem(jsize ndx)
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

JPPyObject JPArrayChar::getItem(jsize ndx, jobject resolved)
{
	JPJavaAccess frame(m_Context);
	return m_CompType->getFastArrayItem(frame, (jarray) resolved, m_Start + ndx * m_Step);
}

JPArray* JPArrayChar::slice(jsize start, jsize stop, jsize step)
{
	return new JPArrayChar(this, start, stop, step);
}

void JPCharType::getView(JPJavaFrame& frame, JPArrayView& view)
{
	view.m_Memory = (void*) frame.GetCharArrayElements(
			(jcharArray) view.m_Array->getJava(frame), &view.m_IsCopy);
	view.m_Buffer.format = "H";
	view.m_Buffer.itemsize = sizeof (jchar);
}

void JPCharType::releaseView(JPJavaFrame& frame, JPArrayView& view)
{
	try
	{
		frame.ReleaseCharArrayElements((jcharArray) view.m_Array->getJava(frame),
				(jchar*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (...)
	{
		// This is called as part of the cleanup routine and exceptions
		// are not permitted
	}
}

const char* JPCharType::getBufferFormat()
{
	return "H";
}

Py_ssize_t JPCharType::getItemSize()
{
	return sizeof (jchar);
}

void JPCharType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	auto* b = (jchar*) ((char*) memory + offset);
	frame.GetCharArrayRegion((jcharArray) a, start, len, b);
}

void JPCharType::setElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		const void* memory, int offset)
{
	auto* b = (jchar*) ((const char*) memory + offset);
	frame.SetCharArrayRegion((jcharArray) a, start, len, const_cast<jchar*>(b));
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

jobject JPCharType::newMultiArrayObject(JPJavaFrame &frame, JPPyBuffer &buffer, jconverter converter, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPCharType::newMultiArrayObject");
	return convertMultiArrayObject<type_t>(
			frame, this, &pack, converter,
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}
