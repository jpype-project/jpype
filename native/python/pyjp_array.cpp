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
#include <cstddef>
#include "jpype.h"
#include "pyjp.h"
#include "jp_array.h"
#include "jp_arrayclass.h"
#include "jp_primitive_accessor.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Native iterator for JArray (list(arr), for x in arr, tuple(arr), *arr,
// comprehensions, ...). Deliberately NOT just relying on the sq_item slot
// below plus CPython's generic PySeqIter fallback: PySeqIter detects the
// end of iteration by calling sq_item one index past the end and
// catching IndexError, and profiling found that raising *any* exception
// while a JPJavaFrame is open and then popping that frame is expensive
// here (thousands of futex calls per hit -- looks like JVM safepoint
// synchronization triggered by the frame-pop/exception-state interaction,
// not anything in jpype's own code) -- cheap for a real error, but much
// too expensive to pay on every normal iteration's last step, and the
// cost is worse than proportionally so for many small arrays (deep
// multi-dimensional pulls) than for one large flat one. This type
// mirrors CPython's own list/tuple iterators instead: an explicit length
// check before ever calling into array access, returning NULL with *no*
// exception set to signal a clean stop -- the same trick that lets
// list/tuple iteration avoid exception-raising overhead entirely.
struct PyJPArrayIter
{
	PyObject_HEAD
	PyJPArray *m_Array; // strong ref, cleared once exhausted
	Py_ssize_t m_Index;
};

static PyTypeObject *PyJPArrayIter_Type = nullptr;

static void PyJPArrayIter_dealloc(PyJPArrayIter *self)
{
	Py_CLEAR(self->m_Array);
	Py_TYPE(self)->tp_free(self);
}

static PyObject *PyJPArrayIter_iter(PyObject *self)
{
	Py_INCREF(self);
	return self;
}

static PyObject *PyJPArrayIter_next(PyJPArrayIter *self)
{
	JP_PY_TRY("PyJPArrayIter_next");
	if (self->m_Array == nullptr)
		return nullptr; // already exhausted
	// No JPJavaFrame constructed here on purpose -- JPArray::getItem() is
	// fully self-contained per concrete subclass (a primitive read needs
	// no frame at all on its common path; an object-array read pushes
	// its own outer() internally). Pushing one here "just in case" would
	// tax every element of every array type with a real
	// PushLocalFrame/PopLocalFrame pair regardless of whether the
	// concrete type ever needs one, exactly the per-call cost this whole
	// design exists to avoid.
	JPArray *array = self->m_Array->m_Array;
	if (array == nullptr || self->m_Index >= array->getLength())
	{
		// Clean stop, no exception -- see the design note above.
		Py_CLEAR(self->m_Array);
		return nullptr;
	}
	PyObject *result = array->getItem((jsize) self->m_Index).keep();
	self->m_Index++;
	return result;
	JP_PY_CATCH(nullptr);
}

static PyType_Slot arrayIterSlots[] = {
	{ Py_tp_dealloc, (void*) PyJPArrayIter_dealloc},
	{ Py_tp_iter,	 (void*) PyJPArrayIter_iter},
	{ Py_tp_iternext, (void*) PyJPArrayIter_next},
	{0}
};

static PyType_Spec arrayIterSpec = {
	"_jpype._JArrayIterator",
	sizeof (PyJPArrayIter),
	0,
	Py_TPFLAGS_DEFAULT,
	arrayIterSlots
};

static PyObject *PyJPArray_iter(PyJPArray *self)
{
	JP_PY_TRY("PyJPArray_iter");
	if (self->m_Array == nullptr)
		JP_RAISE(PyExc_ValueError, "Null array");
	auto *it = (PyJPArrayIter*) PyJPArrayIter_Type->tp_alloc(PyJPArrayIter_Type, 0);
	if (it == nullptr)
		return nullptr; // GCOVR_EXCL_LINE
	Py_INCREF(self);
	it->m_Array = self;
	it->m_Index = 0;
	return (PyObject*) it;
	JP_PY_CATCH(nullptr);
}

