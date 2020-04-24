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

JPPyObject::JPPyObject(JPPyRef::Type usage, PyObject* obj)
: pyobj(NULL)
{
	// See if we are to check the result.
	if ((usage & 1) == 1)
		JP_PY_CHECK();

	if ((usage & 2) == 2)
	{
		if ((usage & 4) == 4)
		{
			ASSERT_NOT_NULL(obj);
			assertValid(obj);
		} else if (obj == NULL)
			PyErr_Clear();

		// Claim it by stealing a references
		pyobj = obj;
		JP_TRACE_PY("pyref new(claim)", pyobj);
	} else
	{
		pyobj = obj;
		if (pyobj == NULL)
		{
			JP_TRACE_PY("pyref new(null)", pyobj);
			return;
		}

		assertValid(obj);
		incref();
		JP_TRACE_PY("pyref new(inc)", pyobj);
	}
}

JPPyObject::JPPyObject(const JPPyObject &self)
: pyobj(self.pyobj)
{
	if (pyobj != NULL)
	{
		incref();
		JP_TRACE_PY("pyref copy ctor(inc)", pyobj);
	}
}

JPPyObject::~JPPyObject()
{
	if (pyobj != NULL)
	{
		JP_TRACE_PY("pyref dtor(dec)", pyobj);
		decref();
	} else
	{
		JP_TRACE_PY("pyref dtor(null)", pyobj);
	}
}

JPPyObject& JPPyObject::operator=(const JPPyObject& self)
{
	if (pyobj == self.pyobj)
		return *this;
	if (pyobj != NULL)
	{
		JP_TRACE_PY("pyref op=(dec)", pyobj);
		decref();
	}
	pyobj = self.pyobj;
	if (pyobj != NULL)
	{
		incref();
		JP_TRACE_PY("pyref op=(inc)", pyobj);
	}
	return *this;
}

PyObject* JPPyObject::keep()
{
	// This can only happen if we have a fatal error in our reference
	// management system.  It should never be triggered by the user.
	if (pyobj == NULL)
	{
		JP_RAISE(PyExc_SystemError, "Attempt to keep null reference");
	}
	JP_TRACE_PY("pyref keep ", pyobj);
	PyObject *out = pyobj;
	pyobj = NULL;
	return out;
}

void JPPyObject::incref()
{
	assertValid(pyobj);
	Py_INCREF(pyobj);
}

void JPPyObject::decref()
{
	assertValid(pyobj);
	Py_DECREF(pyobj);
	pyobj = 0;
}

bool JPPyObject::isNone(PyObject* pyobj)
{
	return pyobj == Py_None;
}

bool JPPyObject::isSequenceOfItems(PyObject* obj)
{
	return PySequence_Check(obj) && !JPPyString::check(obj);
}

const char* JPPyObject::getTypeName(PyObject* obj)
{
	if (obj == NULL)
		return "null";
	return Py_TYPE(obj)->tp_name;
}

JPPyObject JPPyObject::getNone()
{
	return JPPyObject(JPPyRef::_use, Py_None);
}

/****************************************************************************
 * Number types
 ***************************************************************************/

jlong JPPyLong::asLong(PyObject* obj)
{
	jlong res = PyLong_AsLongLong(obj);
	JP_PY_CHECK();
	return res;
}

/****************************************************************************
 * String
 ***************************************************************************/

JPPyObject JPPyString::fromCharUTF16(jchar c)
{
#if defined(PYPY_VERSION)
	wchar_t buf[1];
	buf[0] = c;
	return JPPyObject(JPPyRef::_call, PyUnicode_FromWideChar(buf, 1));
#else
	if (c < 128)
	{
		char c1 = (char) c;
		return JPPyObject(JPPyRef::_call, PyUnicode_FromStringAndSize(&c1, 1));
	}
	JPPyObject buf(JPPyRef::_call, PyUnicode_New(1, 65535));
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
	JP_TRACE_OUT;
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
	JPPyObject bytes(JPPyRef::_call, PyBytes_FromStringAndSize(str.c_str(), len));
	return JPPyObject(JPPyRef::_call, PyUnicode_FromEncodedObject(bytes.get(), "UTF-8", "strict"));
}

