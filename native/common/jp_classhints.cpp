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
#include <cctype>
#include <utility>

#include <Python.h>
#include "jpype.h"
#include "jp_arrayclass.h"
#include "jp_classhints.h"
#include "jp_proxy.h"
#include "jp_stringtype.h"

#include "pyjp.h"

#include "jp_primitive_accessor.h"
#include "jp_inttype.h"
#include "jp_bytetype.h"
#include "jp_shorttype.h"

JPMatch::JPMatch() : conversion(nullptr), frame(nullptr), object(nullptr),
					 type(JPMatch::_none), closure(nullptr), cacheable(true),
					 m_SlotResolved(false), m_SlotClass(nullptr), m_SlotValue()
{}

JPMatch::JPMatch(JPJavaFrame *fr, PyObject *obj) : conversion(nullptr), frame(fr), object(obj),
												   type(JPMatch::_none), closure(nullptr), cacheable(true),
												   m_SlotResolved(false), m_SlotClass(nullptr), m_SlotValue()
{}

void JPMatch::resolveSlot()
{
	if (m_SlotResolved)
		return;
	m_SlotResolved = true;
	m_SlotClass = PyJPValue_getJPClass(object);
	if (m_SlotClass != nullptr && frame != nullptr)
		m_SlotValue = PyJPValue_getJValue(*frame, object);
}

JPClass *JPMatch::getJPClass()
{
	resolveSlot();
	return m_SlotClass;
}

jvalue JPMatch::getJValue()
{
	resolveSlot();
	return m_SlotValue;
}

jvalue JPMatch::convert()
{
	// Sanity check, this should not happen
	if (conversion == nullptr)
		JP_RAISE(PyExc_SystemError, "Fail in conversion"); // GCOVR_EXCL_LINE
	return conversion->convert(*this);
}

JPMethodMatch::JPMethodMatch(JPJavaFrame &frame, JPPyObjectVector& args, bool callInstance)
	: m_Arguments(args.size()), m_Type(JPMatch::_none), m_IsVarIndirect(false),// m_Overload(nullptr),
	  m_Offset(0), m_Skip(0)
{
	m_Hash = callInstance ? 0 : 1000;
	for (size_t i = 0; i < args.size(); ++i)
	{
		PyObject *arg = args[i];
		m_Arguments[i] = JPMatch(&frame, arg);

		// This is an LCG used to compute a hash code for the incoming
		// arguments using (A*X+A_i) mod2^64 where A_i is the address of each
		// type the argument list.  The hash will be checked to avoid needing
		// to resolve the method if the same overload is called twice. There
		// is only a speed cost if there is a collision, so we don't need to
		// prove this is a perfect hash function.
		m_Hash *= 0x10523C01;
		PyTypeObject* type = Py_TYPE(arg);
		m_Hash += (long)type;

		// Specialized fast-check for functional types
		if (type == &PyFunction_Type)
		{
			// A single pointer dereference is much faster than PyFunction_GetCode
			// co_argcount is at a fixed offset in the code object.
			PyObject* code = ((PyFunctionObject*)arg)->func_code;
			m_Hash ^= (long)code; 
		}
		else if (type == &PyMethod_Type)
		{
			PyObject* func = ((PyMethodObject*)arg)->im_func;
			m_Hash ^= (long)func;
		}
	}
}

JPConversion::~JPConversion() = default;

uint64_t JPClassHints::s_Generation = 1;

JPClassHints::JPClassHints()
{
	m_ConvertJava = false;
}

JPClassHints::~JPClassHints()
{
	for (auto & conversion : conversions)
	{
		delete conversion;
	}
	conversions.clear();
}

JPMatch::Type JPClassHints::getConversion(JPMatch& match, JPClass *cls)
{
	JPConversion *best = nullptr;
	for (auto & conversion : conversions)
	{
		JPMatch::Type quality = conversion->matches(cls, match);
		if (quality > JPMatch::_explicit)
			return match.type;
		if (quality != JPMatch::_none)
			best = conversion;
	}
	match.conversion = best;
	if (best == nullptr)
		return match.type = JPMatch::_none;
	return match.type = JPMatch::_explicit;
}

void JPIndexConversion::getInfo(JPClass *cls, JPConversionInfo &info)
{
	PyObject *typing = PyImport_AddModule("jpype.protocol");
	JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "SupportsIndex"));
	PyList_Append(info.implicit, proto.get());
}

void JPNumberConversion::getInfo(JPClass *cls, JPConversionInfo &info)
{
	JPIndexConversion::getInfo(cls, info);
	PyObject *typing = PyImport_AddModule("jpype.protocol");
	JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "SupportsFloat"));
	PyList_Append(info.implicit, proto.get());
}

/**
 * Conversion for all user specified conversions.
 */
class JPPythonConversion : public JPConversion
{
public:

	explicit JPPythonConversion(PyObject *method)
	{
		method_ = JPPyObject::use(method);
	}

	~JPPythonConversion() override = default;

	jvalue convert(JPMatch &match) override
	{
		JP_TRACE_IN("JPPythonConversion::convert");
		JPClass *cls = ((JPClass*) match.closure);
		JPPyObject args = JPPyTuple_Pack(cls->getHost(), match.object);
		JPPyObject ret = JPPyObject::call(PyObject_Call(method_.get(), args.get(), nullptr));
		JPClass *retCls = PyJPValue_getJPClass(ret.get());
		if (retCls != nullptr)
		{
			jvalue v = PyJPValue_getJValue(*match.frame, ret.get());
			JP_TRACE("Value", v.l);
			v.l = match.frame->NewLocalRef(v.l);
			return v;
		}
		JPProxy *proxy = PyJPProxy_getJPProxy(ret.get());
		if (proxy != nullptr)
		{
			jvalue v = proxy->getProxy();
			JP_TRACE("Proxy", v.l);
			v.l = match.frame->NewLocalRef(v.l);
			return v;
		}
		JP_RAISE(PyExc_TypeError, "Bad type conversion");
		JP_TRACE_OUT;
	}
private:

	JPPyObject method_;
} ;

//<editor-fold desc="attribute conversion" defaultstate="collapsed">

class JPAttributeConversion : public JPPythonConversion
{
public:

	JPAttributeConversion(string attribute, PyObject *method)
	: JPPythonConversion(method), attribute_(std::move(attribute))
	{
	}

	~JPAttributeConversion() override = default;

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPAttributeConversion::matches");
		// Duck-typed attribute presence is always instance-dependent (e.g.
		// __getattr__, per-instance attributes) -- never safe to cache by
		// Py_TYPE(object) alone.
		match.cacheable = false;
		JPPyObject attr = JPPyObject::accept(PyObject_GetAttrString(match.object, attribute_.c_str()));
		if (attr.isNull())
			return JPMatch::_none;
		match.conversion = this;
		match.closure = cls;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.attributes, JPPyString::fromStringUTF8(attribute_).get());
	}


private:
	std::string attribute_;

} ;

void JPClassHints::addAttributeConversion(const string &attribute, PyObject *conversion)
{
	JP_TRACE_IN("JPClassHints::addAttributeConversion", this);
	JP_TRACE(attribute);
	conversions.push_back(new JPAttributeConversion(attribute, conversion));
	++s_Generation;
	JP_TRACE_OUT;
}

//</editor-fold>
//<editor-fold desc="type conversion" defaultstate="collapsed">

class JPNoneConversion : public JPConversion
{
public:

	explicit JPNoneConversion(PyObject *type)
	{
		type_ = JPPyObject::use(type);
	}

