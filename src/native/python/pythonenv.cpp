/*****************************************************************************
   Copyright 2004 Steve M�nard

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
#include <jpype_python.h>

#define PY_CHECK(op) op; { PyObject* __ex = PyErr_Occurred(); if (__ex) { 	throw new PythonException(); }};

bool JPyString::check(PyObject* obj)
{
	return PyString_Check(obj) || PyUnicode_Check(obj);
}

bool JPyString::checkStrict(PyObject* obj)
{
	return PyString_Check(obj);
}

bool JPyString::checkUnicode(PyObject* obj)
{
	return PyUnicode_Check(obj);
}

Py_UNICODE* JPyString::AsUnicode(PyObject* obj)
{
	return PyUnicode_AsUnicode(obj);
}

string JPyString::asString(PyObject* obj) 
{	
	TRACE_IN("JPyString::asString");
	PY_CHECK( string res = string(PyString_AsString(obj)) );
	return res;
	TRACE_OUT;
}

JCharString JPyString::asJCharString(PyObject* obj) 
{	
	PyObject* torelease = NULL;
	TRACE_IN("JPyString::asJCharString");
	
	if (PyString_Check(obj))
	{
		PY_CHECK( obj = PyUnicode_FromObject(obj) );	
		torelease = obj;
	}

	Py_UNICODE* val = PyUnicode_AS_UNICODE(obj);	
	Py_ssize_t length = JPyObject::length(obj);
	JCharString res(length);
	for (int i = 0; val[i] != 0; i++)
	{
		res[i] = (jchar)val[i];
	}

	if (torelease != NULL)
	{
		Py_DECREF(torelease);
	}

	return res;
	TRACE_OUT;
}

PyObject* JPyString::fromUnicode(const jchar* str, int len) 
{
	Py_UNICODE* value = new Py_UNICODE[len+1];
	value[len] = 0;
	for (int i = 0; i < len; i++)
	{
		value[i] = (Py_UNICODE)str[i];
	}
	PY_CHECK( PyObject* obj = PyUnicode_FromUnicode(value, len) );
	delete value;
	return obj;
}

PyObject* JPyString::fromString(const char* str) 
{
	PY_CHECK( PyObject* obj = PyString_FromString(str) );
	return obj;
}


Py_ssize_t JPyString::AsStringAndSize(PyObject *obj, char **buffer, Py_ssize_t *length)
{	
	PY_CHECK( Py_ssize_t res = PyString_AsStringAndSize(obj, buffer, length) );
	return res;
}

PyObject* JPySequence::newTuple(Py_ssize_t sz)
{
	PY_CHECK( PyObject* res = PyTuple_New(sz););
	return res;
}

PyObject* JPySequence::newList(Py_ssize_t sz)
{
	PY_CHECK( PyObject* res = PyList_New(sz););
	return res;
}

void JPySequence::setItem(PyObject* lst, Py_ssize_t ndx, PyObject* val)
{
	if (PyList_Check(lst))
	{
		Py_XINCREF(val);
		PY_CHECK( PyList_SetItem(lst, ndx, val) );
	}
	else if (PyTuple_Check(lst))
	{
		Py_XINCREF(val);
		PY_CHECK( PyTuple_SetItem(lst, ndx, val) );
	}
	else
	{
		Py_XINCREF(val);
		PY_CHECK( PySequence_SetItem(lst, ndx, val) );
	}
}

bool JPySequence::check(PyObject* obj)
{
	if (PySequence_Check(obj) || PyList_Check(obj) || PyTuple_Check(obj))
	{
		return true;
	}
	return false;
}

PyObject* JPySequence::getItem(PyObject* tuple, Py_ssize_t ndx) 
{
	PY_CHECK( PyObject* res = PySequence_GetItem(tuple, ndx) );
	return res;
}

Py_ssize_t JPyObject::length(PyObject* obj) 
{
	PY_CHECK( Py_ssize_t res = PyObject_Length(obj) );
	return res;
}

bool JPyObject::hasAttr(PyObject* m, PyObject* k)
{
	PY_CHECK( int res = PyObject_HasAttr(m, k) );
	if (res) 
		return true;
	return false;
}

PyObject* JPyObject::getAttr(PyObject* m, PyObject* k)
{
	PY_CHECK( PyObject* res = PyObject_GetAttr(m, k) );
	return res;
}

PyObject* JPyObject::getAttrString(PyObject* m, const char* k)
{
	PY_CHECK( PyObject* res = PyObject_GetAttrString(m, (char*)k) );
	return res;
}

void JPyObject::setAttrString(PyObject* m, const char* k, PyObject *v)
{
	PY_CHECK( PyObject_SetAttrString(m, (char*)k, v ) );
}


PyObject* JPyObject::call(PyObject* c, PyObject* a, PyObject* w)
{
	PY_CHECK( PyObject* res = PyObject_Call(c, a, w) );
	return res;
}

bool JPyObject::isInstance(PyObject* obj, PyObject* t)
{
	PY_CHECK( int res = PyObject_IsInstance(obj, t) );
	if (res)
	{
		return true;
	}
	return false;
}

bool JPyObject::isSubclass(PyObject* obj, PyObject* t)
{
	int res = PyObject_IsSubclass(obj, t);
	if (res)
	{
		return true;
	}
	return false;
}

void JPyErr::setString(PyObject* exClass, const char* str)
{
	PyErr_SetString(exClass, str);
}

void JPyErr::setObject(PyObject* exClass, PyObject* str)
{
	PyErr_SetObject(exClass, str);
}

PyObject* JPyInt::fromLong(long l)
{
	TRACE_IN("JPyInt::fromLong");
	PY_CHECK( PyObject* res = PyInt_FromLong(l) );
	return res; 
	TRACE_OUT;
}

bool JPyInt::check(PyObject* obj)
{
	return PyInt_Check(obj);
}

long JPyInt::asLong(PyObject* obj)
{
	return PyInt_AsLong(obj);
}

PyObject* JPyLong::fromLongLong(PY_LONG_LONG l)
{
	TRACE_IN("JPyLong::fromLongLong");
	PY_CHECK( PyObject* res = PyLong_FromLongLong(l) );
	return res; 
	TRACE_OUT;
}

bool JPyLong::check(PyObject* obj)
{
	return PyLong_Check(obj);
}

PY_LONG_LONG JPyLong::asLongLong(PyObject* obj)
{
	return PyLong_AsLongLong(obj);
}

PyObject* JPyFloat::fromDouble(double l)
{
	PY_CHECK( PyObject* res = PyFloat_FromDouble(l) );
	return res; 
}

bool JPyFloat::check(PyObject* obj)
{
	return PyFloat_Check(obj);
}

double JPyFloat::asDouble(PyObject* obj)
{
	return PyFloat_AsDouble(obj);
}

PyObject* JPyBoolean::getTrue()
{
	return PyInt_FromLong(1);
}

PyObject* JPyBoolean::getFalse()
{
	return PyInt_FromLong(0);
}

bool JPyDict::contains(PyObject* m, PyObject* k)
{
	PY_CHECK( int res = PyMapping_HasKey(m, k) );
	if (res) 
		return true;
	return false;
}

PyObject* JPyDict::getItem(PyObject* m, PyObject* k)
{
	PY_CHECK( PyObject* res = PyDict_GetItem(m, k) );
	Py_XINCREF(res);
	return res;
}

bool JPyDict::check(PyObject* obj)
{
	return PyDict_Check(obj);
}

PyObject* JPyDict::getKeys(PyObject* m)
{
	PY_CHECK( PyObject* res = PyDict_Keys(m) );
	return res;
}

PyObject* JPyDict::copy(PyObject* m)
{
	PY_CHECK( PyObject* res = PyDict_Copy(m) );
	return res;
}

PyObject* JPyDict::newInstance()
{
	PY_CHECK( PyObject* res = PyDict_New() );
	return res;
}

void JPyDict::setItemString(PyObject* d, PyObject* o, const char* n)
{
	PY_CHECK( PyDict_SetItemString(d, n, o) );
}

PythonException::PythonException()
{
	TRACE_IN("PythonException::PythonException");
	PyObject* traceback;
	PyErr_Fetch(&m_ExceptionClass, &m_ExceptionValue, &traceback);
	Py_INCREF(m_ExceptionClass);
	Py_INCREF(m_ExceptionValue);

	PyObject* name = JPyObject::getAttrString(m_ExceptionClass, "__name__");
	string ascname = JPyString::asString(name);
	TRACE1(ascname);
	Py_DECREF(name);
	TRACE1(m_ExceptionValue->ob_type->tp_name);

	if (JPySequence::check(m_ExceptionValue))
	{

	}

	PyErr_Restore(m_ExceptionClass, m_ExceptionValue, traceback);
	TRACE_OUT;
}

PythonException::PythonException(PythonException& ex)
{
	m_ExceptionClass = ex.m_ExceptionClass;
	Py_INCREF(m_ExceptionClass);
	m_ExceptionValue = ex.m_ExceptionValue;
	Py_INCREF(m_ExceptionValue);
}

PythonException::~PythonException()
{
	Py_XDECREF(m_ExceptionClass);
	Py_XDECREF(m_ExceptionValue);
}

PyObject* PythonException::getJavaException()
{
	PyObject* retVal = NULL;

	// If the exception was caught further down ...
	if (JPySequence::check(m_ExceptionValue) && JPyObject::length(m_ExceptionValue) == 1)
	{
		PyObject* v0 = JPySequence::getItem(m_ExceptionValue, 0);
		if (JPySequence::check(v0) && JPyObject::length(v0) == 2)
		{
			PyObject* v00 = JPySequence::getItem(v0, 0);
			PyObject* v01 = JPySequence::getItem(v0, 1);

			if (v00 == hostEnv->getSpecialConstructorKey())
			{
				retVal = v01;
			}
			else
			{
				Py_DECREF(v01);
			}

			Py_DECREF(v00);
		}
		else
		{
			Py_DECREF(v0);
		}
	}
	else
	{
		Py_XINCREF(m_ExceptionValue);
		retVal = m_ExceptionValue;
	}
	return retVal;
}

PyObject* JPyCObject::fromVoid(void* data, void (*destr)(void *))
{
	PY_CHECK( PyObject* res = PyCObject_FromVoidPtr(data, destr) );
	return res;
}

PyObject* JPyCObject::fromVoidAndDesc(void* data, void* desc, void (*destr)(void *, void*))
{
	PY_CHECK( PyObject* res = PyCObject_FromVoidPtrAndDesc(data, desc, destr) );
	return res;
}

void* JPyCObject::asVoidPtr(PyObject* obj)
{
	PY_CHECK( void* res = PyCObject_AsVoidPtr(obj) );
	return res;
}

void* JPyCObject::getDesc(PyObject* obj)
{
	PY_CHECK( void* res = PyCObject_GetDesc(obj) );
	return res;
}

bool JPyCObject::check(PyObject* obj)
{
	return PyCObject_Check(obj);
}

bool JPyType::check(PyObject* obj)
{
	return PyType_Check(obj);
}

bool JPyType::isSubclass(PyObject* o1, PyObject* o2)
{
	if (PyType_IsSubtype((PyTypeObject*)o1, (PyTypeObject*)o2))
	{
		return true;
	}
	return false;
}

void JPyHelper::dumpSequenceRefs(PyObject* seq, const char* comment)
{
	cerr << "Dumping sequence state at " << comment << endl;
	cerr << "   sequence has " << (long)seq->ob_refcnt << " reference(s)" << endl;
	Py_ssize_t dx = PySequence_Length(seq);
	for (Py_ssize_t i = 0; i < dx; i++)
	{
		PyObject* el = PySequence_GetItem(seq, i);
		Py_XDECREF(el); // PySequence_GetItem return a new ref
		cerr << "   item[" << (long)i << "] has " << (long)el->ob_refcnt << " references" << endl;
	}
}

