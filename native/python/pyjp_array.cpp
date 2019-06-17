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

// FIXME PyJPArray should inherit from PyJPValue so that arrays are
// properly specializations of value types.

#include <pyjp.h>

static PyMethodDef classMethods[] = {
	{"getArrayLength", (PyCFunction) (&PyJPArray::getArrayLength), METH_NOARGS, ""},
	{"getArrayItem", (PyCFunction) (&PyJPArray::getArrayItem), METH_VARARGS, ""},
	{"setArrayItem", (PyCFunction) (&PyJPArray::setArrayItem), METH_VARARGS, ""},
	{"getArraySlice", (PyCFunction) (&PyJPArray::getArraySlice), METH_VARARGS, ""},
	{"setArraySlice", (PyCFunction) (&PyJPArray::setArraySlice), METH_VARARGS, ""},
	{NULL},
};

PyTypeObject PyJPArray::Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	/* tp_name           */ "_jpype.PyJPArray",
	/* tp_basicsize      */ sizeof (PyJPArray),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPArray::__dealloc__,
	/* tp_print          */ 0,
	/* tp_getattr        */ 0,
	/* tp_setattr        */ 0,
	/* tp_compare        */ 0,
	/* tp_repr           */ 0,
	/* tp_as_number      */ 0,
	/* tp_as_sequence    */ 0,
	/* tp_as_mapping     */ 0,
	/* tp_hash           */ 0,
	/* tp_call           */ 0,
	/* tp_str            */ (reprfunc) PyJPArray::__str__,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT,
	/* tp_doc            */ "Java array instance",
	/* tp_traverse       */ 0,
	/* tp_clear          */ 0,
	/* tp_richcompare    */ 0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter           */ 0,
	/* tp_iternext       */ 0,
	/* tp_methods        */ classMethods,
	/* tp_members        */ 0,
	/* tp_getset         */ 0,
	/* tp_base           */ 0,
	/* tp_dict           */ 0,
	/* tp_descr_get      */ 0,
	/* tp_descr_set      */ 0,
	/* tp_dictoffset     */ 0,
	/* tp_init           */ (initproc) PyJPArray::__init__,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPArray::__new__
};

// Static methods

void PyJPArray::initType(PyObject* module)
{
	PyType_Ready(&PyJPArray::Type);
	Py_INCREF(&PyJPArray::Type);
	PyModule_AddObject(module, "PyJPArray", (PyObject*) & PyJPArray::Type);
}

bool PyJPArray::check(PyObject* o)
{
	return Py_TYPE(o) == &PyJPArray::Type;
}

JPPyObject PyJPArray::alloc(JPArray* obj)
{
	JPJavaFrame fame;
	JP_TRACE_IN("PyJPArray::alloc");
	PyJPArray* res = (PyJPArray*) PyJPArray::Type.tp_alloc(&PyJPArray::Type, 0);
	JP_PY_CHECK();
	res->m_Array = obj;
	return JPPyObject(JPPyRef::_claim, (PyObject*) res);
	JP_TRACE_OUT;
}

PyObject* PyJPArray::__new__(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
	PyJPArray* self = (PyJPArray*) type->tp_alloc(type, 0);
	self->m_Array = NULL;
	return (PyObject*) self;
}

int PyJPArray::__init__(PyJPArray* self, PyObject* args, PyObject* kwargs)
{
	JP_TRACE_IN("PyJPArray::__init__");
	try
	{
		ASSERT_JVM_RUNNING("PyJPArray::__init__");
		JPJavaFrame frame;

		PyObject* v;
		if (!PyArg_ParseTuple(args, "O!", &PyJPValue::Type, &v))
		{
			return -1;
		}
		JPValue& val = ((PyJPValue*) v)->m_Value;

		JPArrayClass* arrayClass = dynamic_cast<JPArrayClass*> (val.getClass());
		if (arrayClass == NULL)
		{
			PyErr_SetString(PyExc_TypeError, "Class must be array type");
			return -1;
		}

		self->m_Array = new JPArray(arrayClass, (jarray) (val.getJavaObject()));
		return 0;
	}
	PY_STANDARD_CATCH;
	return -1;
	JP_TRACE_OUT;
}