	~JPNoneConversion() override
	= default;

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPTypeConversion::matches");
		if (!PyObject_IsInstance(match.object, type_.get()))
			return JPMatch::_none;
		match.conversion = this;
		match.type = JPMatch::_none;
		return JPMatch::_implicit; // Prevent further searching
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.none, type_.get());
	}

	jvalue convert(JPMatch &match) override
	{
		return jvalue();
	}

private:
	JPPyObject type_;
} ;

class JPTypeConversion : public JPPythonConversion
{
public:

	JPTypeConversion(PyObject *type, PyObject *method, bool exact)
	: JPPythonConversion(method), exact_(exact)
	{
		type_ = JPPyObject::use(type);
	}

	~JPTypeConversion() override
	= default;

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPTypeConversion::matches");
		if ((exact_ && ((PyObject*) Py_TYPE(match.object)) == type_.get())
				|| PyObject_IsInstance(match.object, type_.get()))
		{
			match.closure = cls;
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return JPMatch::_none;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.implicit, type_.get());
	}

private:
	JPPyObject type_;
	bool exact_;
} ;

void JPClassHints::addTypeConversion(PyObject *type, PyObject *method, bool exact)
{
	JP_TRACE_IN("JPClassHints::addTypeConversion", this);
	if (PyJPClass_Check(type))
		m_ConvertJava = true;
	conversions.push_back(new JPTypeConversion(type, method, exact));
	++s_Generation;
	JP_TRACE_OUT;
}

void JPClassHints::excludeConversion(PyObject *type)
{
	JP_TRACE_IN("JPClassHints::addTypeConversion", this);
	conversions.push_front(new JPNoneConversion(type));
	++s_Generation;
	JP_TRACE_OUT;
}

void JPClassHints::getInfo(JPClass *cls, JPConversionInfo &info)
{
	for (auto iter = conversions.begin();
			iter != conversions.end(); ++iter)
	{
		(*iter)->getInfo(cls, info);
	}
}

class JPHintsConversion : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		auto *pyhints = (PyJPClassHints*) cls->getHints();
		JPClassHints *hints = pyhints->m_Hints;
		hints->getConversion(match, cls);
		return match.type;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		auto *pyhints = (PyJPClassHints*) cls->getHints();
		JPClassHints *hints = pyhints->m_Hints;
		hints->getInfo(cls, info);
	}

	jvalue convert(JPMatch &match) override
	{
		return jvalue();
	}
} _hintsConversion;

//</editor-fold>

class JPConversionCharArray : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionCharArray::matches");
		auto* acls = dynamic_cast<JPArrayClass*>( cls);
		if (match.frame == nullptr  || !JPPyString::check(match.object) ||
				acls->getComponentType() != JPContext_global->_char)
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		auto* acls = dynamic_cast<JPArrayClass*>( cls);
		if (acls->getComponentType() != JPContext_global->_char)
			return;
		PyList_Append(info.implicit, (PyObject*) & PyUnicode_Type);
	}

	jvalue convert(JPMatch &match) override
	{
		JPJavaFrame *frame = match.frame;
		JP_TRACE("char[]");
		jvalue res;

		// Convert to a string
		string str = JPPyString::asStringUTF8(match.object);

		// Convert to new java string
		jstring jstr = frame->fromStringUTF8(str);

		// call toCharArray()
		res.l = frame->toCharArray(jstr);
		return res;
	}
} _charArrayConversion;

class JPConversionByteArray : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionByteArray::matches");
		auto* acls = dynamic_cast<JPArrayClass*>( cls);
		if (match.frame == nullptr ||
				acls->getComponentType() != JPContext_global->_byte)
			return match.type = JPMatch::_none;

		// Check for bytes first - implicit conversion
		if (PyBytes_Check(match.object))
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}

		// Check for bytearray - derived match for byte[] (issue #598)
		// bytearray is semantically a mutable byte array, so it should
		// match byte[] better than char[] (which gets _implicit)
		// Use _derived instead of _exact to avoid caching issues
		if (PyByteArray_Check(match.object))
		{
			match.conversion = this;
			return match.type = JPMatch::_derived;
		}

		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		auto* acls = dynamic_cast<JPArrayClass*>( cls);
		if (acls->getComponentType() != JPContext_global->_byte)
			return;
		PyList_Append(info.implicit, (PyObject*) & PyBytes_Type);
		PyList_Append(info.implicit, (PyObject*) & PyByteArray_Type);
	}

	jvalue convert(JPMatch &match) override
	{
		JPJavaFrame frame(*match.frame);
		jvalue res;
		Py_ssize_t size = 0;
		char *buffer = nullptr;

		// Handle both bytes and bytearray
		// matches() already verified it's one of these
		if (PyBytes_Check(match.object))
		{
			PyBytes_AsStringAndSize(match.object, &buffer, &size); // internal reference
		}
		else // PyByteArray_Check
		{
			size = PyByteArray_GET_SIZE(match.object);
			buffer = PyByteArray_AS_STRING(match.object); // internal reference
		}

		jbyteArray byteArray = frame.NewByteArray((jsize) size);
		frame.SetByteArrayRegion(byteArray, 0, (jsize) size, (jbyte*) buffer);
		res.l = frame.keep(byteArray);
		return res;
	}
} _byteArrayConversion;

