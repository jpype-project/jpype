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
#include "jpype.h"
#include "pyjp.h"
#include "jp_array.h"
#include "jp_arrayclass.h"
#include "jp_primitive_accessor.h"

// Note: java represents arrays of zero length as null, thus we
// need to be careful to handle these properly.  We need to
// carry them around so that we can match types.

JPArray::JPArray(const JPValue &value)
: m_Object((jarray) value.getValue().l)
{
	m_Class = dynamic_cast<JPArrayClass*>( value.getClass());
	JPJavaFrame frame = JPJavaFrame::outer();
	JP_TRACE_IN("JPArray::JPArray");
	ASSERT_NOT_NULL(m_Class);
	JP_TRACE(m_Class->toString());

	// We will use this during range checks, so cache it
	if (m_Object.get() == nullptr)
		m_Length = 0;  // GCOVR_EXCL_LINE
	else
		m_Length = frame.GetArrayLength(m_Object.get());

	m_Step = 1;
	m_Start = 0;
	m_Slice = false;

	JP_TRACE_OUT;
}

JPArray::JPArray(JPArray* instance, jsize start, jsize stop, jsize step)
: m_Object((jarray) instance->getJava())
{
	JP_TRACE_IN("JPArray::JPArraySlice");
	m_Class = instance->m_Class;
	m_Step = step * instance->m_Step;
	m_Start = instance->m_Start + instance->m_Step*start;
	if (step > 0)
		m_Length =  (stop - start - 1 + step) / step;
	else
		m_Length =  (stop - start + 1 + step) / step;
	if (m_Length < 0)
		m_Length = 0;  // GCOVR_EXCL_LINE
	m_Slice = true;
	JP_TRACE_OUT;
}

JPArray::~JPArray()
= default;

JPArray* JPArray::create(const JPValue& value)
{
	auto* arrayClass = dynamic_cast<JPArrayClass*>(value.getClass());
	ASSERT_NOT_NULL(arrayClass);
	return arrayClass->getComponentType()->createArrayWrapper(value);
}

jsize JPArray::checkIndex(jsize ndx) const
{
	if (ndx < 0)
		ndx += m_Length;
	if (ndx >= m_Length || ndx < 0)
		JP_RAISE(PyExc_IndexError, "array index out of bounds");
	return ndx;
}

jsize JPArray::getLength() const
{
	return m_Length;
}

void JPArray::setRange(jsize start, jsize length, jsize step, PyObject* val)
{
	JP_TRACE_IN("JPArray::setRange");

	// Make sure it is an iterable before we start
	if (!PySequence_Check(val))
		JP_RAISE(PyExc_TypeError, "can only assign a sequence");

	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass* compType = m_Class->getComponentType();
	JPPySequence seq = JPPySequence::use(val);
	long plength = (long) seq.size();

	JP_TRACE("Verify lengths", length, plength);
	if ((long) length != plength)
	{
		// Python would allow mismatching size by growing or shrinking
		// the length of the array.  But java arrays are immutable in length.
		std::stringstream out;
		out << "Slice assignment must be of equal lengths : " << length << " != " << plength;
		JP_RAISE(PyExc_ValueError, out.str());
	}

	JP_TRACE("Call component set range");
	jsize i0 = m_Start + m_Step*start;
	compType->setArrayRange(frame, m_Object.get(), i0, length, m_Step*step, val);
	JP_TRACE_OUT;
}

void JPArray::setItem(jsize ndx, PyObject* val)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass* compType = m_Class->getComponentType();

	if (ndx < 0)
		ndx += m_Length;

	if (ndx >= m_Length || ndx < 0)
		JP_RAISE(PyExc_IndexError, "java array assignment out of bounds");

	compType->setArrayItem(frame, m_Object.get(), m_Start + ndx*m_Step, val);
}

JPArrayObject::JPArrayObject(const JPValue& array)
: JPArray(array)
{
}

JPArrayObject::JPArrayObject(JPArrayObject* src, jsize start, jsize stop, jsize step)
: JPArray(src, start, stop, step)
{
}