PyObject* PyJPArray::__str__(PyJPArray* self)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPArray::__str__");
		JPJavaFrame frame;
		stringstream sout;

		// FIXME way too hard to get this type name.
		sout << "<java array " << self->m_Array->getClass()->toString() << ">";
		return JPPyString::fromStringUTF8(sout.str()).keep();
	}
	PY_STANDARD_CATCH;
	Py_RETURN_NONE;
}

void PyJPArray::__dealloc__(PyJPArray* self)
{
	JP_TRACE_IN("PyJPArray::dealloc");
	JPArray* array = self->m_Array;
	Py_TYPE(self)->tp_free(self);
	delete array;
	JP_TRACE_OUT;
}

PyObject* PyJPArray::getArrayLength(PyJPArray* self, PyObject* arg)
{
	try
	{
		ASSERT_JVM_RUNNING("JPypeJavaArray::getArrayLength");
		JPJavaFrame frame;
		return PyInt_FromLong(self->m_Array->getLength());
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPArray::getArrayItem(PyJPArray* self, PyObject* arg)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPArray::getArrayItem");
		JPJavaFrame frame;
		int ndx;
		PyArg_ParseTuple(arg, "i", &ndx);
		JP_PY_CHECK();
		return self->m_Array->getItem(ndx).keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPArray::getArraySlice(PyJPArray* self, PyObject* arg)
{
	int lo = -1;
	int hi = -1;
	try
	{
		ASSERT_JVM_RUNNING("PyJPArray::getArraySlice");
		JPJavaFrame frame;

		PyArg_ParseTuple(arg, "ii", &lo, &hi);
		JP_PY_CHECK();

		JPArray* a = (JPArray*) self->m_Array;
		int length = a->getLength();

		// stolen from jcc, to get nice slice support
		if (lo < 0) lo = length + lo;
		if (lo < 0) lo = 0;
		else if (lo > length) lo = length;
		if (hi < 0) hi = length + hi;
		if (hi < 0) hi = 0;
		else if (hi > length) hi = length;
		if (lo > hi) lo = hi;

		return a->getRange(lo, hi).keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPArray::setArraySlice(PyJPArray* self, PyObject* arg)
{
	JP_TRACE_IN("PyJPArray::setArraySlice")
	try
	{
		ASSERT_JVM_RUNNING("PyJPArray::setArraySlice");
		JPJavaFrame frame;

		// Parse arguments
		PyObject* sequence;
		int lo = -1;
		int hi = -1;
		PyArg_ParseTuple(arg, "iiO", &lo, &hi, &sequence);
		JP_PY_CHECK();

		JPArray* a = (JPArray*) self->m_Array;

		int length = a->getLength();
		if (length == 0)
			Py_RETURN_NONE;

		if (lo < 0) lo = length + lo;
		if (lo < 0) lo = 0;
		else if (lo > length) lo = length;
		if (hi < 0) hi = length + hi;
		if (hi < 0) hi = 0;
		else if (hi > length) hi = length;
		if (lo > hi) lo = hi;

		a->setRange(lo, hi, sequence);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH

	return NULL;
	JP_TRACE_OUT
}

PyObject* PyJPArray::setArrayItem(PyJPArray* self, PyObject* arg)
{
	JP_TRACE_IN("JPypeJavaArray::setArrayItem");
	try
	{
		ASSERT_JVM_RUNNING("JPypeJavaArray::setArrayItem");
		JPJavaFrame frame;

		int ndx;
		PyObject* value;
		PyArg_ParseTuple(arg, "iO", &ndx, &value);
		JP_PY_CHECK();

		self->m_Array->setItem(ndx, value);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH

	return NULL;
	JP_TRACE_OUT;
}