class JPConversionBuffer : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionBuffer::matches");
		auto *acls = dynamic_cast<JPArrayClass*>( cls);
		JPClass *componentType = acls->getComponentType();
		if ( !componentType->isPrimitive())
			return match.type = JPMatch::_none;

		// Issue #598: bytearray should match byte[] not char[]
		// byteArrayConversion handles this more specifically
		if (PyByteArray_Check(match.object) &&
		    (componentType == JPContext_global->_char || componentType == JPContext_global->_byte))
			return match.type = JPMatch::_none;

		// If is isn't a buffer we can skip. PyBUF_STRIDES (not just
		// PyBUF_ND) so a non-contiguous 1D source (e.g. a numpy column
		// slice) still qualifies here instead of falling back to
		// sequenceConversion's per-element probing. setArrayRange (called
		// from convert() below) already opens its own PyBUF_FULL_RO view
		// and walks strides correctly regardless, so this only changes
		// matches()'s cost/quality for a strided source, not convert()'s.
		JPPyBuffer	buffer(match.object, PyBUF_STRIDES | PyBUF_FORMAT);
		if (!buffer.valid())
		{
			PyErr_Clear();
			return match.type = JPMatch::_none;
		}

		// This conversion only ever produces a flat primitive[]: a
		// multi-dimensional source (a 2D+ numpy array, or a memoryview over
		// one) can never be reinterpreted as one, no matter its element
		// type. Reject it explicitly and cheaply here, from the ndim this
		// buffer already reports -- convert() below re-derives the same
		// ndim==1 fact from its own Py_buffer to decide whether its bulk
		// fast path applies, but by then this conversion has already been
		// selected as the best-matching candidate for the parameter, so
		// checking there is too late to keep a >1D source from winning the
		// overload match in the first place (multiArrayBufferConversion,
		// not this conversion, is what should claim an N-D source against
		// an N-D array parameter).
		if (buffer.getView().ndim != 1)
			return match.type = JPMatch::_none;

		// If it is a buffer we only need to test the first item in the list
		JPPySequence seq = JPPySequence::use(match.object);
		jlong length = seq.size();
		if (length == -1 && PyErr_Occurred())
		{
			PyErr_Clear();
			return match.type = JPMatch::_none;
		}
		// From here the result depends on the buffer's element type/content,
		// not just Py_TYPE(object) -- e.g. a numpy array of floats vs ints.
		match.cacheable = false;
		match.type = JPMatch::_implicit;
		if (length > 0)
		{
			JPPyObject item = seq[0];
			JPMatch imatch(match.frame, item.get());
			componentType->findJavaConversion(imatch);
			if (imatch.type < match.type)
				match.type = imatch.type;
		}
		match.closure = cls;
		match.conversion = bufferConversion;
		return match.type;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		// This will be covered by Sequence
	}

	jvalue convert(JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionBuffer::convert");
		JPJavaFrame frame(*match.frame);
		jvalue res;
		auto *acls = (JPArrayClass *) match.closure;
		auto length = (jsize) PySequence_Length(match.object);
		JPClass *ccls = acls->getComponentType();

		// Fast path: hand the whole source buffer to Java in one JNI call
		// (Support.fillFlatFromBuffer does the dtype coercion and any
		// non-unit stride walk there) instead of pinning a freshly
		// allocated destination array and running a per-element
		// jconverter() loop back in C++ -- JPClass::setArrayRange's general
		// path below, kept only for what this can't classify (an exotic
		// buffer format, e.g. complex/structured dtypes) or a
		// negative-stride source (a reversed numpy view), neither of which
		// is worth the extra address-arithmetic to support here.
		auto *pcls = dynamic_cast<JPPrimitiveType*>(ccls);
		if (pcls != nullptr && length > 0 && PyObject_CheckBuffer(match.object))
		{
			JPPyBuffer buffer(match.object, PyBUF_STRIDES | PyBUF_FORMAT);
			if (buffer.valid())
			{
				Py_buffer &view = buffer.getView();
				if (view.ndim == 1)
				{
					const char *format = view.format != nullptr ? view.format : "B";
					Py_ssize_t vstep = view.strides != nullptr ? view.strides[0] : view.itemsize;
					JPBufferSource src;
					if (vstep > 0 && classifyBufferSource(format, (int) view.itemsize, src))
					{
						jobject directBuf = frame.NewDirectByteBuffer(view.buf,
								(jlong) ((length - 1) * vstep + view.itemsize));
						jarray out = (jarray) frame.fillFlatFromBuffer(pcls->getTypeCode(),
								src.kind, src.size, (jboolean) src.swapped,
								directBuf, length, (jint) vstep);
						res.l = frame.keep(out);
						return res;
					}
				}
			} else
			{
				PyErr_Clear();
			}
		}

		jarray array = ccls->newArrayOf(frame, (jsize) length);
		ccls->setArrayRange(frame, array, 0, length, 1, match.object);
		res.l = frame.keep(array);
		return res;
		JP_TRACE_OUT;
	}
}  _bufferConversion;

// Fast path for multi-dimensional primitive arrays (int[][], double[][][],
// ...) from a buffer-protocol object (a numpy ndarray, chiefly) whose ndim
// matches the array nesting depth. Without this, JPArrayClass falls through
// to sequenceConversion, which walks the outer dimension via Python-level
// indexing (materializing a fresh sub-array/view object per row) and then
// recurses into a separate JPConversionBuffer/setArrayRange call per row.
// This instead traverses the buffer directly off its own shape/strides
// (convertMultiArrayObject, shared with the explicit arrayFromBuffer()
// call), with no per-row Python object churn.
class JPConversionMultiArrayBuffer : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionMultiArrayBuffer::matches");
		auto *acls = dynamic_cast<JPArrayClass*>( cls);
		// Depth/leaf are precomputed once at class-construction time (see
		// JPArrayClass's ctor) specifically so this common-case bailout
		// (called for every array-typed argument, including plain lists
		// that will never match here) stays a couple of field reads, not a
		// dynamic_cast walk. A single dimension is already handled (faster,
		// since it also covers non-buffer sequences too) by
		// bufferConversion/setArrayRange's own buffer fast path -- this
		// conversion only has something to offer starting at int[][] and
		// deeper.
		JPPrimitiveType *pcls = acls->getMultiArrayLeaf();
		int depth = acls->getMultiArrayDepth();
		if (pcls == nullptr || depth < 2)
			return match.type = JPMatch::_none;

		if (!PyObject_CheckBuffer(match.object))
			return match.type = JPMatch::_none;

		// Same flags as bufferConversion above: PyBUF_STRIDES (not just
		// PyBUF_ND) so a non-contiguous source (a transposed numpy array,
		// a sliced sub-array, ...) still matches here instead of falling
		// through to the fully general per-row sequenceConversion.
		// convert() below already only takes its bulk DirectByteBuffer
		// fast path when PyBuffer_IsContiguous holds; the non-contiguous
		// case falls to newMultiArrayObject/convertMultiArrayObject,
		// which already walks view.strides correctly at every dimension.
		JPPyBuffer buffer(match.object, PyBUF_STRIDES | PyBUF_FORMAT);
		if (!buffer.valid())
		{
			PyErr_Clear();
			return match.type = JPMatch::_none;
		}
		Py_buffer &view = buffer.getView();
		if (view.ndim != depth)
			return match.type = JPMatch::_none;

		char code[2] = {(char) tolower(pcls->getTypeCode()), 0};
		const char *format = view.format != nullptr ? view.format : "B";
		if (getConverter(format, (int) view.itemsize, code) == nullptr)
			return match.type = JPMatch::_none;

		// Depends on the buffer's shape/dtype, not just Py_TYPE(object).
		match.cacheable = false;
		match.closure = cls;
		match.conversion = multiArrayBufferConversion;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		// This will be covered by Sequence, same as bufferConversion above.
	}

	jvalue convert(JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionMultiArrayBuffer::convert");
		JPJavaFrame frame(*match.frame);
		auto *acls = (JPArrayClass *) match.closure;
		JPPrimitiveType *pcls = acls->getMultiArrayLeaf();
		// matches() above already declined (returned _none) whenever
		// getMultiArrayLeaf() is null, so convert() -- only ever called on
		// the winning candidate -- can't actually reach here with a null
		// pcls. Checked anyway to keep this call site consistent with
		// every other getMultiArrayLeaf() caller, which do check.
		if (pcls == nullptr)  // GCOVR_EXCL_LINE
			JP_RAISE(PyExc_TypeError, "No multi-array leaf type");  // GCOVR_EXCL_LINE

		JPPyBuffer buffer(match.object, PyBUF_STRIDES | PyBUF_FORMAT);
		if (!buffer.valid())
			JP_RAISE(PyExc_TypeError, "buffer protocol required");
		Py_buffer &view = buffer.getView();

		jintArray jdims = buildDimsArray(frame, view);
		Py_ssize_t subs = 1;
		for (int i = 0; i < view.ndim - 1; ++i)
			subs *= view.shape[i];
		Py_ssize_t base = view.shape[view.ndim - 1];

		char code[2] = {(char) tolower(pcls->getTypeCode()), 0};
		const char *format = view.format != nullptr ? view.format : "B";
		jconverter converter = getConverter(format, (int) view.itemsize, code);
		if (converter == nullptr)
			JP_RAISE(PyExc_TypeError, "No type converter found");

		jvalue res;

		// Fast path: whenever the source buffer's bytes can be turned
		// into pcls's elements by some fixed, bulk-friendly operation --
		// raw reinterpret, a plain byte-swap, or half-precision decode --
		// hand its memory to Java as a single DirectByteBuffer and let
		// Java do the entire reshape (no further JNI calls, so no
		// per-leaf-array GetPrimitiveArrayCritical section). Falls back
		// to the general per-leaf critical-section path below only for
		// genuine dtype coercion (e.g. float64 -> int32), which still
		// requires visiting every element through `converter`.
		jarray fast = nullptr;
		if (tryFastMultiArrayBuffer(frame, pcls, buffer, jdims, fast))
		{
			res.l = frame.keep(fast);
			return res;
		}

		res.l = frame.keep(pcls->newMultiArrayObject(frame, buffer, converter,
				(int) subs, (int) base, (jobject) jdims));
		return res;
		JP_TRACE_OUT;
	}
} _multiArrayBufferConversion;

