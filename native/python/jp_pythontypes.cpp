#include <Python.h>
#include <jpype.h>
#include <jpype_memory_view.h>

#include "pyjp_module.h"

/****************************************************************************
 * Base object
 ***************************************************************************/
JPPyObject::JPPyObject(JPPyRef::Type usage, PyObject* obj)
: pyobj(NULL)
{
	// See if we are to check the result.
	if ((usage & 1) == 1)
		JP_PY_CHECK();

	if ((usage & 2) == 2)
	{
		// Claim it by stealing a references
		ASSERT_NOT_NULL(obj);
		pyobj = obj;
		JP_TRACE_PY("pyref new(claim)", pyobj);
	}
	else
	{
		pyobj = obj;
		if (pyobj == NULL)
		{
			JP_TRACE_PY("pyref new(null)", pyobj);
			return;
		}

		JP_TRACE_PY("pyref new(inc)", pyobj);
		incref();
	}
}

JPPyObject::JPPyObject(const JPPyObject &self)
: pyobj(self.pyobj)
{
	if (pyobj != NULL)
	{
		JP_TRACE_PY("pyref copy ctor(inc)", pyobj);
		incref();
	}
}

JPPyObject::~JPPyObject()
{
	if (pyobj != NULL)
	{
		JP_TRACE_PY("pyref dtor(dec)", pyobj);
		decref();
	}
	else
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
		JP_TRACE_PY("pyref op=(inc)", pyobj);
		incref();
	}
	return *this;
}

PyObject* JPPyObject::keep()
{
	if (pyobj == NULL)
	{
		JP_RAISE_RUNTIME_ERROR("Attempt to keep null reference");
	}
	JP_TRACE_PY("pyref keep ", pyobj);
	PyObject *out = pyobj;
	pyobj = NULL;
	return out;
}

void JPPyObject::incref()
{
	Py_INCREF(pyobj);
}

void JPPyObject::decref()
{
	if (pyobj->ob_refcnt <= 0)
	{
		// At this point our car has traveled beyond the end of the 
		// cliff and it will hit the ground some twenty
		// python calls later with a nearly untracable fault, thus
		// rather than waiting for the inevitable, we chose to take 
		// a noble death here.
		JPTracer::trace("Python referencing fault");
		int *i = 0;
		*i = 0;
	}
	Py_DECREF(pyobj);
	pyobj = 0;
}

bool JPPyObject::isNone(PyObject* pyobj)
{
	return pyobj == Py_None;
}

bool JPPyObject::isSequenceOfItems(PyObject* obj)
{
	return JPPySequence::check(obj) && !JPPyString::check(obj);
}

string JPPyObject::str()
{
	JPPyObject s(JPPyRef::_call, PyObject_Str(pyobj));
	return JPPyString::asStringUTF8(s.get());
}

bool JPPyObject::hasAttrString(PyObject* pyobj, const char* k)
{
	int res = PyObject_HasAttrString(pyobj, (char*) k);
	JP_PY_CHECK();
	return res != 0;
}

JPPyObject JPPyObject::getAttrString(const char* k)
{
	return JPPyObject(JPPyRef::_call, PyObject_GetAttrString(pyobj, (char*) k)); // new reference
}

JPPyObject JPPyObject::getAttrString(PyObject* pyobj, const char* k)
{
	return JPPyObject(JPPyRef::_call, PyObject_GetAttrString(pyobj, (char*) k)); // new reference
}

const char* JPPyObject::getTypeName()
{
	return Py_TYPE(pyobj)->tp_name;
}

const char* JPPyObject::getTypeName(PyObject* obj)
{
	if (obj == NULL)
		return "null";
	return Py_TYPE(obj)->tp_name;
}

JPPyObject JPPyObject::call(PyObject* args, PyObject* kwargs)
{
	ASSERT_NOT_NULL(pyobj);
	ASSERT_NOT_NULL(args);
	return JPPyObject(JPPyRef::_call, PyObject_Call(pyobj, args, kwargs));
}

bool JPPyObject::isInstance(PyObject* pyobj, PyObject* type)
{
	int res = PyObject_IsInstance(pyobj, type);
	JP_PY_CHECK();
	return res != 0;
}

bool JPPyObject::isSubclass(PyObject* pycls, PyObject* type)
{
	int res = PyObject_IsSubclass(pycls, type);
	return res != 0;
}

