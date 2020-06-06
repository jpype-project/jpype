#include "jpype.h"
#include "pyjp.h"

/****************************************************************************
 * Base object
 ***************************************************************************/

static void assertValid(PyObject *obj)
{
	if (obj->ob_refcnt >= 1)
		return;

	// GCOVR_EXCL_START
	// At this point our car has traveled beyond the end of the
	// cliff and it will hit the ground some twenty
	// python calls later with a nearly untracable fault, thus
	// rather than waiting for the inevitable, we chose to take
	// a noble death here.
	JP_TRACE_PY("pyref FAULT", obj);
	JP_RAISE(PyExc_SystemError, "Deleted reference");
	// GCOVR_EXCL_STOP
}

/**
 * This policy is used if we need to hold a reference to an existing
 * object for some duration.  The object may be null.
 *
 * Increment reference count if not null, and decrement when done.
 */
JPPyObject JPPyObject::use(PyObject* obj)
{
	JP_TRACE_PY("pyref use(inc)", obj);
	if (obj != NULL)
	{
		assertValid(obj);
		Py_INCREF(obj);
	}
	return JPPyObject(obj, 0);
}

/**
 * This policy is used when we are given a new reference that we must
 * destroy.  This will steal a reference.
 *
 * claim reference, and decremented when done. Clears errors if NULL.
 */
JPPyObject JPPyObject::accept(PyObject* obj)
{
	JP_TRACE_PY("pyref new(accept)", obj);
	if (obj == NULL)
		PyErr_Clear();

	return JPPyObject(obj, 1);
}

/**
 * This policy is used when we are given a new reference that we must
 * destroy.  This will steal a reference.
 *
 * Assert not null, claim reference, and decremented when done.
 * Will throw an exception if the object is null.
 */
JPPyObject JPPyObject::claim(PyObject* obj)
{
	JP_TRACE_PY("pyref new(claim)", obj);
	ASSERT_NOT_NULL(obj);
	assertValid(obj);
	return JPPyObject(obj, 2);
}

/**
 * This policy is used when we are capturing an object returned from a python
 * call that we are responsible for.  This will steal a reference.
 *
 * Check for errors, assert not null, then claim.
 * Will throw an exception an error occurs.
 */
JPPyObject JPPyObject::call(PyObject* obj)
{
	JP_TRACE_PY("pyref new(call)", obj);
	JP_PY_CHECK();
	ASSERT_NOT_NULL(obj);
	assertValid(obj);
	return JPPyObject(obj, 3);
}

JPPyObject::JPPyObject(PyObject* obj, int i)
: m_PyObject(obj)
{
}

JPPyObject::JPPyObject(const JPPyObject &self)
: m_PyObject(self.m_PyObject)
{
	if (m_PyObject != NULL)
	{
		incref();
		JP_TRACE_PY("pyref copy ctor(inc)", m_PyObject);
	}
}

JPPyObject::~JPPyObject()
{
	if (m_PyObject != NULL)
	{
		JP_TRACE_PY("pyref dtor(dec)", m_PyObject);
		decref();
	} else
	{
		JP_TRACE_PY("pyref dtor(null)", m_PyObject);
	}
}

JPPyObject& JPPyObject::operator=(const JPPyObject& self)
{
	if (m_PyObject == self.m_PyObject)
		return *this;
	if (m_PyObject != NULL)
	{
		JP_TRACE_PY("pyref op=(dec)", m_PyObject);
		decref();
	}
	m_PyObject = self.m_PyObject;
	if (m_PyObject != NULL)
	{
		incref();
		JP_TRACE_PY("pyref op=(inc)", m_PyObject);
	}
	return *this;
}

PyObject* JPPyObject::keep()
{
	// This can only happen if we have a fatal error in our reference
	// management system.  It should never be triggered by the user.
	if (m_PyObject == NULL)
	{
		JP_RAISE(PyExc_SystemError, "Attempt to keep null reference"); // GCOVR_EXCL_LINE
	}
	JP_TRACE_PY("pyref keep ", m_PyObject);
	PyObject *out = m_PyObject;
	m_PyObject = NULL;
	return out;
}

void JPPyObject::incref()
{
	assertValid(m_PyObject);
	Py_INCREF(m_PyObject);
}

void JPPyObject::decref()
{
	assertValid(m_PyObject);
	Py_DECREF(m_PyObject);
	m_PyObject = 0;
}

JPPyObject JPPyObject::getNone()
{
	return JPPyObject::use(Py_None);
}

/****************************************************************************
 * String
 ***************************************************************************/

JPPyObject JPPyString::fromCharUTF16(jchar c)
{
#if defined(PYPY_VERSION)
	wchar_t buf[1];
	buf[0] = c;
	return JPPyObject::call(PyUnicode_FromWideChar(buf, 1));
#else
	if (c < 128)
	{
		char c1 = (char) c;
		return JPPyObject::call(PyUnicode_FromStringAndSize(&c1, 1));
	}
	JPPyObject buf = JPPyObject::call(PyUnicode_New(1, 65535));
	Py_UCS4 c2 = c;
	PyUnicode_WriteChar(buf.get(), 0, c2);
	JP_PY_CHECK();
	PyUnicode_READY(buf.get());
	return buf;
#endif
}