/**
 * Create a new object.
 *
 * This is only called from the Python side.
 *
 * @param type
 * @param args
 * @param kwargs
 * @return
 */
static PyObject *PyJPArray_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPArray_new");
	auto* self = (PyJPArray*) type->tp_alloc(type, 0);
	JP_PY_CHECK();
	self->m_Array = nullptr;
	self->m_View = nullptr;
	return (PyObject*) self;
	JP_PY_CATCH(nullptr);
}

static int PyJPArray_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPArray_init");

	// Cases here.
	//  -  We got here with a JPValue
	//  -  We get an integer. Just create a new array with desired size.
	//  -  We get a sequence. Allocate with desired size and call setItems.
	//  -  We get something else.... ???

	PyObject* v;
	if (!PyArg_ParseTuple(args, "O", &v))
		return -1;

	JPClass *cls = PyJPClass_getJPClass((PyObject*) Py_TYPE(self));
	auto* arrayClass = dynamic_cast<JPArrayClass*> (cls);
	if (arrayClass == nullptr)
		JP_RAISE(PyExc_TypeError, "Class must be array type");

	JPJavaFrame frame = JPJavaFrame::outer();

	JPClass *valueCls = PyJPValue_getJPClass(v);
	if (valueCls != nullptr)
	{
		auto* arrayClass2 = dynamic_cast<JPArrayClass*> (valueCls);
		if (arrayClass2 == nullptr)
			JP_RAISE(PyExc_TypeError, "Class must be array type");
		if (arrayClass2 != arrayClass)
			JP_RAISE(PyExc_TypeError, "Array class mismatch");

		// Check if the input is a PyJPArray and if it's a slice
		// If so, we need to clone it to get the actual sliced elements
		if (PyObject_IsInstance(v, (PyObject*) PyJPArray_Type))
		{
			JPArray* srcArray = ((PyJPArray*) v)->m_Array;
			if (srcArray->isSlice())
			{
				// Create a new array with the correct length and copy elements
				jsize sliceLength = srcArray->getLength();
				JPValue newArray = arrayClass->newArray(frame, sliceLength);
				((PyJPArray*) self)->m_Array = JPArray::create(newArray);
				((PyJPArray*) self)->m_Array->setRange(0, sliceLength, 1, v);
				PyJPValue_assignJavaSlot(frame, self, newArray);
				return 0;
			}
		}

		JPValue value(valueCls, PyJPValue_getJValue(frame, v));
		((PyJPArray*) self)->m_Array = JPArray::create(value);
		PyJPValue_assignJavaSlot(frame, self, value);
		return 0;
	}

	// Buffer-protocol fast path for a multi-dimensional target (int[][],
	// double[][][], ...) -- without this, a numpy array also satisfies
	// PySequence_Check below, so construction would always fall into the
	// generic newArray+setRange(0, length, 1, v) path. That's still fast
	// for a 1D target (JPClass::setArrayRange's *primitive* overrides
	// already try tryFastBufferPush internally), but for an N-D target the
	// componentType is itself an array class, so setArrayRange's generic
	// default implementation applies instead -- no buffer shortcut, one
	// findJavaConversion+set call per row. Same gate/fallback contract as
	// JPConversionMultiArrayBuffer::matches (jp_classhints.cpp) and
	// JArray.of()'s N-D branch (PyJPModule_convertBuffer, pyjp_module.cpp),
	// which both already reuse tryFastMultiArrayBuffer (jp_convert.cpp) the
	// same way.
	JPPrimitiveType *multiLeaf = arrayClass->getMultiArrayLeaf();
	int multiDepth = arrayClass->getMultiArrayDepth();
	if (multiLeaf != nullptr && multiDepth >= 2 && PyObject_CheckBuffer(v))
	{
		JPPyBuffer buffer(v, PyBUF_STRIDES | PyBUF_FORMAT);
		if (!buffer.valid())
		{
			PyErr_Clear();
		} else
		{
			Py_buffer &view = buffer.getView();
			jarray fast = nullptr;
			if (view.ndim == multiDepth)
			{
				try
				{
					jintArray jdims = buildDimsArray(frame, view);
					tryFastMultiArrayBuffer(frame, multiLeaf, buffer, jdims, fast);
				} catch (...)
				{
					// Declined -- e.g. an element format with no Java
					// primitive converter at all (a numpy object-dtype
					// array), which getConverter reports by raising rather
					// than returning nullptr. Fall through to the general
					// PySequence_Check path below, which raises the
					// appropriate TypeError for genuinely unconvertible
					// elements -- same outcome as if this fast path had
					// never been attempted.
					fast = nullptr;
				}
			}
			if (fast != nullptr)
			{
				JPClass *outType = frame.findClassForObject(fast);
				jvalue val;
				val.l = fast;
				JPValue value(outType, val);
				((PyJPArray*) self)->m_Array = JPArray::create(value);
				PyJPValue_assignJavaSlot(frame, self, value);
				return 0;
			}
		}
	}

	if (PySequence_Check(v))
	{
		JP_TRACE("Sequence");
		jlong length =  PySequence_Size(v);
		if (length < 0 || length > 2147483647)
			JP_RAISE(PyExc_ValueError, "Array size invalid");
		JPValue newArray = arrayClass->newArray(frame, (int) length);
		((PyJPArray*) self)->m_Array = JPArray::create(newArray);
		((PyJPArray*) self)->m_Array->setRange(0, (jsize) length, 1, v);
		PyJPValue_assignJavaSlot(frame, self, newArray);
		return 0;
	}

	if (PyIndex_Check(v))
	{
		JP_TRACE("Index");
		long long length = PyLong_AsLongLong(v);
		if (length < 0 || length > 2147483647)
			JP_RAISE(PyExc_ValueError, "Array size invalid");
		JPValue newArray = arrayClass->newArray(frame, (int) length);
		((PyJPArray*) self)->m_Array = JPArray::create(newArray);
		PyJPValue_assignJavaSlot(frame, self, newArray);
		return 0;
	}

	JP_FAULT_RETURN("PyJPArray_init.null", 0);
	JP_RAISE(PyExc_TypeError, "Invalid type");
	JP_PY_CATCH(-1);
}