JPPyObject JPArrayObject::getItem(jsize ndx)
{
	ndx = checkIndex(ndx);
	JPClass* compType = m_Class->getComponentType();
	JPJavaFrame frame = JPJavaFrame::outer();
	return compType->getArrayItem(frame, m_Object.get(), m_Start + ndx * m_Step);
}

JPArray* JPArrayObject::slice(jsize start, jsize stop, jsize step)
{
	return new JPArrayObject(this, start, stop, step);
}

jarray JPArray::clone(JPJavaFrame& frame, PyObject* obj)
{
	JPValue value = m_Class->newArray(frame, m_Length);
	JPClass* compType = m_Class->getComponentType();
	auto out = (jarray) value.getValue().l;
	compType->setArrayRange(frame, out, 0, m_Length, 1, obj);
	return out;
}

namespace
{

// Copies `len` itemsize-wide elements from a flat, row-major scratch
// buffer into an arbitrary-shape/strided destination Py_buffer, walking
// the destination's own shape/strides -- the flat-source counterpart to
// JPPyBuffer::getBufferPtr's own pointer arithmetic (jp_pythontypes.cpp),
// duplicated here rather than shared because the source here is a plain
// flat scratch buffer, not itself a Py_buffer. Used by pullToRectangular
// below whenever the destination isn't C-contiguous, after the leaf data
// has already been bulk-collected into scratch memory by
// Support.collectMultiArrayToBuffer.
void copyFlatToBufferView(const char *src, Py_ssize_t itemsize, Py_ssize_t len, Py_buffer &view)
{
	std::vector<Py_ssize_t> indices(view.ndim, 0);
	int u = view.ndim - 1;
	for (Py_ssize_t idx = 0; idx < len; ++idx)
	{
		char *pointer = (char*) view.buf;
		// The Py_buffer feeding this is always opened with PyBUF_STRIDES
		// (see pullToRectangular below), and the buffer protocol guarantees
		// a compliant exporter fills view.strides whenever that flag is
		// requested -- so view.strides == nullptr shouldn't happen for any
		// real source. Kept as a defensive fallback (same contiguous-index
		// arithmetic as JPPyBuffer::getBufferPtr's own copy of this case,
		// jp_pythontypes.cpp) in case that contract is ever violated.
		// GCOVR_EXCL_START
		if (view.strides == nullptr)
		{
			Py_ssize_t index = 0;
			for (int i = 0; i < view.ndim; i++)
				index = index * view.shape[i] + indices[i];
			pointer += index * view.itemsize;
		} else
		// GCOVR_EXCL_STOP
		{
			for (int i = 0; i < view.ndim; i++)
			{
				pointer += view.strides[i] * indices[i];
				// suboffsets is only ever populated when PyBUF_INDIRECT is
				// requested; this call site never requests it, so
				// view.suboffsets is nullptr by contract, not just in
				// practice. Kept for the same defense-in-depth reason as
				// the strides==nullptr branch above.
				if (view.suboffsets != nullptr && view.suboffsets[i] >= 0)  // GCOVR_EXCL_LINE
					pointer = *((char**) pointer) + view.suboffsets[i];  // GCOVR_EXCL_LINE
			}
		}
		memcpy(pointer, src + idx * itemsize, (size_t) itemsize);

		for (int d = u; d >= 0; --d)
		{
			if (++indices[d] < view.shape[d])
				break;
			indices[d] = 0;
		}
	}
}

// Validates a rectangular Java array's shape (from collectRectangular's
// [1] element) against a Python-side Py_buffer's own ndim/shape --
// shared by pullToRectangular and pushFromRectangular below, both of
// which need this same per-dimension check (stricter than a flattened
// total-count comparison) before touching either side's memory.
void validateRectangularShape(JPJavaFrame &frame, jobjectArray collected, Py_buffer &view)
{
	jobject shapeObj = frame.GetObjectArrayElement(collected, 1);
	JPPrimitiveArrayAccessor<jintArray, jint*> accessor(frame, (jintArray) shapeObj,
			&JPJavaFrame::GetIntArrayElements, &JPJavaFrame::ReleaseIntArrayElements);
	jint *shape = accessor.get();
	jsize shapeLen = frame.GetArrayLength((jarray) shapeObj);
	// Both callers already establish view.ndim == depth == the collected
	// array's own dimensionality before reaching here (pullTo/pushFrom's
	// own ndim check, and pullToRectangular/pushFromRectangular's recursion
	// keeping depth and view.ndim in lockstep at every level) -- so
	// shapeLen and view.ndim can't actually diverge. Kept as a defensive
	// invariant check, not an expected-reachable path.
	if (shapeLen != view.ndim)  // GCOVR_EXCL_LINE
		JP_RAISE(PyExc_ValueError, "mismatched size");  // GCOVR_EXCL_LINE
	for (int i = 0; i < shapeLen; ++i)
		if (shape[i] != view.shape[i])
			JP_RAISE(PyExc_ValueError, "mismatched size");
	accessor.abort();
}

// Slices off row `i` of the outermost dimension of `view`, one dimension
// shallower -- shared by pullToRectangular's and pushFromRectangular's
// depth>4 recursion, which both peel the outermost dimension the same
// way regardless of transfer direction.
Py_buffer sliceOuterDim(Py_buffer &view, jsize i)
{
	Py_buffer subView = view;
	subView.ndim = view.ndim - 1;
	subView.shape = view.shape + 1;
	if (view.strides != nullptr)
	{
		subView.buf = (char*) view.buf + view.strides[0] * i;
		subView.strides = view.strides + 1;
	} else
	// GCOVR_EXCL_START -- see copyFlatToBufferView's comment: callers always
	// open with PyBUF_STRIDES, so this contiguous-index fallback is
	// defensive, not an expected-reachable path.
	{
		Py_ssize_t rowElems = 1;
		for (int d = 1; d < view.ndim; ++d)
			rowElems *= view.shape[d];
		subView.buf = (char*) view.buf + rowElems * view.itemsize * i;
	}
	// GCOVR_EXCL_STOP
	// suboffsets is only populated under PyBUF_INDIRECT, which callers here
	// never request -- dead by contract, kept for defense-in-depth.
	if (view.suboffsets != nullptr)  // GCOVR_EXCL_LINE
		subView.suboffsets = view.suboffsets + 1;  // GCOVR_EXCL_LINE
	return subView;
}

// Copies `len` itemsize-wide elements out of an arbitrary-shape/strided
// source Py_buffer into a flat, row-major scratch buffer -- the
// push-direction mirror of copyFlatToBufferView above (source and
// destination roles swapped). Used by pushFromRectangular whenever the
// source isn't C-contiguous, to assemble a flat buffer suitable for a
// single Support.fillFromBufferIntoRectangular JNI call.
void copyBufferViewToFlat(Py_buffer &view, char *dest, Py_ssize_t itemsize, Py_ssize_t len)
{
	std::vector<Py_ssize_t> indices(view.ndim, 0);
	int u = view.ndim - 1;
	for (Py_ssize_t idx = 0; idx < len; ++idx)
	{
		char *pointer = (char*) view.buf;
		// See copyFlatToBufferView's identical comment above -- the source
		// here is likewise always opened with PyBUF_STRIDES (see
		// pushFromRectangular below), so this branch is a defensive
		// fallback, not an expected-reachable path.
		// GCOVR_EXCL_START
		if (view.strides == nullptr)
		{
			Py_ssize_t index = 0;
			for (int i = 0; i < view.ndim; i++)
				index = index * view.shape[i] + indices[i];
			pointer += index * view.itemsize;
		} else
		// GCOVR_EXCL_STOP
		{
			for (int i = 0; i < view.ndim; i++)
			{
				pointer += view.strides[i] * indices[i];
				if (view.suboffsets != nullptr && view.suboffsets[i] >= 0)  // GCOVR_EXCL_LINE
					pointer = *((char**) pointer) + view.suboffsets[i];  // GCOVR_EXCL_LINE
			}
		}
		memcpy(dest + idx * itemsize, pointer, (size_t) itemsize);

		for (int d = u; d >= 0; --d)
		{
			if (++indices[d] < view.shape[d])
				break;
			indices[d] = 0;
		}
	}
}

// Bulk-fills a rectangular N-D destination Py_buffer straight from a
// rectangular primitive Java array, for JPArray::pullTo's N-D case.
// depth<=4: one Support.collectRectangular call (Java-side leaf
// discovery, capped at 4 dims to bound a single JNI round trip's own
// cost -- not a supported-depth limit, see jp_convert.cpp's
// tryFastMultiArrayBuffer/Support.java's fillFromBuffer for the
// write-direction counterpart, which has no such cap at all) collects
// every leaf array's identity in one call, then either a direct
// DirectByteBuffer handoff (C-contiguous destination) or a collect-to-
// scratch-then-strided-copy (any other destination) fills it.
// depth>4: peels the outermost dimension and recurses once per
// top-level slice, each of which is depth-1 shallower -- this is what
// safely carries the fast path one level beyond collectRectangular's own
// cap, and generalizes to any depth.
// Raises on a non-rectangular (ragged) source (collectRectangular
// returns null) or a shape mismatch against the destination -- no fill/
// pad semantics, by design (see RESULTS.md/plan notes).
void pullToRectangular(JPJavaFrame &frame, jarray arr, JPPrimitiveType *pcls, int depth, Py_buffer &view)
{
	if (depth <= 4)
	{
		auto collected = (jobjectArray) frame.collectRectangular(arr);
		if (collected == nullptr)
			JP_RAISE(PyExc_TypeError, "pullTo requires a rectangular primitive array");
		validateRectangularShape(frame, collected, view);

		Py_ssize_t total = 1;
		for (int i = 0; i < view.ndim; ++i)
			total *= view.shape[i];

		if (PyBuffer_IsContiguous(&view, 'C'))
		{
			jobject directBuf = frame.NewDirectByteBuffer(view.buf, total * view.itemsize);
			frame.collectMultiArrayToBuffer(pcls->getTypeCode(), collected, directBuf);
		} else
		{
			std::vector<char> temp((size_t) (total * view.itemsize));
			jobject directBuf = frame.NewDirectByteBuffer(temp.data(), total * view.itemsize);
			frame.collectMultiArrayToBuffer(pcls->getTypeCode(), collected, directBuf);
			copyFlatToBufferView(temp.data(), view.itemsize, total, view);
		}
		return;
	}

	jsize n = frame.GetArrayLength(arr);
	if (n != view.shape[0])
		JP_RAISE(PyExc_ValueError, "mismatched size");
	for (jsize i = 0; i < n; ++i)
	{
		auto sub = (jarray) frame.GetObjectArrayElement((jobjectArray) arr, i);
		Py_buffer subView = sliceOuterDim(view, i);
		pullToRectangular(frame, sub, pcls, depth - 1, subView);
	}
}

// Push-direction mirror of pullToRectangular above -- bulk-fills a
// rectangular N-D Java array's existing leaf arrays in place from a
// rectangular N-D source Py_buffer (JPArray::pushFrom's N-D case). Same
// depth<=4/depth>4 split, same rectangular-only gate, same
// shape-validation rules; the only difference is direction (Support.
// fillFromBufferIntoRectangular writing into the array's own leaves
// rather than Support.collectMultiArrayToBuffer reading out of them) and,
// for a non-contiguous source, assembling the flat scratch buffer by
// reading the source (copyBufferViewToFlat) rather than writing the
// destination.
void pushFromRectangular(JPJavaFrame &frame, jarray arr, JPPrimitiveType *pcls, int depth, Py_buffer &view)
{
	if (depth <= 4)
	{
		auto collected = (jobjectArray) frame.collectRectangular(arr);
		if (collected == nullptr)
			JP_RAISE(PyExc_TypeError, "pushFrom requires a rectangular primitive array");
		validateRectangularShape(frame, collected, view);

		Py_ssize_t total = 1;
		for (int i = 0; i < view.ndim; ++i)
			total *= view.shape[i];

		if (PyBuffer_IsContiguous(&view, 'C'))
		{
			jobject directBuf = frame.NewDirectByteBuffer(view.buf, total * view.itemsize);
			frame.fillBufferIntoMultiArray(pcls->getTypeCode(), collected, directBuf);
		} else
		{
			std::vector<char> temp((size_t) (total * view.itemsize));
			copyBufferViewToFlat(view, temp.data(), view.itemsize, total);
			jobject directBuf = frame.NewDirectByteBuffer(temp.data(), total * view.itemsize);
			frame.fillBufferIntoMultiArray(pcls->getTypeCode(), collected, directBuf);
		}
		return;
	}

	jsize n = frame.GetArrayLength(arr);
	if (n != view.shape[0])
		JP_RAISE(PyExc_ValueError, "mismatched size");
	for (jsize i = 0; i < n; ++i)
	{
		auto sub = (jarray) frame.GetObjectArrayElement((jobjectArray) arr, i);
		Py_buffer subView = sliceOuterDim(view, i);
		pushFromRectangular(frame, sub, pcls, depth - 1, subView);
	}
}

} // namespace