bool JPPyType::check(PyObject* obj)
{
	return PyType_Check(obj);
}

JPPyObject JPPyObject::getNone()
{
	return JPPyObject(JPPyRef::_use, Py_None);
}

/****************************************************************************
 * Number types
 ***************************************************************************/

bool JPPyBool::check(PyObject* obj)
{
	return PyBool_Check(obj);
}

JPPyObject JPPyBool::fromLong(jlong value)
{
	return JPPyObject(JPPyRef::_claim, PyBool_FromLong(value ? 1 : 0));
}

JPPyObject JPPyInt::fromInt(jint l)
{
#if PY_MAJOR_VERSION >= 3 
	return JPPyObject(JPPyRef::_call, PyLong_FromLong(l));
#else
	return JPPyObject(JPPyRef::_call, PyInt_FromLong(l));
#endif
}

JPPyObject JPPyInt::fromLong(jlong l)
{
#if PY_MAJOR_VERSION >= 3 
	return JPPyObject(JPPyRef::_call, PyLong_FromLongLong(l));
#else
	return JPPyObject(JPPyRef::_call, PyInt_FromLong((int) l));
#endif
}

bool JPPyInt::check(PyObject* obj)
{
#if PY_MAJOR_VERSION >= 3 || LONG_MAX > 2147483647
	return false;
#else
	return PyInt_Check(obj);
#endif
}

jint JPPyInt::asInt(PyObject* obj)
{
	jint res = PyInt_AsLong(obj);
	JP_PY_CHECK();
	return res;
}

//=====================================================================
// JPLong

JPPyObject JPPyLong::fromLong(jlong l)
{
	return JPPyObject(JPPyRef::_call, PyLong_FromLongLong(l));
}

bool JPPyLong::check(PyObject* obj)
{
#if PY_MAJOR_VERSION >= 3
	return PyLong_Check(obj);
#else
	return PyInt_Check(obj)
			|| PyLong_Check(obj);
#endif
}

bool JPPyLong::checkConvertable(PyObject* obj)
{
#if PY_MAJOR_VERSION >= 3
	return PyLong_Check(obj)
			|| PyObject_HasAttrString(obj, "__int__");
#else
	return PyInt_Check(obj)
			|| PyLong_Check(obj)
			|| PyObject_HasAttrString(obj, "__int__")
			|| PyObject_HasAttrString(obj, "__long__");
#endif
}

bool JPPyLong::checkIndexable(PyObject* obj)
{
	return PyObject_HasAttrString(obj, "__index__");
}

jlong JPPyLong::asLong(PyObject* obj)
{
	jlong res;
#if PY_MAJOR_VERSION >= 3
	res = PyLong_AsLongLong(obj);
#elif LONG_MAX > 2147483647
	res = PyInt_Check(obj) ? PyInt_AsLong(obj) : PyLong_AsLongLong(obj);
#else
	res = PyLong_AsLongLong(obj);
#endif
	JP_PY_CHECK();
	return res;
}

//=====================================================================
// JPPyFloat

JPPyObject JPPyFloat::fromFloat(jfloat l)
{
	return JPPyObject(JPPyRef::_call, PyFloat_FromDouble(l));
}

JPPyObject JPPyFloat::fromDouble(jdouble l)
{
	return JPPyObject(JPPyRef::_call, PyFloat_FromDouble(l));
}

bool JPPyFloat::check(PyObject* obj)
{
	return PyFloat_Check(obj);
}

bool JPPyFloat::checkConvertable(PyObject* obj)
{
	return PyFloat_Check(obj) || PyObject_HasAttrString(obj, "__float__");
}

jdouble JPPyFloat::asDouble(PyObject* obj)
{
	jdouble res = PyFloat_AsDouble(obj);
	JP_PY_CHECK();
	return res;
}

/****************************************************************************
 * String
 ***************************************************************************/

/*
// This is needed for unicode to jchar[] in array conversions
void JPPyString::getRawUnicodeString(jchar** outBuffer, jlong& outSize)
{
  // FIXME jni uses a different encoding than is standard, thus we may need conversion here.
  outSize = length();
 *outBuffer = (jchar*)PyUnicode_AsUnicode(pyobj);
}
 */