bool JPPyString::checkCharUTF16(PyObject* pyobj)
{
	JP_TRACE_IN("JPPyString::checkCharUTF16");
	if (PyIndex_Check(pyobj))
		return true;
	if (PyUnicode_Check(pyobj) && PyUnicode_GetLength(pyobj) == 1)
		return true;
	if (PyBytes_Check(pyobj) && PyBytes_Size(pyobj) == 1)
		return true;
	return false;
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}

jchar JPPyString::asCharUTF16(PyObject* pyobj)
{
	if (PyIndex_Check(pyobj))
	{
		jlong val = PyLong_AsLongLong(pyobj);
		if (val < 0 || val > 65535)
			JP_RAISE(PyExc_OverflowError, "Unable to convert int into char range");
		return (jchar) val;
	}

#if defined(PYPY_VERSION)
	if (PyBytes_Check(pyobj))
	{
		int sz = PyBytes_Size(pyobj);
		if (sz != 1)
			JP_RAISE(PyExc_ValueError, "Char must be length 1");

		jchar c = PyBytes_AsString(pyobj)[0];
		if (PyErr_Occurred())
			JP_RAISE_PYTHON();
		return c;
	}
	if (PyUnicode_Check(pyobj))
	{
		if (PyUnicode_GetLength(pyobj) > 1)
			JP_RAISE(PyExc_ValueError, "Char must be length 1");

		PyUnicode_READY(pyobj);
		Py_UCS4 value = PyUnicode_READ_CHAR(pyobj, 0);
		if (value > 0xffff)
		{
			JP_RAISE(PyExc_ValueError, "Unable to pack 4 byte unicode into java char");
		}
		return value;
	}
#else
	if (PyBytes_Check(pyobj))
	{
		Py_ssize_t sz = PyBytes_Size(pyobj);
		if (sz != 1)
			JP_RAISE(PyExc_ValueError, "Char must be length 1");

		jchar c = PyBytes_AsString(pyobj)[0];
		JP_PY_CHECK();
		return c;
	}
	if (PyUnicode_Check(pyobj))
	{
		if (PyUnicode_GetLength(pyobj) > 1)
			JP_RAISE(PyExc_ValueError, "Char must be length 1");

		PyUnicode_READY(pyobj);
		Py_UCS4 value = PyUnicode_ReadChar(pyobj, 0);
		if (value > 0xffff)
		{
			JP_RAISE(PyExc_ValueError, "Unable to pack 4 byte unicode into java char");
		}
		return value;
	}
#endif
	PyErr_Format(PyExc_TypeError, "Unable to convert '%s'  to char", Py_TYPE(pyobj)->tp_name);
	JP_RAISE_PYTHON();
}

/** Check if the object is a bytes or unicode.
 *
 * @returns true if the object is bytes or unicode.
 */
bool JPPyString::check(PyObject* obj)
{
	return PyUnicode_Check(obj) || PyBytes_Check(obj);
}

/** Create a new string from utf8 encoded string.
 * Note: java utf8 is not utf8.
 */
JPPyObject JPPyString::fromStringUTF8(const string& str)
{
	size_t len = str.size();

	// Python 3 is always unicode
	JPPyObject bytes = JPPyObject::call(PyBytes_FromStringAndSize(str.c_str(), len));
	return JPPyObject::call(PyUnicode_FromEncodedObject(bytes.get(), "UTF-8", "strict"));
}

string JPPyString::asStringUTF8(PyObject* pyobj)
{
	JP_TRACE_IN("JPPyUnicode::asStringUTF8");
	ASSERT_NOT_NULL(pyobj);

	if (PyUnicode_Check(pyobj))
	{
		Py_ssize_t size = 0;
		char *buffer = NULL;
		JPPyObject val = JPPyObject::call(PyUnicode_AsEncodedString(pyobj, "UTF-8", "strict"));
		PyBytes_AsStringAndSize(val.get(), &buffer, &size);
		JP_PY_CHECK();
		if (buffer != NULL)
			return string(buffer, size);
		else
			return string();
	} else if (PyBytes_Check(pyobj))
	{
		Py_ssize_t size = 0;
		char *buffer = NULL;
		PyBytes_AsStringAndSize(pyobj, &buffer, &size);
		JP_PY_CHECK();
		return string(buffer, size);
	}
	// GCOVR_EXCL_START
	JP_RAISE(PyExc_RuntimeError, "Failed to convert to string.");
	return string();
	JP_TRACE_OUT;
	// GCOVR_EXCL_STOP
}

/****************************************************************************
 * Container types
 ***************************************************************************/

jlong JPPySequence::size()
{
	return PySequence_Size(m_Sequence.get());
}

JPPyObject JPPySequence::operator[](jlong i)
{
	return JPPyObject::call(PySequence_GetItem(m_Sequence.get(), i)); // new reference
}