void JPArray::pullTo(PyObject* dest)
{
	JP_TRACE_IN("JPArray::pullTo");
	auto *compType = dynamic_cast<JPPrimitiveType*>(m_Class->getComponentType());
	JPJavaFrame frame = JPJavaFrame::outer();
	if (compType == nullptr)
	{
		JPPrimitiveType *pcls = m_Class->getMultiArrayLeaf();
		int depth = m_Class->getMultiArrayDepth();
		if (pcls == nullptr)
			JP_RAISE(PyExc_TypeError, "pullTo requires a primitive array");

		JPPyBuffer buffer(dest, PyBUF_WRITABLE | PyBUF_STRIDES | PyBUF_FORMAT);
		if (!buffer.valid())
			JP_PY_CHECK();
		Py_buffer& view = buffer.getView();
		if (view.ndim != depth)
			JP_RAISE(PyExc_ValueError, "mismatched size");
		if (view.itemsize != pcls->getItemSize())
			JP_RAISE(PyExc_TypeError, "mismatched item size");

		pullToRectangular(frame, m_Object.get(), pcls, depth, view);
		return;
	}

	JPPyBuffer buffer(dest, PyBUF_WRITABLE | PyBUF_STRIDES | PyBUF_FORMAT);
	if (!buffer.valid())
		JP_PY_CHECK();
	Py_buffer& view = buffer.getView();

	Py_ssize_t total = 1;
	for (int i = 0; i < view.ndim; ++i)
		total *= view.shape[i];
	if (total != m_Length)
		JP_RAISE(PyExc_ValueError, "mismatched size");
	if (view.itemsize != compType->getItemSize())
		JP_RAISE(PyExc_TypeError, "mismatched item size");

	// Fast path: a single Get<Type>ArrayRegion call straight into the
	// destination memory. Requires a unit-step source (no sliced array)
	// and a C-contiguous destination (any number of dims, so long as the
	// whole thing is one contiguous run).
	if (m_Step == 1 && view.suboffsets == nullptr && PyBuffer_IsContiguous(&view, 'C'))
	{
		compType->copyElements(frame, m_Object.get(), m_Start, m_Length, view.buf, 0);
	} else
	{
		// General path: stepped source and/or non-contiguous/N-D destination.
		copyArrayToBuffer(frame, m_Object.get(), m_Start, m_Step, m_Length,
				compType->getItemSize(), buffer);
	}
	JP_TRACE_OUT;
}