// Ragged-native fast path for a nested Python list of int/long/float/double
// being pushed into a multi-dimensional primitive array (int[][],
// double[][][], ...). Unlike JPConversionMultiArrayBuffer above, there's
// no buffer-protocol shape/stride/dtype to read off in one shot here:
// this is a genuine
// Python list tree, so every leaf value still has to be visited once, no
// matter what. What this conversion avoids is JPClass::setArrayRange's
// redundant re-matching of the same elements (verify pass, then copy
// pass, each re-running findJavaConversion) at every non-primitive
// nesting level -- matches() below does one real validation walk (same
// as JPConversionSequence's own), and convert() does one real encode
// walk, versus today's 3+ redundant passes compounding recursively with
// depth.
//
// Scope: every primitive leaf type. I/J/F/D are 4 or 8 bytes -- already a
// clean multiple of the int32 length marker in the wire format below.
// Z/B/C/S are 1 or 2 bytes and don't naturally land on that boundary, so
// each leaf run of those types is padded (raggedAlign4 below) back up to
// a multiple of 4 once the run ends, keeping every length marker at every
// level 4-byte aligned regardless of leaf width -- see matchRaggedNode
// (computes the padded size), encodeRaggedNode (writes it, implicitly:
// the padding bytes are never written, only skipped over, and the
// buffer's own zero-initialization -- std::vector<char> -- covers them),
// and Support.readRaggedLeaf (Java side, skips the same padding by
// position).
bool isRaggedEligible(char typeCode)
{
	switch (typeCode)
	{
		case 'I':
		case 'J':
		case 'F':
		case 'D':
		case 'Z':
		case 'B':
		case 'C':
		case 'S':
			return true;
		default:
			return false;
	}
}

static inline size_t raggedItemSize(char typeCode)
{
	switch (typeCode)
	{
		case 'Z':
		case 'B':
			return sizeof (jbyte);
		case 'C':
		case 'S':
			return sizeof (jshort);
		case 'I':
		case 'F':
			return sizeof (jint);
		default: // 'J'/'D'
			return sizeof (jlong);
	}
}

// Rounds a byte count up to the next multiple of sizeof(jint) (4) -- the
// width of the wire format's own length marker. Z/B/C/S leaf runs (1 or
// 2 bytes/element) don't necessarily end on that boundary; I/J/F/D runs
// (4 or 8 bytes/element) always already do, so this is a no-op for them.
static inline size_t raggedAlign4(size_t n)
{
	return (n + 3) & ~size_t (3);
}

// Fast, exact-type-only leaf check -- the same raw-type-check shape used
// by JPClass::sequenceCheck's own fast path (jp_class.cpp). Anything that
// doesn't pass
// (a bool, a numpy scalar, an __index__ object, a plain Python int handed
// to a float[]/double[] target, ...) fails the *whole* match for this
// conversion (see matchRaggedNode below) and falls through to
// JPConversionSequence's general path unchanged -- this is the only
// fallback trigger in this design, raggedness itself never is one.
static inline bool isRaggedLeafElement(char typeCode, PyObject *obj)
{
	switch (typeCode)
	{
		case 'I':
		case 'J':
		case 'B':
		case 'S':
			return PyLong_CheckExact(obj);
		case 'F':
		case 'D':
			return PyFloat_CheckExact(obj);
		case 'Z':
			return PyBool_Check(obj);
		case 'C':
			return PyUnicode_CheckExact(obj) && PyUnicode_GET_LENGTH(obj) == 1;
		default:
			return false; // GCOVR_EXCL_LINE
	}
}

// Which per-node element-access strategy matchRaggedNode/encodeRaggedNode
// should use, decided once per node rather than re-derived per element.
// list/tuple are the overwhelmingly common case (see nested_list()-style
// builders throughout this codebase's own benchmarks and tests) and get
// PyList_GET_ITEM/PyTuple_GET_ITEM -- index straight into the container's
// backing array, borrowed reference, no protocol dispatch -- the same
// specialization JPClass::sequenceCheckList/sequenceCheckTuple already use
// for the flat (1D) push path (jp_class.h). Anything else (a range, a
// numpy array passed as a sub-list, a user PySequence_Check-true type)
// falls back to the generic JPPySequence-wrapped seq[i], unchanged from
// before -- one node's worth of extra dispatch cost, not the whole
// subtree's.
enum class RaggedSeqKind
{
	LIST, TUPLE, GENERIC
};

static inline RaggedSeqKind raggedSeqKind(PyObject *node)
{
	if (PyList_CheckExact(node))
		return RaggedSeqKind::LIST;
	if (PyTuple_CheckExact(node))
		return RaggedSeqKind::TUPLE;
	return RaggedSeqKind::GENERIC;
}

// Depth-first pre-order walk mirroring the wire format's own encode()
// shape exactly: one int32 length marker per node, uniformly at every
// level including the outermost, then raw leaf values once
// remainingDepth reaches 1. Computes the exact serialized byte count for
// this subtree as a free byproduct of the validation walk matches() has
// to do anyway (out-param byteCount), and returns false -- whole subtree
// disqualified, no partial credit -- the instant anything doesn't fit:
// wrong Python type at a leaf, a non-sequence where a sub-list was
// expected, or a sequence whose size() itself raised.
//
// Self-contained rather than routed through componentType->
// findJavaConversion()/JPArrayClass::findJavaConversionImpl the way
// JPConversionSequence recurses: that dispatch path only re-enters this
// same conversion while m_MultiArrayDepth stays >= 2, so at the
// second-to-last level (e.g. int[] as the component of int[][]) it would
// fall through to JPConversionSequence's plain per-element check instead
// -- which has no byteCount field to hand back. Self-recursion sidesteps
// that mismatch entirely and stays exactly in step with encode() below.
static bool matchRaggedNode(PyObject *node, int remainingDepth, char typeCode, jlong &byteCount)
{
	RaggedSeqKind kind = raggedSeqKind(node);
	jlong length;
	if (kind == RaggedSeqKind::LIST)
		length = PyList_GET_SIZE(node);
	else if (kind == RaggedSeqKind::TUPLE)
		length = PyTuple_GET_SIZE(node);
	else
	{
		if (!PySequence_Check(node) || JPPyString::check(node))
			return false;
		JPPySequence seq = JPPySequence::use(node);
		length = seq.size();
		if (length == -1 && PyErr_Occurred())
		{
			PyErr_Clear();
			return false;
		}
	}

	jlong total = sizeof (jint); // this node's own length marker
	if (remainingDepth == 1)
	{
		switch (kind)
		{
			case RaggedSeqKind::LIST:
				for (jlong i = 0; i < length; i++)
					if (!isRaggedLeafElement(typeCode, PyList_GET_ITEM(node, i)))
						return false;
				break;
			case RaggedSeqKind::TUPLE:
				for (jlong i = 0; i < length; i++)
					if (!isRaggedLeafElement(typeCode, PyTuple_GET_ITEM(node, i)))
						return false;
				break;
			default:
			{
				JPPySequence seq = JPPySequence::use(node);
				for (jlong i = 0; i < length; i++)
				{
					JPPyObject item = seq[i];
					if (!isRaggedLeafElement(typeCode, item.get()))
						return false;
				}
			}
		}
		total += (jlong) raggedAlign4((size_t) (length * (jlong) raggedItemSize(typeCode)));
	} else
	{
		switch (kind)
		{
			case RaggedSeqKind::LIST:
				for (jlong i = 0; i < length; i++)
				{
					jlong childBytes = 0;
					if (!matchRaggedNode(PyList_GET_ITEM(node, i), remainingDepth - 1, typeCode, childBytes))
						return false;
					total += childBytes;
				}
				break;
			case RaggedSeqKind::TUPLE:
				for (jlong i = 0; i < length; i++)
				{
					jlong childBytes = 0;
					if (!matchRaggedNode(PyTuple_GET_ITEM(node, i), remainingDepth - 1, typeCode, childBytes))
						return false;
					total += childBytes;
				}
				break;
			default:
			{
				JPPySequence seq = JPPySequence::use(node);
				for (jlong i = 0; i < length; i++)
				{
					JPPyObject item = seq[i];
					jlong childBytes = 0;
					if (!matchRaggedNode(item.get(), remainingDepth - 1, typeCode, childBytes))
						return false;
					total += childBytes;
				}
			}
		}
	}
	byteCount = total;
	return true;
}