JPPyObject JPPyString::fromCharUTF16(jchar c)
{
#if PY_MAJOR_VERSION < 3 || defined(PYPY_VERSION)
	wchar_t buf[1];
	buf[0] = c;
	return JPPyObject(JPPyRef::_call, PyUnicode_FromWideChar(buf, 1));
#else
	if (c < 128)
	{
		char c1 = c;
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
	JP_TRACE_IN_C("JPPyString::checkCharUTF16");
	if (JPPyLong::checkIndexable(pyobj))
		return true;
#if PY_MAJOR_VERSION < 3
	if (PyUnicode_Check(pyobj) && PyUnicode_GET_SIZE(pyobj) == 1)
		return true;
	if (PyString_Check(pyobj) && PyString_Size(pyobj) == 1)
		return true;
#else
	if (PyUnicode_Check(pyobj) && PyUnicode_GET_LENGTH(pyobj) == 1)
		return true;
	if (PyBytes_Check(pyobj) && PyBytes_Size(pyobj) == 1)
		return true;
#endif
	return false;
	JP_TRACE_OUT_C;
}

jchar JPPyString::asCharUTF16(PyObject* pyobj)
{
	if (JPPyLong::checkConvertable(pyobj))
	{
		jlong val = JPPyLong::asLong(pyobj);
		if (val < 0 || val > 65535)
		{
			JP_RAISE_OVERFLOW_ERROR("Unable to convert int into char range");
		}
		return (jchar) val;
	}

#if PY_MAJOR_VERSION < 3 
	if (PyString_Check(pyobj))
	{
		Py_ssize_t sz = PyString_Size(pyobj);
		if (sz != 1)
			JP_RAISE_VALUE_ERROR("Char must be length 1");

		jchar c = PyString_AsString(pyobj)[0];
		if (PyErr_Occurred())
			JP_RAISE_PYTHON("Error in byte conversion");
		return c;
	}

	if (PyUnicode_Check(pyobj))
	{
		if (PyUnicode_GET_SIZE(pyobj) > 1)
			JP_RAISE_VALUE_ERROR("Char must be length 1");

		wchar_t buffer;
		PyUnicode_AsWideChar((PyUnicodeObject*) pyobj, &buffer, 1);
		return buffer;
	}
#elif defined(PYPY_VERSION)
	if (PyBytes_Check(pyobj))
	{
		int sz = PyBytes_Size(pyobj);
		if (sz != 1)
			JP_RAISE_VALUE_ERROR("Char must be length 1");

		jchar c = PyBytes_AsString(pyobj)[0];
		if (PyErr_Occurred())
			JP_RAISE_PYTHON("Error in byte conversion");
		return c;
	}
	if (PyUnicode_Check(pyobj))
	{
		if (PyUnicode_GET_LENGTH(pyobj) > 1)
			JP_RAISE_VALUE_ERROR("Char must be length 1");

		PyUnicode_READY(pyobj);
		Py_UCS4 value = PyUnicode_READ_CHAR(pyobj, 0);
		if (value > 0xffff)
		{
			JP_RAISE_VALUE_ERROR("Unable to pack 4 byte unicode into java char");
		}
		return value;
	}
#else
	if (PyBytes_Check(pyobj))
	{
		int sz = PyBytes_Size(pyobj);
		if (sz != 1)
			JP_RAISE_VALUE_ERROR("Char must be length 1");

		jchar c = PyBytes_AsString(pyobj)[0];
		if (PyErr_Occurred())
			JP_RAISE_PYTHON("Error in byte conversion");
		return c;
	}
	if (PyUnicode_Check(pyobj))
	{
		if (PyUnicode_GET_LENGTH(pyobj) > 1)
			JP_RAISE_VALUE_ERROR("Char must be length 1");

		PyUnicode_READY(pyobj);
		Py_UCS4 value = PyUnicode_ReadChar(pyobj, 0);
		if (value > 0xffff)
		{
			JP_RAISE_VALUE_ERROR("Unable to pack 4 byte unicode into java char");
		}
		return value;
	}
#endif
	JP_RAISE_RUNTIME_ERROR("error converting string to char");
	return 0;
}

/** Check if the object is a bytes or unicode.
 * 
 * @returns true if the object is bytes or unicode.
 */
bool JPPyString::check(PyObject* obj)
{
#if PY_MAJOR_VERSION < 3
	return PyUnicode_Check(obj) || PyString_Check(obj);
#else
	return PyUnicode_Check(obj) || PyBytes_Check(obj);
#endif
}

/** Create a new string from utf8 encoded string.
 * Note: java utf8 is not utf8.
 */
JPPyObject JPPyString::fromStringUTF8(const string& str, bool unicode)
{
	JP_TRACE_IN_C("JPPyString::fromStringUTF8");
	size_t len = str.size();

#if PY_MAJOR_VERSION < 3
	// Python 2 is unicode only on request
	if (unicode)
	{
		return JPPyObject(JPPyRef::_call, PyUnicode_FromStringAndSize(str.c_str(), len));
	}
	else
	{
		return JPPyObject(JPPyRef::_call, PyString_FromStringAndSize(str.c_str(), len));
	}
#else
	// Python 3 is always unicode
	JPPyObject bytes(JPPyRef::_call, PyBytes_FromStringAndSize(str.c_str(), len));
	return JPPyObject(JPPyRef::_call, PyUnicode_FromEncodedObject(bytes.get(), "UTF-8", "strict"));
#endif
	JP_TRACE_OUT_C;
}

string JPPyString::asStringUTF8(PyObject* pyobj)
{
	JP_TRACE_IN_C("JPPyUnicode::asStringUTF8");
	ASSERT_NOT_NULL(pyobj);

#if PY_MAJOR_VERSION < 3
	if (PyUnicode_Check(pyobj))
	{
		Py_ssize_t size = 0;
		char *buffer = NULL;
		JPPyObject val(JPPyRef::_call, PyUnicode_AsEncodedString(pyobj, "UTF-8", "strict"));
		PyBytes_AsStringAndSize(val.get(), &buffer, &size); // internal reference
		JP_PY_CHECK();
		if (buffer != NULL)
			return string(buffer, size);
		else
			return string();
	}
	else
	{
		char *buffer = PyString_AsString(pyobj); // internal reference
		JP_PY_CHECK();
		return string(buffer);
	}
#else
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
	}
	else if (PyBytes_Check(pyobj))
	{
		Py_ssize_t size = 0;
		char *buffer = NULL;
		PyBytes_AsStringAndSize(pyobj, &buffer, &size);
		JP_PY_CHECK();
		return string(buffer, size);
	}
#endif
	JP_RAISE_RUNTIME_ERROR("Failed to convert to string.");
	return string();
	JP_TRACE_OUT_C;
}