void JPArray::pushFrom(PyObject* src)
{
	JP_TRACE_IN("JPArray::pushFrom");
	auto *compType = dynamic_cast<JPPrimitiveType*>(m_Class->getComponentType());
	JPJavaFrame frame = JPJavaFrame::outer();
	if (compType == nullptr)
	{
		JPPrimitiveType *pcls = m_Class->getMultiArrayLeaf();
		int depth = m_Class->getMultiArrayDepth();
		if (pcls == nullptr)
			JP_RAISE(PyExc_TypeError, "pushFrom requires a primitive array");

		JPPyBuffer buffer(src, PyBUF_STRIDES | PyBUF_FORMAT);
		if (!buffer.valid())
			JP_PY_CHECK();
		Py_buffer& view = buffer.getView();
		if (view.ndim != depth)
			JP_RAISE(PyExc_ValueError, "mismatched size");
		if (view.itemsize != pcls->getItemSize())
			JP_RAISE(PyExc_TypeError, "mismatched item size");

		pushFromRectangular(frame, m_Object.get(), pcls, depth, view);
		return;
	}

	JPPyBuffer buffer(src, PyBUF_STRIDES | PyBUF_FORMAT);
	if (!buffer.valid())
		JP_PY_CHECK();
	Py_buffer& view = buffer.getView();

	Py_ssize_t total = 1;
	for (int i = 0; i < view.ndim; ++i)
		total *= view.shape[i];
	if (total != m_Length)
		JP_RAISE(PyExc_ValueError, "mismatched size");

	char code[2] = {(char) tolower(compType->getTypeCode()), 0};
	const char *format = view.format != nullptr ? view.format : "B";
	jconverter converter = getConverter(format, (int) view.itemsize, code);
	// getConverter() raises ValueError itself on an unrecognized format
	// rather than returning nullptr (see jp_convert.cpp) -- dead, kept
	// defensively in case that contract ever changes.
	if (converter == nullptr)  // GCOVR_EXCL_LINE
		JP_RAISE(PyExc_TypeError, "No type converter found");  // GCOVR_EXCL_LINE

	// Fast path: source needs no per-element conversion at all (matching
	// dtype, native byte order) -- a single Set<Type>ArrayRegion call
	// straight from the source memory. Requires a unit-step destination
	// (no sliced array) and a C-contiguous source (any number of dims, so
	// long as the whole thing is one contiguous run). Unlike the
	// multi-dim push path (JPConversionMultiArrayBuffer), JPArray is
	// always a single flat Java array, so there is no per-row pinning
	// cost to dodge for RAW_SWAPPED/RAW_HALF_* here -- the general path
	// below already pins the whole destination array exactly once
	// (copyBufferToArray), so only the strictly-cheapest case (no
	// conversion at all) earns a dedicated fast path.
	JPRawTransferMode mode = classifyRawTransfer(converter, compType, format, (int) view.itemsize, code);
	if (mode == RAW_NATIVE && m_Step == 1 && view.suboffsets == nullptr && PyBuffer_IsContiguous(&view, 'C'))
	{
		compType->setElements(frame, m_Object.get(), m_Start, m_Length, view.buf, 0);
	} else
	{
		// General path: real value conversion (dtype coercion, byte swap,
		// half-precision) and/or stepped destination and/or
		// non-contiguous/N-D source. Single JNI critical section for the
		// whole destination array.
		copyBufferToArray(frame, m_Object.get(), m_Start, m_Step, m_Length,
				compType->getItemSize(), converter, buffer);
	}
	JP_TRACE_OUT;
}