static void PyJPArray_dealloc(PyJPArray *self)
{
	JP_PY_TRY("PyJPArray_dealloc");
	delete self->m_Array;
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH(); // GCOVR_EXCL_LINE
}

static PyObject *PyJPArray_repr(PyJPArray *self)
{
	JP_PY_TRY("PyJPArray_repr");
	return PyUnicode_FromFormat("<java array '%s'>", Py_TYPE(self)->tp_name);
	JP_PY_CATCH(nullptr);
}

static Py_ssize_t PyJPArray_len(PyJPArray *self)
{
	JP_PY_TRY("PyJPArray_len");
	// JPArray::getLength() returns a cached jsize set at construction --
	// no JNI call, so no frame (fast or real) is needed here at all.
	if (self->m_Array == nullptr)
		JP_RAISE(PyExc_ValueError, "Null array"); // GCOVR_EXCL_LINE
	return self->m_Array->getLength();
	JP_PY_CATCH(-1);
}

static PyObject* PyJPArray_length(PyJPArray *self, PyObject *closure)
{
	return PyLong_FromSsize_t(PyJPArray_len(self));
}

static PyObject *PyJPArray_sqItem(PyJPArray *self, Py_ssize_t index)
{
	JP_PY_TRY("PyJPArray_sqItem");
	// No JPJavaFrame constructed here -- see PyJPArrayIter_next's comment
	// above; JPArray::getItem() is fully self-contained per subclass.
	if (self->m_Array == nullptr)
		JP_RAISE(PyExc_ValueError, "Null array");

	// Bounds-check here, rather than letting JPArray::getItem's own check
	// raise, because CPython's built-in PySeqIter (which drives
	// list(arr)/for x in arr/tuple(arr)/etc -- see jpype/_jarray.py, no
	// Python-level __iter__ defined on purpose) detects end-of-iteration
	// by calling sq_item one index past the end and expecting IndexError.
	// That happens once per array on the hot path, not just on genuine
	// caller error -- but once per *innermost* array in a nested
	// structure, so for a deep multi-dim pull it fires thousands of
	// times. getItem's own out-of-bounds path goes through JP_RAISE (a
	// real C++ throw, caught by JP_PY_CATCH below) -- fine for a genuine
	// error, too expensive to pay on every iteration's normal end.
	// Confirmed by benchmark: without this check, multi-dim list() got
	// *slower* after switching to the native iterator, not faster.
	jsize length = self->m_Array->getLength();
	Py_ssize_t ndx = index;
	if (ndx < 0)
		ndx += length;
	if (ndx < 0 || ndx >= length)
	{
		PyErr_SetString(PyExc_IndexError, "array index out of bounds");
		return nullptr;
	}
	return self->m_Array->getItem((jsize) ndx).keep();
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPArray_getItem(PyJPArray *self, PyObject *item)
{
	JP_PY_TRY("PyJPArray_getArrayItem");
	// No JPJavaFrame constructed here on the general entry path -- see
	// PyJPArrayIter_next's comment. The index branch below needs none at
	// all (JPArray::getItem() is self-contained); only the slice branch
	// genuinely uses one (PyJPValue_assignJavaSlot/PyJPValue_getJValue),
	// so it constructs its own, scoped to just that branch.
	if (self->m_Array == nullptr)
		JP_RAISE(PyExc_ValueError, "Null array");

	if (PyIndex_Check(item))
	{
		Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return nullptr;  // GCOVR_EXCL_LINE
		return self->m_Array->getItem((jsize) i).keep();
	}

	if (PySlice_Check(item))
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		Py_ssize_t start, stop, step, slicelength;
		auto length = (Py_ssize_t) self->m_Array->getLength();

		if (PySlice_Unpack(item, &start, &stop, &step) < 0)
			return nullptr;

		slicelength = PySlice_AdjustIndices(length, &start, &stop, step);

		if (slicelength <= 0)
		{
			// This should point to a null array so we don't hold worthless
			// memory, but this is a low priority
			start = stop = 0;
			step = 1;
		}

		JPPyObject tuple = JPPyObject::call(PyTuple_New(0));

		JPPyObject newArray = JPPyObject::claim(Py_TYPE(self)->tp_new(Py_TYPE(self), tuple.get(), nullptr));

		// Copy over the JPValue
		PyJPValue_assignJavaSlot(frame, newArray.get(),
				JPValue(PyJPValue_getJPClass((PyObject*) self), PyJPValue_getJValue(frame, (PyObject*) self)));

		// Set up JPArray as slice
		JPArray *array = ((PyJPArray*) self)->m_Array;
		((PyJPArray*) newArray.get())->m_Array = array->slice(
				(jsize) start, (jsize) stop, (jsize) step);
		return newArray.keep();
	}

	JP_RAISE(PyExc_TypeError, "Unsupported getItem type");
	JP_PY_CATCH(nullptr);
}