// convert()-side mirror of matchRaggedNode -- run once, only for the
// winning candidate, writing exactly what matchRaggedNode already proved
// would fit. No re-validation of element types here: that's already done,
// and re-checking would just be the third redundant pass this whole
// design exists to cut out. buffer must already be sized to exactly
// matches()'s byteCount; offset is threaded through by reference so every
// recursive call -- including successive siblings at the same level --
// keeps writing forward from where the last one left off.
// One leaf value's worth of encodeRaggedNode's old inline switch, factored
// out so it can be shared by the list/tuple/generic specialized loops below
// without tripling this switch.
static inline void encodeRaggedLeaf(char typeCode, PyObject *item, char *buffer, size_t &offset)
{
	switch (typeCode)
	{
		case 'I':
		{
			long v = PyLong_AsLong(item);
			if (v == -1)
				JP_PY_CHECK();  // GCOVR_EXCL_LINE
			*(jint*) (buffer + offset) = (jint) JPIntType::assertRange(v);
			offset += sizeof (jint);
			break;
		}
		case 'J':
		{
			jlong v = PyLong_AsLongLong(item);
			if (v == -1)
				JP_PY_CHECK();  // GCOVR_EXCL_LINE
			*(jlong*) (buffer + offset) = v;
			offset += sizeof (jlong);
			break;
		}
		case 'F':
		{
			double v = PyFloat_AsDouble(item);
			if (v == -1.)
				JP_PY_CHECK();  // GCOVR_EXCL_LINE
			*(jfloat*) (buffer + offset) = (jfloat) v;
			offset += sizeof (jfloat);
			break;
		}
		case 'D':
		{
			double v = PyFloat_AsDouble(item);
			if (v == -1.)
				JP_PY_CHECK();  // GCOVR_EXCL_LINE
			*(jdouble*) (buffer + offset) = (jdouble) v;
			offset += sizeof (jdouble);
			break;
		}
		case 'Z':
		{
			// isRaggedLeafElement already required PyBool_Check, so this
			// is exactly JPBooleanType::setArrayRange's own PyBool_Check
			// branch (jp_booleantype.cpp) -- no PyObject_IsTrue fallback
			// needed here, unlike that function's non-bool branch.
			*(jboolean*) (buffer + offset) = (jboolean) (item == Py_True);
			offset += sizeof (jboolean);
			break;
		}
		case 'B':
		{
			long v = PyLong_AsLong(item);
			if (v == -1)
				JP_PY_CHECK();  // GCOVR_EXCL_LINE
			*(jbyte*) (buffer + offset) = (jbyte) JPByteType::assertRange(v);
			offset += sizeof (jbyte);
			break;
		}
		case 'S':
		{
			long v = PyLong_AsLong(item);
			if (v == -1)
				JP_PY_CHECK();  // GCOVR_EXCL_LINE
			*(jshort*) (buffer + offset) = (jshort) JPShortType::assertRange(v);
			offset += sizeof (jshort);
			break;
		}
		default: // 'C'
		{
			jchar v = JPPyString::asCharUTF16(item);
			JP_PY_CHECK();  // GCOVR_EXCL_LINE
			*(jchar*) (buffer + offset) = v;
			offset += sizeof (jchar);
			break;
		}
	}
}

static void encodeRaggedNode(PyObject *node, int remainingDepth, char typeCode, char *buffer, size_t &offset)
{
	RaggedSeqKind kind = raggedSeqKind(node);
	jlong length;
	if (kind == RaggedSeqKind::LIST)
		length = PyList_GET_SIZE(node);
	else if (kind == RaggedSeqKind::TUPLE)
		length = PyTuple_GET_SIZE(node);
	else
		length = JPPySequence::use(node).size();
	*(jint*) (buffer + offset) = (jint) length;
	offset += sizeof (jint);

	if (remainingDepth == 1)
	{
		switch (kind)
		{
			case RaggedSeqKind::LIST:
				for (jlong i = 0; i < length; i++)
					encodeRaggedLeaf(typeCode, PyList_GET_ITEM(node, i), buffer, offset);
				break;
			case RaggedSeqKind::TUPLE:
				for (jlong i = 0; i < length; i++)
					encodeRaggedLeaf(typeCode, PyTuple_GET_ITEM(node, i), buffer, offset);
				break;
			default:
			{
				JPPySequence seq = JPPySequence::use(node);
				for (jlong i = 0; i < length; i++)
				{
					JPPyObject item = seq[i];
					encodeRaggedLeaf(typeCode, item.get(), buffer, offset);
				}
			}
		}
		// Restore 4-byte alignment for the next length marker -- see
		// raggedAlign4/matchRaggedNode above, which already reserved
		// exactly this many bytes. No-op for I/J/F/D (already a multiple
		// of 4); the padding bytes themselves are left as whatever
		// convert()'s std::vector<char> zero-initialized them to, since
		// Support.readRaggedLeaf only skips over them by position, never
		// reads their value.
		offset = raggedAlign4(offset);
	} else
	{
		switch (kind)
		{
			case RaggedSeqKind::LIST:
				for (jlong i = 0; i < length; i++)
					encodeRaggedNode(PyList_GET_ITEM(node, i), remainingDepth - 1, typeCode, buffer, offset);
				break;
			case RaggedSeqKind::TUPLE:
				for (jlong i = 0; i < length; i++)
					encodeRaggedNode(PyTuple_GET_ITEM(node, i), remainingDepth - 1, typeCode, buffer, offset);
				break;
			default:
			{
				JPPySequence seq = JPPySequence::use(node);
				for (jlong i = 0; i < length; i++)
				{
					JPPyObject item = seq[i];
					encodeRaggedNode(item.get(), remainingDepth - 1, typeCode, buffer, offset);
				}
			}
		}
	}
}