JPPyObject JPArray::toList(JPPrimitiveType* dtype, bool wrap)
{
	JP_TRACE_IN("JPArray::toList");
	auto *compType = dynamic_cast<JPPrimitiveType*>(m_Class->getComponentType());
	if (compType != nullptr)
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		return compType->getArrayRange(frame, m_Object.get(), m_Start, m_Step, m_Length, dtype, wrap);
	}

	// Object[] or a nested array class -- no bulk read possible (each
	// element can be a distinct runtime type), but recurse into any
	// nested Java array so multi-dim primitive arrays still come out as
	// genuinely nested Python lists. dtype/wrap pass through unchanged so
	// they apply once recursion reaches the primitive leaf level.
	JPPyObject list = JPPyObject::call(PyList_New(m_Length));
	for (jsize i = 0; i < m_Length; ++i)
	{
		JPPyObject item = getItem(i);
		if (item.get() != nullptr && PyObject_IsInstance(item.get(), (PyObject*) PyJPArray_Type))
			item = ((PyJPArray*) item.get())->m_Array->toList(dtype, wrap);
		PyList_SET_ITEM(list.get(), i, item.keep());
	}
	return list;
	JP_TRACE_OUT;
}

JPArrayView::JPArrayView(JPArray* array)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	m_Array = array;
	m_RefCount = 0;
	m_Buffer.obj = nullptr;
	m_Buffer.ndim = 1;
	m_Buffer.suboffsets = nullptr;
	auto *type = dynamic_cast<JPPrimitiveType*>( array->getClass()->getComponentType());
	type->getView(*this);
	m_Strides[0] = m_Buffer.itemsize * array->m_Step;
	m_Shape[0] = array->m_Length;
	m_Buffer.buf = (char*) m_Memory + m_Buffer.itemsize * array->m_Start;
	m_Buffer.len = array->m_Length * m_Buffer.itemsize;
	m_Buffer.shape = m_Shape;
	m_Buffer.strides = m_Strides;
	m_Buffer.readonly = 1;
	m_Owned = false;
}