bool JPPyMemoryView::check(PyObject* obj)
{
	return PyMemoryView_Check(obj); // macro, cannot fail
}

void JPPyMemoryView::getByteBufferSize(PyObject* obj, char** outBuffer, long& outSize)
{
	JP_TRACE_IN_C("JPPyMemoryView::getByteBufferPtr");
	Py_buffer* py_buf = PyMemoryView_GET_BUFFER(obj); // macro, does no checks
	*outBuffer = (char*) py_buf->buf;
	outSize = (long) py_buf->len;
	JP_TRACE_OUT_C;
}


/****************************************************************************
 * Container types
 ***************************************************************************/

//=====================================================================
// JPPyTuple

JPPyTuple JPPyTuple::newTuple(jlong sz)
{
	return JPPyTuple(JPPyRef::_call, PyTuple_New((Py_ssize_t) sz));
}

bool JPPyTuple::check(PyObject* obj)
{
	return (PyTuple_Check(obj)) ? true : false;
}

void JPPyTuple::setItem(jlong ndx, PyObject* val)
{
	ASSERT_NOT_NULL(val);
	PyTuple_SetItem(pyobj, (Py_ssize_t) ndx, val); // steals reference
	JP_PY_CHECK();

	// Return the stolen reference, but only after transfer has been completed.
	Py_INCREF(val);
}

PyObject* JPPyTuple::getItem(jlong ndx)
{
	PyObject* res = PyTuple_GetItem(pyobj, (Py_ssize_t) ndx);
	JP_PY_CHECK();
	return res;
}

jlong JPPyTuple::size()
{
	jlong res = PyTuple_Size(pyobj);
	JP_PY_CHECK();
	return res;
}

//=====================================================================
// JPPyList

JPPyList JPPyList::newList(jlong sz)
{
	return JPPyList(JPPyRef::_call, PyList_New((Py_ssize_t) sz));
}

bool JPPyList::check(PyObject* obj)
{
	return (PyList_Check(obj)) ? true : false;
}

