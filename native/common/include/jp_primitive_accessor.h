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
#ifndef JP_PRIMITIVE_ACCESSOR_H
#define JP_PRIMITIVE_ACCESSOR_H
#include <cstring>
#include <Python.h>
#include "jp_exception.h"
#include "jp_javaframe.h"
#include "jp_match.h"

template <typename array_t, typename ptr_t>
class JPPrimitiveArrayAccessor
{
	using releaseFnc = void (JPJavaFrame::*)(array_t, ptr_t, jint);
	using accessFnc = ptr_t (JPJavaFrame::*)(array_t, jboolean *);

	JPJavaFrame& _frame;
	array_t _array;
	ptr_t _elem;
	releaseFnc _release;
	jboolean _iscopy;

public:

	JPPrimitiveArrayAccessor(JPJavaFrame& frame, jarray array, accessFnc access, releaseFnc release)
	: _frame(frame), _array((array_t) array), _release(release)
	{
		_elem = ((&_frame)->*access)(_array, &_iscopy);
	}

	~JPPrimitiveArrayAccessor()
	{
		// This is fallback if commit or abort is not called.
		// It should only occur in cases where a throw has
		// already been issued.
		try
		{
			if (_array)
				((&_frame)->*_release)(_array, _elem, JNI_ABORT);
		}		catch (...) // GCOVR_EXCL_LINE
		{
			// We can't throw here because it would abort.
			// But this is called on a non-op release, so
			// we will just eat it
		}
	}

	jsize size()
	{
		return _frame.GetArrayLength(_array);
	}

	ptr_t get()
	{
		return _elem;
	}

	void commit()
	{
		// Prevent the dtor from calling a second time
		array_t a = _array;
		_array = 0;
		((&_frame)->*_release)(a, _elem, 0);
	}

	void abort()
	{
		// Prevent the dtor from calling a second time
		array_t a = _array;
		_array = 0;
		((&_frame)->*_release)(a, _elem, JNI_ABORT);
	}

} ;

// Traverses a Python buffer and packs it into a (possibly multi-dimensional)
// Java primitive array, returning the raw local ref. Split out from
// convertMultiArray below so callers that want the jvalue directly (the
// general argument-conversion path, which mustn't round-trip through a
// Python wrapper object just to unwrap it again) can use it without also
// paying for convertToPythonObject. converter must already have been
// resolved (see convertMultiArray) -- this half assumes it is valid.
template <class type_t> jobject convertMultiArrayObject(
		JPJavaFrame &frame,
		JPPrimitiveType* cls,
		void (*pack)(type_t*, jvalue),
		jconverter converter,
		JPPyBuffer &buffer,
		int subs, int base, jobject dims)
{
	JPContext *context = frame.getContext();
	Py_buffer& view = buffer.getView();

	// Reserve space for array.
	auto contents = (jobjectArray) context->_java_lang_Object->newArrayOf(frame, subs);
	std::vector<Py_ssize_t> indices(view.ndim);
	int u = view.ndim - 1;
	int k = 0;
	jarray a0 = cls->newArrayOf(frame, base);
	frame.SetObjectArrayElement(contents, k++, a0);
	jboolean isCopy;
	void *mem = frame.getEnv()->GetPrimitiveArrayCritical(a0, &isCopy);
	JP_TRACE_JAVA("GetPrimitiveArrayCritical", mem);
	auto *dest = (type_t*) mem;

	Py_ssize_t step;
	if (view.strides == nullptr)
		step = view.itemsize;
	else
		step = view.strides[u];

	// Align with the first element in the array
	char *src = buffer.getBufferPtr(indices);

	// Traverse the array
	while (true)
	{
		if (indices[u] == view.shape[u])
		{
			int j;
			for (j = 0; j < u; ++j)
			{
				indices[u - j - 1]++;
				if (indices[u - j - 1] < view.shape[u - j - 1])
					break;
				indices[u - j - 1] = 0;
			}
			// Commit the current section
			indices[u] = 0;
			JP_TRACE_JAVA("ReleasePrimitiveArrayCritical", mem);
			frame.getEnv()->ReleasePrimitiveArrayCritical(a0, mem, JNI_COMMIT);
			frame.DeleteLocalRef(a0);

			// If we hit the shape of the uppermost we are done
			if (j == u)
				break;

			a0 = cls->newArrayOf(frame, base);
			frame.SetObjectArrayElement(contents, k++, a0);
			mem = frame.getEnv()->GetPrimitiveArrayCritical(a0, &isCopy);
			JP_TRACE_JAVA("GetPrimitiveArrayCritical", mem);
			dest = (type_t*) mem;
			src = buffer.getBufferPtr(indices);
		}
		pack(dest, converter(src));
		src += step;
		dest++;
		indices[u]++;
	}

	// Assemble it into a multidimensional array
	return frame.assemble(dims, contents);
}