JPPyObjectVector::JPPyObjectVector(PyObject* sequence)
{
	m_Sequence = JPPyObject::use(sequence);
	size_t n = PySequence_Size(m_Sequence.get());
	m_Contents.resize(n);
	for (size_t i = 0; i < n; ++i)
	{
		m_Contents[i] = JPPyObject::call(PySequence_GetItem(m_Sequence.get(), i));
	}
}

JPPyObjectVector::JPPyObjectVector(PyObject* inst, PyObject* sequence)
{
	m_Instance = JPPyObject::use(inst);
	m_Sequence = JPPyObject::use(sequence);
	size_t n = 0;
	if (sequence != NULL)
		n = PySequence_Size(m_Sequence.get());
	m_Contents.resize(n + 1);
	for (size_t i = 0; i < n; ++i)
	{
		m_Contents[i + 1] = JPPyObject::call(PySequence_GetItem(m_Sequence.get(), i));
	}
	m_Contents[0] = m_Instance;
}

bool JPPyErr::fetch(JPPyObject& exceptionClass, JPPyObject& exceptionValue, JPPyObject& exceptionTrace)
{
	PyObject *v1, *v2, *v3;
	PyErr_Fetch(&v1, &v2, &v3);
	if (v1 == NULL && v2 == NULL && v3 == NULL)
		return false;
	exceptionClass = JPPyObject::accept(v1);
	exceptionValue = JPPyObject::accept(v2);
	exceptionTrace = JPPyObject::accept(v3);
	return true;
}

void JPPyErr::restore(JPPyObject& exceptionClass, JPPyObject& exceptionValue, JPPyObject& exceptionTrace)
{
	PyErr_Restore(exceptionClass.keepNull(), exceptionValue.keepNull(), exceptionTrace.keepNull());
}

JPPyCallAcquire::JPPyCallAcquire()
{
	PyGILState_STATE* save = new PyGILState_STATE;
	*save = PyGILState_Ensure();
	m_State = (void*) save;
}

JPPyCallAcquire::~JPPyCallAcquire()
{
	PyGILState_STATE* save = (PyGILState_STATE*) m_State;
	PyGILState_Release(*save);
	delete save;
}

// This is used when leaving python from to perform some

JPPyCallRelease::JPPyCallRelease()
{
	// Release the lock and set the thread state to NULL
	m_State1 = (void*) PyEval_SaveThread();
}

JPPyCallRelease::~JPPyCallRelease()
{
	// Reaquire the lock
	PyThreadState *save = (PyThreadState *) m_State1;
	PyEval_RestoreThread(save);
}

JPPyBuffer::JPPyBuffer(PyObject* obj, int flags)
{
	int ret = PyObject_GetBuffer(obj, &m_View, flags);
	m_Valid = (ret != -1);
}

JPPyBuffer::~JPPyBuffer()
{
	if (m_Valid)
		PyBuffer_Release(&m_View);
}

char *JPPyBuffer::getBufferPtr(std::vector<Py_ssize_t>& indices)
{
	char *pointer = (char*) m_View.buf;
	// No shape is just a 1D array
	if (m_View.shape == NULL)
	{
		return pointer;
	}

	// No strides is C contiguous ND array
	if (m_View.strides == NULL)
	{
		Py_ssize_t index = 0;
		for (int i = 0; i < m_View.ndim; i++)
		{
			index = index * m_View.shape[i] + indices[i];
		}
		index *= m_View.itemsize;
		return pointer + index;
	}

	// Otherwise we can be a full array
	for (int i = 0; i < m_View.ndim; i++)
	{
		pointer += m_View.strides[i] * indices[i];
		if (m_View.suboffsets != NULL && m_View.suboffsets[i] >= 0 )
		{
			pointer = *((char**) pointer) + m_View.suboffsets[i];
		}
	}
	return pointer;
}

JPPyErrFrame::JPPyErrFrame()
{
	good = JPPyErr::fetch(m_ExceptionClass, m_ExceptionValue, m_ExceptionTrace);
}

JPPyErrFrame::~JPPyErrFrame()
{
	try
	{
		if (good)
			JPPyErr::restore(m_ExceptionClass, m_ExceptionValue, m_ExceptionTrace);
	}	catch (...)  // GCOVR_EXCL_LINE
	{
		// No throw is allowed in dtor.
	}
}

void JPPyErrFrame::clear()
{
	good = false;
}

void JPPyErrFrame::normalize()
{
	// Python uses lazy evaluation on exceptions thus we can't modify it until
	// we have forced it to realize the exception.
	if (!PyExceptionInstance_Check(m_ExceptionValue.get()))
	{
		JPPyObject args = JPPyObject::call(PyTuple_Pack(1, m_ExceptionValue.get()));
		m_ExceptionValue = JPPyObject::call(PyObject_Call(m_ExceptionClass.get(), args.get(), NULL));
		PyException_SetTraceback(m_ExceptionValue.get(), m_ExceptionTrace.get());
		JPPyErr::restore(m_ExceptionClass, m_ExceptionValue, m_ExceptionTrace);
		JPPyErr::fetch(m_ExceptionClass, m_ExceptionValue, m_ExceptionTrace);
	}
}
