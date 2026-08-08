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
		match.closure = cls;
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

		// If is isn't a buffer we can skip
		JPPyBuffer	buffer(match.object, PyBUF_ND | PyBUF_FORMAT);
		if (!buffer.valid())
		{
			PyErr_Clear();
			return match.type = JPMatch::_none;
		}

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

		// Same flags as bufferConversion above: PyBUF_ND requires a
		// C-contiguous view, so anything that can't provide one (e.g. a
		// transposed numpy array) simply fails to match here and falls
		// through to the general (always-correct) sequenceConversion.
		JPPyBuffer buffer(match.object, PyBUF_ND | PyBUF_FORMAT);
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

		JPPyBuffer buffer(match.object, PyBUF_ND | PyBUF_FORMAT);
		if (!buffer.valid())
			JP_RAISE(PyExc_TypeError, "buffer protocol required");
		Py_buffer &view = buffer.getView();

		JPContext *context = frame.getContext();
		auto jdims = (jintArray) context->_int->newArrayOf(frame, view.ndim);
		{
			JPPrimitiveArrayAccessor<jintArray, jint*> accessor(frame, jdims,
					&JPJavaFrame::GetIntArrayElements, &JPJavaFrame::ReleaseIntArrayElements);
			jint *a = accessor.get();
			for (int i = 0; i < view.ndim; ++i)
				a[i] = (jint) view.shape[i];
			accessor.commit();
		}
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
		res.l = frame.keep(pcls->newMultiArrayObject(frame, buffer, converter,
				(int) subs, (int) base, (jobject) jdims));
		return res;
		JP_TRACE_OUT;
	}
} _multiArrayBufferConversion;

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
		match.type = JPMatch::_implicit;
		for (jlong i = 0; i < length && match.type > JPMatch::_none; i++)
		{
			// This is a special case.  Sequences produce new references
			// so we must hold the reference in a container while
			// the match is caching it.
			JPPyObject item = seq[i];

			// Fast path: a raw type check standing in for a known quality,
			// skipping JPMatch construction and the general
			// findJavaConversion dispatch entirely. Falls through to the
			// general path (unchanged) the moment an element doesn't
			// qualify -- so a homogeneous list (the common case) never
			// touches the slow path at all, and a mixed list only pays
			// full price from the first non-conforming element onward, not
			// for the whole list.
			JPMatch::Type fastQuality;
			if (componentType->fastElementCheck(item.get(), fastQuality))
			{
				if (fastQuality < match.type)
					match.type = fastQuality;
				continue;
			}

			JPMatch imatch(match.frame, item.get());
			componentType->findJavaConversion(imatch);
			if (imatch.type < match.type)
				match.type = imatch.type;
		}
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
JPConversion *sequenceConversion = &_sequenceConversion;
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