template <class type_t> PyObject *convertMultiArray(
		JPJavaFrame &frame,
		JPPrimitiveType* cls,
		void (*pack)(type_t*, jvalue),
		const char* code,
		JPPyBuffer &buffer,
		int subs, int base, jobject dims)
{
	JPContext *context = frame.getContext();
	Py_buffer& view = buffer.getView();
	jconverter converter = getConverter(view.format, (int) view.itemsize, code);
	if (converter == nullptr)
	{
		PyErr_Format(PyExc_TypeError, "No type converter found");
		return nullptr;
	}

	jobject out = convertMultiArrayObject<type_t>(
			frame, cls, pack, converter, buffer, subs, base, dims);

	// Convert it to Python
	JPClass *type = context->_java_lang_Object;
	if (out != nullptr)
		type = frame.findClassForObject(out);
	jvalue v;
	v.l = out;
	return type->convertToPythonObject(frame, v, false).keep();
}

/**
 * Bulk-copy a (possibly strided/sliced) 1-D primitive Java array into an
 * arbitrary-shape/strided caller-owned Python buffer (JArray.pullTo's
 * general path).
 *
 * Used whenever the fast contiguous path (a single Get<Type>ArrayRegion
 * call straight into the destination, see JPArray::pullTo) does not
 * apply, i.e. the source is a stepped slice and/or the destination is
 * non-contiguous or N-dimensional. Mirrors convertMultiArrayObject's
 * Py_buffer stride-walk above, but reversed (the Java array is pinned as
 * the *source*, the destination's shape/strides are walked instead of the
 * source's) and without value conversion: this is a same-dtype bulk copy,
 * not a type-converting one, so it is a plain itemsize-wide memcpy per
 * element rather than a pack/converter round trip.
 */
inline void copyArrayToBuffer(JPJavaFrame &frame, jarray source,
		jsize start, jsize step, jsize len, Py_ssize_t itemsize,
		JPPyBuffer &destBuffer)
{
	Py_buffer& view = destBuffer.getView();
	jboolean isCopy;
	void *mem = frame.getEnv()->GetPrimitiveArrayCritical(source, &isCopy);
	JP_TRACE_JAVA("GetPrimitiveArrayCritical", mem);

	std::vector<Py_ssize_t> indices(view.ndim, 0);
	int u = view.ndim - 1;
	for (Py_ssize_t idx = 0; idx < len; ++idx)
	{
		char *destPtr = destBuffer.getBufferPtr(indices);
		const char *srcPtr = (const char*) mem + (start + idx * step) * itemsize;
		memcpy(destPtr, srcPtr, (size_t) itemsize);

		// Odometer-increment the destination index vector.
		for (int d = u; d >= 0; --d)
		{
			if (++indices[d] < view.shape[d])
				break;
			indices[d] = 0;
		}
	}

	JP_TRACE_JAVA("ReleasePrimitiveArrayCritical", mem);
	frame.getEnv()->ReleasePrimitiveArrayCritical(source, mem, JNI_ABORT);
}

/**
 * Bulk-copy an arbitrary-shape/strided caller-owned Python buffer into a
 * (possibly strided/sliced) 1-D primitive Java array (JArray.pushFrom's
 * general path) -- the mirror of copyArrayToBuffer above, converting
 * rather than reinterpreting.
 *
 * Used whenever the fast contiguous+dtype-matching path (a single
 * Set<Type>ArrayRegion call, see JPArray::pushFrom) does not apply, i.e.
 * real value conversion is needed (dtype coercion, byte swap,
 * half-precision decode) and/or the source is non-contiguous or
 * N-dimensional. `converter` must already be resolved (see getConverter).
 * A single JNI critical section covers the whole destination array --
 * unlike the multi-dimensional push path, there is no per-row pinning
 * concern here since JPArray itself is always flat (one Java array
 * object), so this is already about as cheap as a converting copy can be
 * without leaving C++ entirely.
 *
 * jvalue is a plain union (every member aliases the same storage from
 * byte 0), so writing the itemsize-wide prefix of a freshly-converted
 * jvalue is exactly the converted value in `dest`'s own primitive type --
 * no per-type dispatch needed here, unlike convertMultiArrayObject's pack
 * function pointer, because there is no PyObject wrapping step in the way.
 */