string JPPyString::asStringUTF8(PyObject* pyobj)
{
	JP_TRACE_IN("JPPyUnicode::asStringUTF8");
	ASSERT_NOT_NULL(pyobj);

	if (PyUnicode_Check(pyobj))
	{
		Py_ssize_t size = 0;
		char *buffer = NULL;
		JPPyObject val(JPPyRef::_call, PyUnicode_AsEncodedString(pyobj, "UTF-8", "strict"));
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
	JP_RAISE(PyExc_RuntimeError, "Failed to convert to string.");
	return string();
	JP_TRACE_OUT;
}

/****************************************************************************
 * Container types
 ***************************************************************************/

JPPyTuple JPPyTuple::newTuple(jlong sz)
{
	return JPPyTuple(JPPyRef::_call, PyTuple_New((Py_ssize_t) sz));
}

void JPPyTuple::setItem(jlong ndx, PyObject* val)
{
	ASSERT_NOT_NULL(val);
	Py_INCREF(val);
	PyTuple_SetItem(pyobj, (Py_ssize_t) ndx, val); // steals reference
	JP_PY_CHECK();
}

jlong JPPySequence::size()
{
	if (pyobj == NULL)
		return 0;
	return PySequence_Size(pyobj);
}

JPPyObject JPPySequence::operator[](jlong i)
{
	return JPPyObject(JPPyRef::_call, PySequence_GetItem(pyobj, i)); // new reference
}

JPPyObjectVector::JPPyObjectVector(PyObject* sequence)
: seq(JPPyRef::_use, sequence)
{
	size_t n = seq.size();
	contents.resize(n);
	for (size_t i = 0; i < n; ++i)
	{
		contents[i] = seq[i];
	}
}

JPPyObjectVector::JPPyObjectVector(PyObject* inst, PyObject* sequence)
: instance(JPPyRef::_use, inst), seq(JPPyRef::_use, sequence)
{
	size_t n = seq.size();
	contents.resize(n + 1);
	for (size_t i = 0; i < n; ++i)
	{
		contents[i + 1] = seq[i];
	}
	contents[0] = instance;
}

bool JPPyErr::occurred()
{
	return PyErr_Occurred() != 0;
}

bool JPPyErr::fetch(JPPyObject& exceptionClass, JPPyObject& exceptionValue, JPPyObject& exceptionTrace)
{
	PyObject *v1, *v2, *v3;
	PyErr_Fetch(&v1, &v2, &v3);
	if (v1 == NULL && v2 == NULL && v3 == NULL)
		return false;
	exceptionClass = JPPyObject(JPPyRef::_accept, v1);
	exceptionValue = JPPyObject(JPPyRef::_accept, v2);
	exceptionTrace = JPPyObject(JPPyRef::_accept, v3);
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
	state1 = (void*) save;
}

JPPyCallAcquire::~JPPyCallAcquire()
{
	PyGILState_STATE* save = (PyGILState_STATE*) state1;
	PyGILState_Release(*save);
	delete save;
}

// This is used when leaving python from to perform some

JPPyCallRelease::JPPyCallRelease()
{
	// Release the lock and set the thread state to NULL
	state1 = (void*) PyEval_SaveThread();
}

JPPyCallRelease::~JPPyCallRelease()
{
	// Reaquire the lock
	PyThreadState *save = (PyThreadState *) state1;
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
	good = JPPyErr::fetch(exceptionClass, exceptionValue, exceptionTrace);
}

JPPyErrFrame::~JPPyErrFrame()
{
	try
	{
		if (good)
			JPPyErr::restore(exceptionClass, exceptionValue, exceptionTrace);
	}	catch (...)
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
	if (!PyExceptionInstance_Check(exceptionValue.get()))
	{
		JPPyTuple args = JPPyTuple::newTuple(1);
		args.setItem(0, exceptionValue.get());
		exceptionValue = JPPyObject(JPPyRef::_call, PyObject_Call(exceptionClass.get(), args.get(), NULL));
		PyException_SetTraceback(exceptionValue.get(), exceptionTrace.get());
		JPPyErr::restore(exceptionClass, exceptionValue, exceptionTrace);
		JPPyErr::fetch(exceptionClass, exceptionValue, exceptionTrace);
	}
}