void JPPyList::setItem(jlong ndx, PyObject* val)
{
	ASSERT_NOT_NULL(val);
	PySequence_SetItem(pyobj, (Py_ssize_t) ndx, val); // Does not steal
	JP_PY_CHECK();
}

PyObject* JPPyList::getItem(jlong ndx)
{
	PyObject* res = PyList_GetItem(pyobj, (Py_ssize_t) ndx);
	JP_PY_CHECK();
	return res;
}

//=====================================================================
// JPPySequence

bool JPPySequence::check()
{
	if (pyobj == NULL)
		return false;
	return (PySequence_Check(pyobj)) ? true : false;
}

bool JPPySequence::check(PyObject* obj)
{
	return (PySequence_Check(obj)) ? true : false;
}

jlong JPPySequence::size()
{
	if (pyobj == NULL)
		return 0;
	return PySequence_Size(pyobj);
}

JPPyObject JPPySequence::getItem(jlong ndx)
{
	return JPPyObject(JPPyRef::_call, PySequence_GetItem(pyobj, ndx)); // new reference
}

JPPyObjectVector::JPPyObjectVector(int i)
: seq(JPPyRef::_use, NULL)
{
	contents.resize(i);
}

JPPyObjectVector::JPPyObjectVector(PyObject* sequence)
: seq(JPPyRef::_use, sequence)
{
	if (!JPPySequence::check(sequence))
		JP_RAISE_TYPE_ERROR("must be sequence");
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


//=====================================================================
// JPPyDict

bool JPPyDict::contains(PyObject* k)
{
	int res = PyMapping_HasKey(pyobj, k);
	JP_PY_CHECK();
	return res != 0;
}

PyObject* JPPyDict::getItem(PyObject* k)
{
	PyObject* res = PyDict_GetItem(pyobj, k);
	JP_PY_CHECK();
	return res;
}

bool JPPyDict::check(PyObject* obj)
{
	return PyDict_Check(obj);
}

JPPyObject JPPyDict::getKeys()
{
	return JPPyObject(JPPyRef::_call, PyDict_Keys(pyobj));
}

JPPyObject JPPyDict::copy(PyObject* m)
{
	return JPPyObject(JPPyRef::_call, PyDict_Copy(m));
}

JPPyDict JPPyDict::newDict()
{
	return JPPyObject(JPPyRef::_call, PyDict_New());
}

void JPPyDict::setItemString(PyObject* o, const char* n)
{
	PyDict_SetItemString(pyobj, n, o);
	JP_PY_CHECK();
}

void JPPyErr::clear()
{
	PyErr_Clear();
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
	exceptionClass = JPPyObject(JPPyRef::_claim, v1);
	exceptionValue = JPPyObject(JPPyRef::_claim, v2);
	exceptionTrace = JPPyObject(JPPyRef::_claim, v3);
	return true;
}

void JPPyErr::restore(JPPyObject& exceptionClass, JPPyObject& exceptionValue, JPPyObject& exceptionTrace)
{
	PyErr_Restore(exceptionClass.keep(), exceptionValue.keep(), exceptionTrace.keep());
}

int count=0;
JPPyCallAcquire::JPPyCallAcquire()
{
	audit = count++;
	state1 = PyThreadState_New(PyJPModule::s_Interpreter);
	PyEval_AcquireThread((PyThreadState*)state1);
	JP_TRACE_GIL("GIL ACQUIRE", audit);
}

JPPyCallAcquire::~JPPyCallAcquire()
{
	JP_TRACE_GIL("GIL ACQUIRE DONE", audit);
	PyThreadState_Clear((PyThreadState*) state1);
	PyEval_ReleaseThread((PyThreadState*) state1);
	PyThreadState_Delete((PyThreadState*) state1);
}

// This is used when leaving python from to perform some 

JPPyCallRelease::JPPyCallRelease()
{
	audit = count++;
	JP_TRACE_GIL("GIL RELEASE", audit);
	// Release the lock and set the thread state to NULL
	state1 = (void*) PyEval_SaveThread();
}

JPPyCallRelease::~JPPyCallRelease()
{
	// Reacquire the lock
	PyThreadState *save = (PyThreadState *) state1;
	PyEval_RestoreThread(save);
	JP_TRACE_GIL("GIL RELEASE DONE", audit);
}