inline void copyBufferToArray(JPJavaFrame &frame, jarray dest,
		jsize start, jsize step, jsize len, Py_ssize_t itemsize,
		jconverter converter, JPPyBuffer &srcBuffer)
{
	Py_buffer& view = srcBuffer.getView();
	jboolean isCopy;
	void *mem = frame.getEnv()->GetPrimitiveArrayCritical(dest, &isCopy);
	JP_TRACE_JAVA("GetPrimitiveArrayCritical", mem);

	std::vector<Py_ssize_t> indices(view.ndim, 0);
	int u = view.ndim - 1;
	for (Py_ssize_t idx = 0; idx < len; ++idx)
	{
		char *srcPtr = srcBuffer.getBufferPtr(indices);
		jvalue v = converter(srcPtr);
		char *destPtr = (char*) mem + (start + idx * step) * itemsize;
		memcpy(destPtr, &v, (size_t) itemsize);

		// Odometer-increment the source index vector.
		for (int d = u; d >= 0; --d)
		{
			if (++indices[d] < view.shape[d])
				break;
			indices[d] = 0;
		}
	}

	JP_TRACE_JAVA("ReleasePrimitiveArrayCritical", mem);
	frame.getEnv()->ReleasePrimitiveArrayCritical(dest, mem, 0);
}

template <typename base_t>
class JPConversionLong : public JPIndexConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (!PyLong_CheckExact(match.object) && !PyIndex_Check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		if (match.type == JPMatch::_exact)
		{
			auto val = (jlong) PyLong_AsUnsignedLongLongMask(match.object);
			if (val == -1)
				JP_PY_CHECK();  // GCOVR_EXCL_LINE
			base_t::field(res) = (typename base_t::type_t) val;
		} else
		{
			auto val = (jlong) PyLong_AsLongLong(match.object);
			if (val == -1)
				JP_PY_CHECK();  // GCOVR_EXCL_LINE
			base_t::field(res) = (typename base_t::type_t) base_t::assertRange(val);
		}
		return res;
	}
} ;

template <typename base_t>
class JPConversionLongNumber : public JPConversionLong<base_t>
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (!PyNumber_Check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_explicit;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyObject *typing = PyImport_AddModule("jpype.protocol");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "SupportsFloat"));
		PyList_Append(info.expl, proto.get());
	}

	jvalue convert(JPMatch &match) override
	{
		JPPyObject obj = JPPyObject::call(PyNumber_Long(match.object));
		match.object = obj.get();
		return JPConversionLong<base_t>::convert(match);
	}
} ;

template <typename base_t>
class JPConversionLongWiden : public JPConversion
{
public:
	// GCOVR_EXCL_START

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		return JPMatch::_none; // Not used
	}

	void getInfo(JPClass *cls, JPConversionInfo &info)  override
	{
		// Not used
	}
	// GCOVR_EXCL_STOP

	jvalue convert(JPMatch &match) override
	{
		jvalue ret;
		base_t::field(ret) = (typename base_t::type_t) (dynamic_cast<JPPrimitiveType*>(
				match.getJPClass()))->getAsLong(match.getJValue());
		return ret;
	}
} ;

template <typename base_t>
class JPConversionAsFloat : public JPNumberConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (!PyNumber_Check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		double val = PyFloat_AsDouble(match.object);
		if (val == -1.0)
			JP_PY_CHECK();  // GCOVR_EXCL_LINE
		base_t::field(res) = (typename base_t::type_t) val;
		return res;
	}
} ;

template <typename base_t>
class JPConversionLongAsFloat : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (!PyLong_Check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyList_Append(info.implicit, (PyObject*) & PyLong_Type);
	}

	jvalue convert(JPMatch &match) override
	{
		jvalue res;
		jdouble v = PyLong_AsDouble(match.object);
		if (v == -1.0)
			JP_PY_CHECK();  // GCOVR_EXCL_LINE
		base_t::field(res) = (typename base_t::type_t) v;
		return res;
	}
} ;

template <typename base_t>
class JPConversionFloatWiden : public JPConversion
{
public:

	// GCOVR_EXCL_START

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		return JPMatch::_none;  // not used
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
	}
	// GCOVR_EXCL_STOP

	jvalue convert(JPMatch &match) override
	{
		jvalue ret;
		base_t::field(ret) = (typename base_t::type_t) (dynamic_cast<JPPrimitiveType*>( match.getJPClass()))->getAsDouble(match.getJValue());
		return ret;
	}
} ;

#endif /* JP_PRIMITIVE_ACCESSOR_H */