JPArrayView::JPArrayView(JPArray* array, jobject collection)
{
	JP_TRACE_IN("JPArrayView::JPArrayView");
	// All of the work has already been done by org.jpype.Utilities
	JPJavaFrame frame = JPJavaFrame::outer();
	m_Array = array;

	jobject item0 = frame.GetObjectArrayElement((jobjectArray) collection, 0);
	jobject item1 = frame.GetObjectArrayElement((jobjectArray) collection, 1);

	// First element is the primitive type that we are packing the array from
	auto *componentType = dynamic_cast<JPPrimitiveType*>(
			frame.findClass((jclass) item0));

	// Second element is the shape of the array from which we compute the
	// memory size, the shape, and strides
	int dims;
	Py_ssize_t itemsize;
	Py_ssize_t sz;
	{
		JPPrimitiveArrayAccessor<jintArray, jint*> accessor(frame, (jintArray) item1,
				&JPJavaFrame::GetIntArrayElements, &JPJavaFrame::ReleaseIntArrayElements);
		jint* shape2 = accessor.get();
		dims = frame.GetArrayLength((jarray) item1);
		itemsize = componentType->getItemSize();
		sz = itemsize;
		for (int i = 0; i < dims; ++i)
		{
			m_Shape[i] = shape2[i];
			sz *= m_Shape[i];
		}
		accessor.abort();
	}
	Py_ssize_t stride = itemsize;
	for (int i = 0; i < dims; ++i)
	{
		int n = dims - 1 - i;
		m_Strides[n] = stride;
		stride *= m_Shape[n];
	}

	m_RefCount = 0;
	m_Memory = new char[sz];
	m_Owned = true;

	// A single JNI entry into Support.collectToBuffer instead of one reflective
	// GetObjectArrayElement plus one Get<Type>ArrayRegion (via
	// copyElements) per leaf array -- the whole remaining-elements walk
	// and bulk write into m_Memory happens in pure Java (including the
	// serial-vs-parallel decision -- see Support.leafRange), wrapped as a
	// direct buffer so no further JNI calls are needed at all.
	jobject directBuf = frame.NewDirectByteBuffer(m_Memory, sz);
	frame.collectMultiArrayToBuffer(componentType->getTypeCode(), collection, directBuf);

	// Copy values into Python buffer for consumption
	m_Buffer.obj = nullptr;
	m_Buffer.ndim = dims;
	m_Buffer.suboffsets = nullptr;
	m_Buffer.itemsize = itemsize;
	m_Buffer.format = const_cast<char*> (componentType->getBufferFormat());
	m_Buffer.buf = (char*) m_Memory + m_Buffer.itemsize * array->m_Start;
	m_Buffer.len = sz;
	m_Buffer.shape = m_Shape;
	m_Buffer.strides = m_Strides;
	m_Buffer.readonly = 1;
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}

JPArrayView::~JPArrayView()
{
	if (m_Owned)
		delete [] (char*) m_Memory;
}

void JPArrayView::reference()
{
	m_RefCount++;
}

bool JPArrayView::unreference()
{
	m_RefCount--;
	auto *type = dynamic_cast<JPPrimitiveType*>( m_Array->getClass()->getComponentType());
	if (m_RefCount == 0 && !m_Owned)
		type->releaseView(*this);
	return m_RefCount == 0;
}