// closure encoding for this conversion only -- a documented, deliberate
// single exception to every other JPConversion in this file, which
// stores a real registry-owned JPClass*/JPFunctional* there (see the
// comment on JPMatch::closure in jp_match.h, which already anticipates
// this: "or a value that fits directly in the pointer, e.g. an
// integer"). convert() never needs the array class pointer itself here --
// only two cheap facts derived from it (this array's nesting depth and
// leaf type code, both already read once by matches() below) plus the
// byte count matches() computes as a free byproduct of its walk. All
// three fit in one pointer-sized value, so there's no reason to spend the
// slot on a full JPClass* only to immediately reduce it back down to two
// small fields in convert() -- and no reason for JPMatch to carry a
// second field (as an earlier version of this conversion did) just for
// the one value that didn't fit alongside it. That second field cost
// every JPMatch everywhere 8 bytes, paid on every conversion of every
// argument of every call, to serve only this one path.
//
// Layout (high to low bits): 8 bits dims, 8 bits typeCode, 48 bits
// byteCount. 48 bits is 256 TiB -- no real allocation will ever reach
// that; if one somehow did, the std::vector allocation in convert() below
// would already have failed well before this became a real limit.
static inline void *packRaggedClosure(int dims, char typeCode, jlong byteCount)
{
	uint64_t v = ((uint64_t) (unsigned char) dims << 56)
			| ((uint64_t) (unsigned char) typeCode << 48)
			| ((uint64_t) byteCount & 0xFFFFFFFFFFFFULL);
	return reinterpret_cast<void*> ((uintptr_t) v);
}

static inline void unpackRaggedClosure(void *closure, int &dims, char &typeCode, jlong &byteCount)
{
	auto v = (uint64_t) reinterpret_cast<uintptr_t> (closure);
	dims = (int) ((v >> 56) & 0xFF);
	typeCode = (char) ((v >> 48) & 0xFF);
	byteCount = (jlong) (v & 0xFFFFFFFFFFFFULL);
}

class JPConversionRaggedSequence : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionRaggedSequence::matches");
		auto *acls = dynamic_cast<JPArrayClass*>( cls);
		int dims = acls->getMultiArrayDepth();
		char typeCode = acls->getMultiArrayLeaf()->getTypeCode();

		jlong total = 0;
		if (!matchRaggedNode(match.object, dims, typeCode, total))
			return match.type = JPMatch::_none;

		// Depends on the list's actual contents (every leaf value's exact
		// Python type, all the way down), not just Py_TYPE(object) -- same
		// reasoning as JPConversionSequence/JPConversionBuffer above.
		match.cacheable = false;
		match.closure = packRaggedClosure(dims, typeCode, total);
		match.conversion = raggedSequenceConversion;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		// Covered by Sequence, same as multiArrayBufferConversion above --
		// this doesn't accept a different *shape* of Python input, just a
		// faster internal path for what JPConversionSequence already
		// documents.
	}

	jvalue convert(JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionRaggedSequence::convert");
		JPJavaFrame frame(*match.frame);
		int dims;
		char typeCode;
		jlong byteCount;
		unpackRaggedClosure(match.closure, dims, typeCode, byteCount);

		// Exact-size, RAII-local allocation -- byteCount was computed by
		// matches() as a free byproduct of the walk it already had to do,
		// so there's no growth/guessing here, and buffer (along with
		// everything reachable only through it) is destroyed the instant
		// this function returns. The only thing that escapes is the
		// resulting jobject local reference below.
		std::vector<char> buffer(byteCount);
		size_t offset = 0;
		encodeRaggedNode(match.object, dims, typeCode, buffer.data(), offset);

		jobject directBuf = frame.NewDirectByteBuffer(buffer.data(), (jlong) buffer.size());
		jvalue res;
		res.l = frame.keep(frame.fillRaggedFromBuffer(typeCode, (jint) dims, directBuf));
		return res;
		JP_TRACE_OUT;
	}
} _raggedSequenceConversion;

class JPConversionSequence : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionSequence::matches");
		if ( !PySequence_Check(match.object) || JPPyString::check(match.object))
			return match.type = JPMatch::_none;
		auto *acls = dynamic_cast<JPArrayClass*>( cls);
		JPClass *componentType = acls->getComponentType();
		JPPySequence seq = JPPySequence::use(match.object);
		jlong length = seq.size();
		if (length==-1 && PyErr_Occurred())
		{
			PyErr_Clear();
			return match.type = JPMatch::_none;
		}
		// From here the result depends on the sequence's element types, not
		// just Py_TYPE(object) -- e.g. a list of ints vs a list of strings.
		match.cacheable = false;

		// See JPClass::sequenceCheck: generic and safe for every component
		// type, since it only trusts its own per-type cache slot when the
		// ordinary findJavaConversion reports the result cacheable -- the
		// same flag already used (and set correctly by every
		// JPConversion::matches()) for findJavaConversion's own per-class
		// cache. (A plain list/tuple never reaches here at all --
		// JPConversionList/JPConversionTuple below peel those off earlier
		// in each array class's chain, with their own sequenceCheckList/
		// Tuple that avoid seq[i]'s PySequence_GetItem entirely.)
		componentType->sequenceCheck(match, seq, length);
		match.closure = cls;
		match.conversion = sequenceConversion;
		return match.type;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyObject *typing = PyImport_AddModule("jpype.protocol");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "Sequence"));
		PyList_Append(info.implicit, proto.get());
		auto* acls = dynamic_cast<JPArrayClass*>( cls);
		if (acls->getComponentType() == JPContext_global->_char)
			return;
		PyList_Append(info.none, (PyObject*) & PyUnicode_Type);
	}

	jvalue convert(JPMatch &match) override
	{
		JPJavaFrame frame(*match.frame);
		jvalue res;
		auto *acls = (JPArrayClass *) match.closure;
		auto length = (jsize) PySequence_Length(match.object);
		JPClass *ccls = acls->getComponentType();
		jarray array = ccls->newArrayOf(frame, (jsize) length);
		ccls->setArrayRange(frame, array, 0, length, 1, match.object);
		res.l = frame.keep(array);
		return res;
	}
} _sequenceConversion;

// list -> 1D array, specialized so the quality-check loop never branches
// on container type per element (see JPClass::sequenceCheckList).
// Tried ahead of sequenceConversion in each array class's chain; falls
// through (returns _none) for anything that isn't PyList_CheckExact, so
// sequenceConversion remains the correct general fallback for everything
// else (tuples are peeled off by JPConversionTuple below, a custom
// Sequence subclass or a range falls all the way to sequenceConversion).
// Owns its own convert() -- PyList_GET_SIZE instead of PySequence_Length,
// otherwise identical to sequenceConversion::convert(); setArrayRange
// itself still resolves PyList_CheckExact/PyTuple_CheckExact internally
// since it's also reached directly from JPArray::setArrayRange and array
// construction, which never go through this class at all.
class JPConversionList : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionList::matches");
		if (!PyList_CheckExact(match.object))
			return match.type = JPMatch::_none;
		auto *acls = dynamic_cast<JPArrayClass*>( cls);
		JPClass *componentType = acls->getComponentType();
		jlong length = PyList_GET_SIZE(match.object);
		match.cacheable = false;
		componentType->sequenceCheckList(match, match.object, length);
		match.closure = cls;
		match.conversion = this;
		return match.type;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		// No entry of its own -- sequenceConversion's getInfo already
		// advertises "Sequence" (list included) for documentation
		// purposes; this class only exists to speed up matching, not to
		// broaden what's accepted.
	}

	jvalue convert(JPMatch &match) override
	{
		JPJavaFrame frame(*match.frame);
		jvalue res;
		auto *acls = (JPArrayClass *) match.closure;
		auto length = (jsize) PyList_GET_SIZE(match.object);
		JPClass *ccls = acls->getComponentType();
		jarray array = ccls->newArrayOf(frame, length);
		ccls->setArrayRange(frame, array, 0, length, 1, match.object);
		res.l = frame.keep(array);
		return res;
	}
} _listConversion;