static int PyJPArray_assignSubscript(PyJPArray *self, PyObject *item, PyObject *value)
{
	JP_PY_TRY("PyJPArray_assignSubscript");
	JPJavaFrame frame = JPJavaFrame::outer();
	// Verified with numpy that item deletion on immutable should
	// be ValueError
	if ( value == nullptr)
		JP_RAISE(PyExc_ValueError, "item deletion not supported");
	if (self->m_Array == nullptr)
		JP_RAISE(PyExc_ValueError, "Null array");

	// Watch out for self assignment
	if (PyObject_IsInstance(value, (PyObject*) PyJPArray_Type))
	{
		jobject v1 = PyJPValue_getJValue(frame, (PyObject*) self).l;
		jobject v2 = PyJPValue_getJValue(frame, (PyObject*) value).l;
		if (frame.equals(v1, v2))
			JP_RAISE(PyExc_ValueError, "self assignment not support currently");
	}

	if (PyIndex_Check(item))
	{
		Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return -1;  // GCOVR_EXCL_LINE
		self->m_Array->setItem((jsize) i, value);
		return 0;
	}

	if (PySlice_Check(item))
	{
		Py_ssize_t start, stop, step, slicelength;
		auto length = (Py_ssize_t) self->m_Array->getLength();

		if (PySlice_Unpack(item, &start, &stop, &step) < 0)
			return -1;

		slicelength = PySlice_AdjustIndices(length, &start, &stop, step);

		if (slicelength <= 0)
			return 0;

		self->m_Array->setRange((jsize) start, (jsize) slicelength, (jsize) step,  value);
		return 0;
	}
	PyErr_Format(PyExc_TypeError,
			"Java array indices must be integers or slices, not '%s'",
			Py_TYPE(item)->tp_name);
	JP_PY_CATCH(-1);
}

