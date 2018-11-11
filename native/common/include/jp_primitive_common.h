
/*****************************************************************************
   Copyright 2004-2008 Steve Ménard
   
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
 *****************************************************************************/
#ifndef JP_PRIMITIVE_COMMON_H_
#define JP_PRIMITIVE_COMMON_H_

#include <Python.h>
#include <jpype.h>

typedef unsigned int uint;

#ifdef HAVE_NUMPY
#define PY_ARRAY_UNIQUE_SYMBOL jpype_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>
#else
#define NPY_BOOL 0
#define NPY_BYTE 0
#define NPY_SHORT 0
#define NPY_INT 0
#define NPY_INT64 0
#define NPY_FLOAT32 0
#define NPY_FLOAT64 0
#endif

template <typename array_t,  typename ptr_t>
class JPPrimitiveArrayAccessor
{
	typedef void (JPJavaFrame::*releaseFnc)(array_t, ptr_t, jint);
	typedef ptr_t (JPJavaFrame::*accessFnc)(array_t, jboolean*);

	JPJavaFrame& _frame;
	array_t _array;
	ptr_t _elem;
	releaseFnc _release;
	jboolean _iscopy;
	jint _commit;

public:

	JPPrimitiveArrayAccessor(JPJavaFrame& frame, jarray array, accessFnc access, releaseFnc release)
	: _frame(frame), _array((array_t) array), _release(release)
	{
		_commit = JNI_ABORT;
		_elem = ((&_frame)->*access)(_array, &_iscopy);
	}

	~JPPrimitiveArrayAccessor()
	{
		((&_frame)->*_release)(_array, _elem, _commit);
	}

	ptr_t get()
	{
		return _elem;
	}

	void commit()
	{
		_commit = 0;
	}
} ;

#if (PY_VERSION_HEX >= 0x02070000)
// for python 2.6 we have also memory view available, but it does not contain the needed functions.
#include <jpype_memory_view.h>

/** Attempt to copy an entire range using direct buffer access.
 * 
 * @param frame is the java frame to hold references in.
 * @param array is the array to set
 * @param start is the first element to modify
 * @param length is the total length to set (from the sequence)
 * @param sequence is the python array with memory view access.
 * @param setter is a Java Set*ArrayRegion
 * 
 * @return true if the copy was successful, false if we need to fall back
 *   to elementwise copy.
 */
template <typename jarraytype, typename jelementtype, typename setFnc>
inline bool setRangeViaBuffer(JPJavaFrame& frame,
		jarray array, int start, uint length,
		PyObject* sequence, int npy_type, setFnc setter)
{
#ifdef HAVE_NUMPY
	JPPyObject ref;
	// Arrays need to be checked
	// FIXME This will still allow mismatched byte order and 
	// irregular array types pass.  
	if (PyArray_Check(sequence) && PyArray_TYPE(sequence) != npy_type)
	{
		// If we can't cast then fall back to standard methods
		if (!PyArray_CanCastSafely(PyArray_TYPE(sequence), npy_type))
			return false;

		// Otherwise, create a casted array
		sequence = PyArray_Cast((PyArrayObject*) sequence, npy_type);
		ref = JPPyObject(JPPyRef::_call, sequence);
	}
#else
	// If we compile without numpy and then encounter numpy
	// types we will incorrectly convert here.
	return false;
#endif

	JPPyMemoryViewAccessor accessor(sequence);
	if (!accessor.valid())
		return false;

	// ensure length of buffer contains enough elements somehow.
	if ((accessor.size() / sizeof (jelementtype)) != length)
	{
		// Nope, so fall back on per element conversion.
		return false;
	}

	jarraytype a = (jarraytype) array;
	jelementtype* buffer = (jelementtype*) accessor.get();

	(frame.*setter)(a, start, length, buffer);
	return true;
}
#else
// Used only by Python 2.6
template <typename a, typename b, typename c>
bool setRangeViaBuffer(JPJavaFrame& frame, jarray, int, uint, PyObject*, int, c)
{
	return false;
}
#endif

/**
 * gets either a numpy ndarray or a python list with a copy of the underling java array,
 * containing the range [lo, hi].
 *
 * Parameters:
 * -----------
 * lo = low index
 * hi = high index
 * npy_type = e.g NPY_FLOAT64
 * jtype = eg. jdouble
 * convert = function to convert elements to python types. Eg: PyInt_FromLong
 */
template<typename jtype, typename py_wrapper_func>
inline JPPyObject getSlice(JPJavaFrame& frame, jarray array, int lo, int hi, int npy_type,
		py_wrapper_func convert)
{
	JPPrimitiveArrayAccessor<jarray, void*> accessor(frame, array,
			&JPJavaFrame::GetPrimitiveArrayCritical, &JPJavaFrame::ReleasePrimitiveArrayCritical);

	uint len = hi - lo;
#ifdef HAVE_NUMPY
	npy_intp dims[] = {len};
	PyObject* res = PyArray_SimpleNew(1, dims, npy_type);
	if (len > 0)
	{
		jtype* val = (jtype*) accessor.get();
		// use typed numpy arrays for results
		memcpy(((PyArrayObject*) res)->data, &val[lo], len * sizeof (jtype));
	}
	return JPPyObject(JPPyRef::_claim, res);
#else
	JPPyList res(JPPyList::newList(len));
	if (len > 0)
	{
		jtype* val = (jtype*) accessor.get();
		// use python lists for results
		for (Py_ssize_t i = lo; i < hi; i++)
			PyList_SET_ITEM(res.get(), i - lo, convert(val[i]));
	}
	return res;
#endif
}

#endif // JP_PRIMITIVE_COMMON_H_