// tuple -> 1D array, the PyTuple_CheckExact counterpart to
// JPConversionList above -- see there for the full rationale.
class JPConversionTuple : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionTuple::matches");
		if (!PyTuple_CheckExact(match.object))
			return match.type = JPMatch::_none;
		auto *acls = dynamic_cast<JPArrayClass*>( cls);
		JPClass *componentType = acls->getComponentType();
		jlong length = PyTuple_GET_SIZE(match.object);
		match.cacheable = false;
		componentType->sequenceCheckTuple(match, match.object, length);
		match.closure = cls;
		match.conversion = this;
		return match.type;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		// See JPConversionList::getInfo.
	}

	jvalue convert(JPMatch &match) override
	{
		// See JPConversionList::convert.
		JPJavaFrame frame(*match.frame);
		jvalue res;
		auto *acls = (JPArrayClass *) match.closure;
		auto length = (jsize) PyTuple_GET_SIZE(match.object);
		JPClass *ccls = acls->getComponentType();
		jarray array = ccls->newArrayOf(frame, length);
		ccls->setArrayRange(frame, array, 0, length, 1, match.object);
		res.l = frame.keep(array);
		return res;
	}
} _tupleConversion;

class JPConversionNull : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionNull::matches");
		if (match.object != Py_None)
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue v;
		v.l = nullptr;
		return v;
	}
} _nullConversion;

class JPConversionClass : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionClass::matches");
		if (match.frame == nullptr)
			return match.type = JPMatch::_none;
		// PyJPClass_getJPClass gates on Py_TYPE first (PyJPClass_Check) and
		// returns nullptr immediately for anything that isn't a _JClass
		// instance -- that "not a Class object at all" case (the overwhelming
		// majority of callers, e.g. any plain object matched against
		// java.lang.Object) is genuinely type-only and safe to cache.
		JPClass* cls2 = PyJPClass_getJPClass(match.object);
		if (cls2 == nullptr)
			return match.type = JPMatch::_none;
		// Every _JClass type instance shares the same Py_TYPE
		// (PyJPClass_Type) but wraps a different java.lang.Class -- from
		// here the result depends on match.object's identity, not just its
		// type.
		match.cacheable = false;
		match.conversion = this;
		match.closure = cls2;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		PyList_Append(info.implicit, (PyObject*) PyJPClass_Type);
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		auto* cls2 = (JPClass*) match.closure;
		res.l = match.frame->NewLocalRef(cls2->getJavaClass());
		return res;
	}
} _classConversion;

class JPConversionObject : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionObject::matches");
		JPClass *oc = match.getJPClass();
		if (oc == nullptr || match.frame == nullptr)
			return match.type = JPMatch::_none;
		match.conversion = this;
		if (oc == cls)
		{
			// hey, this is me! :)
			return match.type = JPMatch::_exact;
		}
		bool assignable = match.frame->IsAssignableFrom(oc->getJavaClass(), cls->getJavaClass()) != 0;
		JP_TRACE("assignable", assignable, oc->getCanonicalName(), cls->getCanonicalName());
		match.type = (assignable ? JPMatch::_derived : JPMatch::_none);

		// User has request a Java class to class conversion.  We must pass through check it.
		if (!assignable)
		{
			auto *pyhints = (PyJPClassHints*) cls->getHints();
			JPClassHints *hints = pyhints->m_Hints;
			if (hints->m_ConvertJava)
				return match.type;
		}

		// This is the one except to the conversion rule patterns.
		// If it is a Java value then we must prevent it from proceeding
		// through the conversion rules even if it was not a match.
		// Thus the return result and the match type differ here.
		return JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		PyList_Append(info.exact, PyJPClass_create(frame, cls).get());
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		res.l = match.frame->NewLocalRef(match.getJValue().l);
		return res;
	}
} _objectConversion;

JPMatch::Type JPConversionJavaValue::matches(JPClass *cls, JPMatch &match)
{
	JP_TRACE_IN("JPConversionJavaValue::matches");
	JPClass *oc = match.getJPClass();
	if (oc == nullptr || oc != cls)
		return match.type = JPMatch::_none;
	match.conversion = this;
	return match.type = JPMatch::_exact;
	JP_TRACE_OUT;
}

void JPConversionJavaValue::getInfo(JPClass *cls, JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	PyList_Append(info.exact, PyJPClass_create(frame, cls).get());
}

jvalue JPConversionJavaValue::convert(JPMatch &match)
{
	return match.getJValue();
}

JPConversionJavaValue _javaValueConversion;

class JPConversionString : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionString::matches");
		if (match.frame == nullptr || !JPPyString::check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		if (cls == JPContext_global->_java_lang_String)
			return match.type = JPMatch::_exact;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.implicit, (PyObject*) & PyUnicode_Type);
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		string str = JPPyString::asStringUTF8(match.object);
		res.l = match.frame->fromStringUTF8(str);
		return res;
	}
} _stringConversion;

class JPConversionBox : public JPConversion
{
public:

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		JPPyObjectVector args(match.object, nullptr);
		auto *cls = (JPClass*) match.closure;
		JPValue pobj = cls->newInstance(*match.frame, args);
		res.l = pobj.getJavaObject();
		return res;
	}
} ;

class JPConversionBoxBoolean : public JPConversionBox
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match)  override
	{
		JP_TRACE_IN("JPConversionBoxBoolean::matches");
		// This is reached directly via JPObjectType/JPNumberType's chain
		// (a bare Python bool matched against Object/Number), where the
		// box class is always java.lang.Boolean regardless of `cls`. See
		// JPConversionBoxGeneric below for JPBoxedType's own, separate
		// "any boxed primitive" stand-in -- keeping the two roles as
		// distinct objects means neither one's closure handling has to
		// account for the other (a single object previously played both
		// roles, and fixing one's closure handling silently broke the
		// other -- see the JObject(5, Integer) regression this caused).
		if (!PyBool_Check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		match.closure = JPContext_global->_java_lang_Boolean;
		return match.type = JPMatch::_implicit;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.implicit, (PyObject*) & PyBool_Type);
	}

} _boxBooleanConversion;

/**
 * Generic "any boxed primitive" stand-in used only by
 * JPBoxedType::findJavaConversionImpl, once it has already resolved a
 * match via the primitive type and knows which box class (`this`) it
 * wants -- convert() (inherited from JPConversionBox, unmodified) just
 * reads match.closure as the caller already set it to.
 */
class JPConversionBoxGeneric : public JPConversionBox
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		// Never reached via the normal matches() chain -- JPBoxedType
		// assigns this conversion directly, once it already knows the
		// match succeeded some other way.
		return match.type = JPMatch::_none; // GCOVR_EXCL_LINE
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
	}
} _boxGenericConversion;