static void PyJPArray_releaseBuffer(PyJPArray *self, Py_buffer *view)
{
	JP_PY_TRY("PyJPArrayPrimitive_releaseBuffer");
	JPContext* context = JPContext_global;
	if (context->isRunning())
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		if (self->m_View == nullptr || !self->m_View->unreference())
			return;
	}
	delete self->m_View;
	self->m_View = nullptr;
	JP_PY_CATCH(); // GCOVR_EXCL_LINE
}

int PyJPArray_getBuffer(PyJPArray *self, Py_buffer *view, int flags)
{
	JP_PY_TRY("PyJPArray_getBuffer");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (self->m_Array == nullptr)
		JP_RAISE(PyExc_ValueError, "Null array");

	if (!self->m_Array->getClass()->isPrimitiveArray())
	{
		PyErr_SetString(PyExc_BufferError, "Java array is not primitive array");
		return -1;
	}

	if ((flags & PyBUF_WRITEABLE) == PyBUF_WRITEABLE)
	{
		PyErr_SetString(PyExc_BufferError, "Java array buffer is not writable");
		return -1;
	}

	//Check to see if we are a slice and clone it if necessary
	jarray obj = self->m_Array->getJava();
	if (self->m_Array->isSlice())
		obj = self->m_Array->clone(frame, (PyObject*) self);

	jobject result;
	try
	{
		// Collect the members into a rectangular array if possible.
		result = frame.collectRectangular(obj);
	} catch (...)
	{
		// No matter what happens we are only allowed to throw BufferError
		PyErr_SetString(PyExc_BufferError, "Problem in Java buffer extraction");
		return -1;
	}

	if (result == nullptr)
	{
		PyErr_SetString(PyExc_BufferError, "Java array buffer is not rectangular primitives");
		return -1;
	}

	// If it is rectangular so try to create a view
	try
	{
		if (self->m_View == nullptr)
			self->m_View = new JPArrayView(self->m_Array, result);
		JP_PY_CHECK();
		self->m_View->reference();
		*view = self->m_View->m_Buffer;

		// If strides are not requested and this is a slice then fail
		if ((flags & PyBUF_STRIDES) != PyBUF_STRIDES)
			view->strides = nullptr;

		// If shape is not requested
		if ((flags & PyBUF_ND) != PyBUF_ND)
			view->shape = nullptr;

		// If format is not requested
		if ((flags & PyBUF_FORMAT) != PyBUF_FORMAT)
			view->format = nullptr;

		// Okay all successful so reference the parent object
		view->obj = (PyObject*) self;
		Py_INCREF(view->obj);
		return 0;
	} catch (...) // GCOVR_EXCL_LINE
	{
		// GCOVR_EXCL_START
		// Release the partial buffer so we don't leak
		PyJPArray_releaseBuffer(self, view);

		// We are only allowed to raise BufferError
		PyErr_SetString(PyExc_BufferError, "Java array view failed");
		return -1;
		// GCOVR_EXCL_STOP
	}
	JP_PY_CATCH(-1); // GCOVR_EXCL_LINE
}