class JPConversionBoxLong : public JPConversionBox
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match)  override
	{
		JP_TRACE_IN("JPConversionBoxLong::matches");
		if (match.frame == nullptr)
			return match.type = JPMatch::_none;
		if (PyLong_CheckExact(match.object) || PyIndex_Check(match.object))
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyObject *typing = PyImport_AddModule("jpype.protocol");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "SupportsIndex"));
		PyList_Append(info.implicit, proto.get());
	}

	jvalue convert(JPMatch &match) override
	{
		PyTypeObject* type = Py_TYPE(match.object);
		match.closure = JPContext_global->_java_lang_Long;

		// Hot path dispatch using the numpy tree
		PyTypeObject* nptype = PyJP_GetNumPyBaseType(type);
		if (nptype != nullptr)
		{
			if (nptype == (PyTypeObject*) _numpy_int32_type)
				match.closure = JPContext_global->_java_lang_Integer;
			else if (nptype == (PyTypeObject*) _numpy_int16_type)
				match.closure = JPContext_global->_java_lang_Short;
			else if (nptype == (PyTypeObject*) _numpy_int8_type)
				match.closure = JPContext_global->_java_lang_Byte;
		}

		return JPConversionBox::convert(match);
	}
} _boxLongConversion;

class JPConversionBoxDouble : public JPConversionBox
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionBoxDouble::matches");
		if (match.frame == nullptr)
			return match.type = JPMatch::_none;
		if (PyNumber_Check(match.object))
		{
			match.conversion = this;
			return match.type = JPMatch::_implicit;
		}
		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyObject *typing = PyImport_AddModule("jpype.protocol");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "SupportsFloat"));
		PyList_Append(info.implicit, proto.get());
	}

	jvalue convert(JPMatch &match) override
	{
		PyTypeObject* type = Py_TYPE(match.object);
		const char *name = type->tp_name;
		match.closure = JPContext_global->_java_lang_Double;
		if (strncmp(name, "numpy", 5) == 0)
		{
			// We only handle specific sized types, all others go to double.
			if (strcmp(&name[5], ".float32") == 0)
				match.closure = JPContext_global->_java_lang_Float;
		}
		return JPConversionBox::convert(match);
	}
} _boxDoubleConversion;

class JPConversionJavaObjectAny : public JPConversionBox
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionJavaObjectAny::matches");
		JPClass *oc = match.getJPClass();
		if (oc == nullptr || match.frame == nullptr)
			return match.type = JPMatch::_none;
		match.conversion = this;
		if (oc->isPrimitive())
			match.type = JPMatch::_implicit;
		else if (oc == cls)
			match.type = JPMatch::_exact;
		else
			match.type = JPMatch::_derived;
		return match.type;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		PyList_Append(info.implicit, PyJPClass_create(frame, JPContext_global->_java_lang_Object).get());
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		JPJavaFrame *frame = match.frame;
		JPClass *oc = match.getJPClass();
		if (!oc->isPrimitive())
		{
			res.l = frame->NewLocalRef(match.getJValue().l);
			return res;
		} else
		{
			// Okay we need to box it.
			auto* type = dynamic_cast<JPPrimitiveType*> (oc);
			match.closure = type->getBoxedClass(*frame);
			res = JPConversionBox::convert(match);
			return res;
		}
	}
} _javaObjectAnyConversion;

class JPConversionJavaNumberAny : public JPConversionJavaObjectAny
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionJavaNumberAny::matches");
		JPContext *context = JPContext_global;
		JPClass *oc = match.getJPClass();
		// This converter only works for number types, thus boolean and char
		// are excluded.
		if (oc == nullptr || match.frame == nullptr
				|| oc == context->_boolean
				|| oc == context->_char)
			return match.type = JPMatch::_none;
		match.conversion = this;
		// If it is the exact type, then it is exact
		if (oc == cls)
			return match.type = JPMatch::_exact;
		// If it is any primitive except char and boolean then implicit
		if (oc->isPrimitive())
			return match.type = JPMatch::_implicit;
		// Otherwise, check if it is assignable according to Java
		bool assignable = match.frame->IsAssignableFrom(oc->getJavaClass(), cls->getJavaClass()) != 0;
		return match.type = (assignable ? JPMatch::_implicit : JPMatch::_none);
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.implicit, (PyObject*) PyJPNumberLong_Type);
		PyList_Append(info.implicit, (PyObject*) PyJPNumberFloat_Type);
	}

} _javaNumberAnyConversion;

class JPConversionUnbox : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JPContext *context = JPContext_global;
		if (context == nullptr)
			return match.type = JPMatch::_none;
		JPClass *oc = match.getJPClass();
		auto *pcls = dynamic_cast<JPPrimitiveType*>( cls);
		if (oc != pcls->getBoxedClass(*match.frame))
			return match.type = JPMatch::_none;
		match.conversion = this;
		match.closure = cls;
		return match.type = JPMatch::_implicit;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		auto *pcls = dynamic_cast<JPPrimitiveType*>( cls);
		PyList_Append(info.implicit,
				PyJPClass_create(frame, pcls->getBoxedClass(frame)).get());
	}

	jvalue convert(JPMatch &match) override
	{
		auto *cls = (JPClass*) match.closure;
		return cls->getValueFromObject(*match.frame, JPValue(match.getJPClass(), match.getJValue()));
	}
} _unboxConversion;

class JPConversionProxy : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		JP_TRACE_IN("JPConversionProxy::matches");
		JPProxy* proxy = PyJPProxy_getJPProxy(match.object);
		if (proxy == nullptr || match.frame == nullptr)
			// Whether a Python type carries a proxy at all is fixed at
			// class-decoration time (@JImplements), so this branch is
			// genuinely type-only and safe to cache -- unlike the interface
			// list below, which we don't have the same guarantee about.
			return match.type = JPMatch::_none;

		// Interfaces come from a per-instance JPProxy*; not verified to be
		// invariant across all instances of a given Python type, so treated
		// conservatively as never cacheable from here on.
		match.cacheable = false;

		// Check if any of the interfaces matches ...
		vector<JPClass*> itf = proxy->getInterfaces();
		for (auto & i : itf)
		{
			if (match.frame->IsAssignableFrom(i->getJavaClass(), cls->getJavaClass()))
			{
				JP_TRACE("implicit proxy");
				match.conversion = this;
				return match.type = JPMatch::_implicit;
			}
		}
		return match.type = JPMatch::_none;
		JP_TRACE_OUT;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
	}

	jvalue convert(JPMatch &match) override
	{
		return PyJPProxy_getJPProxy(match.object)->getProxy();
	}
} _proxyConversion;

JPConversion *hintsConversion = &_hintsConversion;
JPConversion *charArrayConversion = &_charArrayConversion;
JPConversion *byteArrayConversion = &_byteArrayConversion;
JPConversion *bufferConversion = &_bufferConversion;
JPConversion *multiArrayBufferConversion = &_multiArrayBufferConversion;
JPConversion *raggedSequenceConversion = &_raggedSequenceConversion;
JPConversion *sequenceConversion = &_sequenceConversion;
JPConversion *listConversion = &_listConversion;
JPConversion *tupleConversion = &_tupleConversion;
JPConversion *nullConversion = &_nullConversion;
JPConversion *classConversion = &_classConversion;
JPConversion *objectConversion = &_objectConversion;
JPConversion *javaObjectAnyConversion = &_javaObjectAnyConversion;
JPConversion *javaNumberAnyConversion = &_javaNumberAnyConversion;
JPConversion *javaValueConversion = &_javaValueConversion;
JPConversion *stringConversion = &_stringConversion;
JPConversion *boxBooleanConversion = &_boxBooleanConversion;
JPConversion *boxGenericConversion = &_boxGenericConversion;
JPConversion *boxLongConversion = &_boxLongConversion;
JPConversion *boxDoubleConversion = &_boxDoubleConversion;
JPConversion *unboxConversion = &_unboxConversion;
JPConversion *proxyConversion = &_proxyConversion;