int PyJPArrayPrimitive_getBuffer(PyJPArray *self, Py_buffer *view, int flags)
{
	JP_PY_TRY("PyJPArrayPrimitive_getBuffer");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (self->m_Array == nullptr)
		JP_RAISE(PyExc_ValueError, "Null array");
	try
	{
		if ((flags & PyBUF_WRITEABLE) == PyBUF_WRITEABLE)
		{
			PyErr_SetString(PyExc_BufferError, "Java array buffer is not writable");
			return -1;
		}

		if (self->m_View == nullptr)
		{
			self->m_View = new JPArrayView(self->m_Array);
		}
		self->m_View->reference();
		*view = self->m_View->m_Buffer;

		// We are always contiguous so no need to check that here.
		view->readonly = 1;

		// If strides are not requested and this is a slice then fail
		if ((flags & PyBUF_STRIDES) != PyBUF_STRIDES)
		{
			if (view->strides[0] != view->itemsize)
				JP_RAISE(PyExc_BufferError, "slices required strides");
			view->strides = nullptr;
		}

		// If shape is not requested
		if ((flags & PyBUF_ND) != PyBUF_ND)
		{
			view->shape = nullptr;
		}

		// If format is not requested
		if ((flags & PyBUF_FORMAT) != PyBUF_FORMAT)
			view->format = nullptr;

		// Okay all successful so reference the parent object
		view->obj = (PyObject*) self;
		Py_INCREF(view->obj);
		return 0;
	} catch (...)
	{
		PyJPArray_releaseBuffer(self, view);

		// We are only allowed to raise BufferError
		PyErr_SetString(PyExc_BufferError, "Java array view failed");
		return -1;
	}
	JP_PY_CATCH(-1);
}

static PyObject *PyJPArray_pullTo(PyJPArray *self, PyObject *dest)
{
	JP_PY_TRY("PyJPArray_pullTo");
	if (self->m_Array == nullptr)
		JP_RAISE(PyExc_ValueError, "Null array");
	self->m_Array->pullTo(dest);
	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static const char *pullTo_doc =
		"Bulk-copy this array's elements out into a writable buffer.\n"
		"\n"
		"``dest`` must be a writable buffer-protocol object (e.g. a\n"
		"preallocated numpy array) with the same total element count and\n"
		"item size as this array -- its shape need not match. Only valid\n"
		"for arrays of primitives.\n";

static PyObject *PyJPArray_pushFrom(PyJPArray *self, PyObject *src)
{
	JP_PY_TRY("PyJPArray_pushFrom");
	if (self->m_Array == nullptr)
		JP_RAISE(PyExc_ValueError, "Null array");
	self->m_Array->pushFrom(src);
	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static const char *pushFrom_doc =
		"Bulk-copy a readable buffer's elements into this array in place.\n"
		"\n"
		"``src`` must be a readable buffer-protocol object (e.g. a numpy\n"
		"array) with the same total element count as this array -- its\n"
		"shape need not match, and its dtype need not match this array's\n"
		"component type (a converting fallback handles that case). Only\n"
		"valid for arrays of primitives.\n"
		"\n"
		"For a multi-dimensional array: any buffer export of this array\n"
		"(``memoryview(arr)``, ``numpy.asarray(arr)``) already open at the\n"
		"time of the call is a frozen read-only snapshot and will not\n"
		"reflect this call's changes -- Java's array-of-arrays layout isn't\n"
		"contiguous, so an export can only ever be a one-time collected\n"
		"copy, not a live view. Release any prior export first (or take a\n"
		"fresh one afterwards) to see the update.\n";

namespace
{

// dtype=None (or omitted): dstType == nullptr means "this array's own
// component type, plain output" -- resolved per-array in JPArray::toList.
// dtype=int/float: plain output cast to Java long/double.
// dtype=JByte..JDouble: wrapped output (tagged instance) cast to that type.
struct DtypeSpec
{
	JPPrimitiveType* type = nullptr;
	bool wrap = false;
};

DtypeSpec parseDtypeArg(PyObject* dtype_obj, JPContext* context)
{
	if (dtype_obj == nullptr || dtype_obj == Py_None)
		return DtypeSpec{};

	if (dtype_obj == (PyObject*) &PyLong_Type)
		return DtypeSpec{context->_long, false};
	if (dtype_obj == (PyObject*) &PyFloat_Type)
		return DtypeSpec{context->_double, false};

	if (PyType_Check(dtype_obj))
	{
		JPClass* jc = PyJPClass_getJPClass(dtype_obj);
		auto* prim = dynamic_cast<JPPrimitiveType*>(jc);
		if (prim != nullptr && jc != context->_boolean && jc != context->_char)
			return DtypeSpec{prim, true};
	}

	JP_RAISE(PyExc_TypeError, "dtype must be int, float, or a jpype primitive "
			"numeric type (JByte, JShort, JInt, JLong, JFloat, JDouble)");
}

} // namespace

static PyObject *PyJPArray_toList(PyJPArray *self, PyObject *args, PyObject *kwargs)
{
	static const char *kwlist[] = {"dtype", nullptr};
	PyObject *dtype_obj = nullptr;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:toList", (char**) kwlist, &dtype_obj))
		return nullptr;

	JP_PY_TRY("PyJPArray_toList");
	if (self->m_Array == nullptr)
		JP_RAISE(PyExc_ValueError, "Null array");
	JPJavaFrame frame = JPJavaFrame::outer();
	DtypeSpec spec = parseDtypeArg(dtype_obj, frame.getContext());
	return self->m_Array->toList(spec.type, spec.wrap).keep();
	JP_PY_CATCH(nullptr);
}

static const char *toList_doc =
		"Convert this array into a genuine Python list.\n"
		"\n"
		"For an array of primitives this is a bulk conversion (one JNI\n"
		"critical section for the whole array rather than one JNI call per\n"
		"element via ``list(arr)``); multi-dimensional primitive arrays\n"
		"produce genuinely nested lists. For an array of objects, elements\n"
		"are boxed individually, same as ``list(arr)``.\n"
		"\n"
		"By default, primitive arrays return plain Python types (int,\n"
		"float, bool, str). ``dtype`` requests a forced cast (NumPy-style):\n"
		"\n"
		"    - ``int``/``float``: plain Python type, cast to that kind.\n"
		"    - ``JByte``/``JShort``/``JInt``/``JLong``/``JFloat``/``JDouble``:\n"
		"      a tagged wrapper instance of that type, cast to it.\n"
		"\n"
		"``dtype`` is not supported for ``JBoolean``/``JChar`` arrays.\n";

static const char *length_doc =
		"Get the length of a Java array\n"
		"\n"
		"This method is provided for compatibility with Java syntax.\n"
		"Generally, the Python style ``len(array)`` should be preferred.\n";

static PyMethodDef arrayMethods[] = {
	{"__getitem__", (PyCFunction) (&PyJPArray_getItem), METH_O | METH_COEXIST, ""},
	{"pullTo", (PyCFunction) (&PyJPArray_pullTo), METH_O, (pullTo_doc)},
	{"pushFrom", (PyCFunction) (&PyJPArray_pushFrom), METH_O, (pushFrom_doc)},
	{"toList", (PyCFunction) (&PyJPArray_toList), METH_VARARGS | METH_KEYWORDS, (toList_doc)},
	{nullptr},
};

static PyGetSetDef arrayGetSets[] = {
	{"length", (getter) (&PyJPArray_length), nullptr, (length_doc)},
	{nullptr}
};

static PyType_Slot arraySlots[] = {
	{ Py_tp_new,	  (void*) PyJPArray_new},
	{ Py_tp_init,	 (void*) PyJPArray_init},
	{ Py_tp_dealloc,  (void*) PyJPArray_dealloc},
	{ Py_tp_repr,	 (void*) PyJPArray_repr},
	{ Py_tp_methods,  (void*) &arrayMethods},
	{ Py_mp_subscript, (void*) &PyJPArray_getItem},
	{ Py_sq_length,   (void*) &PyJPArray_len},
	{ Py_sq_item,	 (void*) &PyJPArray_sqItem},
	{ Py_tp_iter,	 (void*) &PyJPArray_iter},
	{ Py_tp_getset,   (void*) &arrayGetSets},
	{ Py_mp_ass_subscript, (void*) &PyJPArray_assignSubscript},
#if PY_VERSION_HEX >= 0x03090000
	{ Py_bf_getbuffer, (void*) &PyJPArray_getBuffer},
	{ Py_bf_releasebuffer, (void*) &PyJPArray_releaseBuffer},
#endif
	{0}
};

#if PY_VERSION_HEX < 0x03090000
static PyBufferProcs arrayBuffer = {
	(getbufferproc) & PyJPArray_getBuffer,
	(releasebufferproc) & PyJPArray_releaseBuffer
};
#endif

PyTypeObject *PyJPArray_Type = nullptr;
static PyType_Spec arraySpec = {
	"_jpype._JArray",
	sizeof (PyJPArray),
	0,
	Py_TPFLAGS_DEFAULT  | Py_TPFLAGS_BASETYPE,
	arraySlots
};

#if PY_VERSION_HEX < 0x03090000
static PyBufferProcs arrayPrimBuffer = {
	(getbufferproc) & PyJPArrayPrimitive_getBuffer,
	(releasebufferproc) & PyJPArray_releaseBuffer
};
#endif

static PyType_Slot arrayPrimSlots[] = {
#if PY_VERSION_HEX >= 0x03090000
	{ Py_bf_getbuffer, (void*) &PyJPArrayPrimitive_getBuffer},
	{ Py_bf_releasebuffer, (void*) &PyJPArray_releaseBuffer},
#endif
	{0}
};

PyTypeObject *PyJPArrayPrimitive_Type = nullptr;
static PyType_Spec arrayPrimSpec = {
	"_jpype._JArrayPrimitive",
	0,
	0,
	Py_TPFLAGS_DEFAULT  | Py_TPFLAGS_BASETYPE,
	arrayPrimSlots
};

#ifdef __cplusplus
}
#endif

void PyJPArray_initType(PyObject * module)
{
	// Array has a real, compile-time-known C layout (struct PyJPArray) and is
	// always single-inheritance below Object, so it goes straight to
	// concrete -- same reasoning as Exception, see pyjp_object.cpp.
	Py_ssize_t offset = offsetof (struct PyJPArray, extra);

	JPPyObject tuple = JPPyTuple_Pack(PyJPObject_Type);
	PyJPArray_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&arraySpec, tuple.get(), offset);
	JP_PY_CHECK();
#if PY_VERSION_HEX < 0x03090000
	PyJPArray_Type->tp_as_buffer = &arrayBuffer;
#endif
	PyModule_AddObject(module, "_JArray", (PyObject*) PyJPArray_Type);
	JP_PY_CHECK();

	// ArrayPrimitive adds no new fields (arrayPrimSpec.basicsize == 0, so it
	// inherits PyJPArray's basicsize as-is) -- same slot location, so pass
	// the identical offset rather than 0 (which would mean legacy).
	tuple = JPPyTuple_Pack(PyJPArray_Type);
	PyJPArrayPrimitive_Type = (PyTypeObject*)
			PyJPClass_FromSpecWithBases(&arrayPrimSpec, tuple.get(), offset);
#if PY_VERSION_HEX < 0x03090000
	PyJPArrayPrimitive_Type->tp_as_buffer = &arrayPrimBuffer;
#endif
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JArrayPrimitive",
			(PyObject*) PyJPArrayPrimitive_Type);
	JP_PY_CHECK();

	// Internal only -- not added to the module namespace, mirrors how
	// CPython doesn't expose list_iterator/tuple_iterator as builtins
	// either. A plain heap type (no Java-wrapping machinery needed).
	PyJPArrayIter_Type = (PyTypeObject*) PyType_FromSpec(&arrayIterSpec);
	JP_PY_CHECK();
}

JPPyObject PyJPArray_create(JPJavaFrame &frame, PyTypeObject *type, const JPValue & value)
{
	PyObject *obj = type->tp_alloc(type, 0);
	JP_PY_CHECK();
	((PyJPArray*) obj)->m_Array = JPArray::create(value);
	PyJPValue_assignJavaSlot(frame, obj, value);
	return JPPyObject::claim(obj);
}
