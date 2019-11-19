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

static PyMethodDef arrayMethods[] = {
	{"__len__", (PyCFunction) (&PyJPArray::getArrayLength), METH_NOARGS, ""},
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
	/* tp_repr           */ (reprfunc) PyJPArray::__repr__,
	/* tp_as_number      */ 0,
	/* tp_as_sequence    */ 0,
	/* tp_as_mapping     */ 0,
	/* tp_hash           */ 0,
	/* tp_call           */ 0,
	/* tp_str            */ 0,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
	/* tp_doc            */ "Java array instance",
	/* tp_traverse       */ 0,
	/* tp_clear          */ 0,
	/* tp_richcompare    */ 0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter           */ 0,
	/* tp_iternext       */ 0,
	/* tp_methods        */ arrayMethods,
	/* tp_members        */ 0,
	/* tp_getset         */ 0,
	/* tp_base           */ &PyJPValue::Type,
	/* tp_dict           */ 0,
	/* tp_descr_get      */ 0,
	/* tp_descr_set      */ 0,
	/* tp_dictoffset     */ 0,
	/* tp_init           */ 0,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPArray::__new__
};

// Static methods

void PyJPArray::initType(PyObject *module)
{
	PyType_Ready(&PyJPArray::Type);
	Py_INCREF(&PyJPArray::Type);
	PyModule_AddObject(module, "PyJPArray", (PyObject*) & PyJPArray::Type);
}

JPPyObject PyJPArray::alloc(PyTypeObject *wrapper, JPContext *context, JPClass *cls, jvalue value)
{
	JP_TRACE_IN_C("PyJPArray::alloc");
	JPPyObject self = PyJPValue::alloc(wrapper, context, cls, value);
	((PyJPArray*) self.get())->m_Array = new JPArray(cls, (jarray) value.l);
	return self;
	JP_TRACE_OUT_C;
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
PyObject *PyJPArray::__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	PyJPArray *self = (PyJPArray*) PyJPValue::__new__(type, args, kwargs);
	self->m_Array = NULL;
	return (PyObject*) self;
}

void PyJPArray::__dealloc__(PyJPArray *self)
{
	JP_TRACE_IN_C("PyJPArray::__dealloc__");
	delete self->m_Array;
	PyJPValue::__dealloc__((PyJPValue*) self);
	JP_TRACE_OUT_C;
}

PyObject *PyJPArray::__repr__(PyJPArray *self)
{
	JP_TRACE_IN_C("PyJPArray::__repr__");
	JPContext *context = self->m_Value.m_Context->m_Context;
	try
	{
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		stringstream sout;

		// FIXME way too hard to get this type name.
		sout << "<java array " << self->m_Array->getClass()->toString() << ">";
		return JPPyString::fromStringUTF8(sout.str()).keep();
	}
	PY_STANDARD_CATCH(0);
	JP_TRACE_OUT_C;
}

PyObject *PyJPArray::getArrayLength(PyJPArray *self, PyObject *arg)
{
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		return PyInt_FromLong(self->m_Array->getLength());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPArray::getArrayItem(PyJPArray *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPArray::getArrayItem");
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		int ndx;
		PyArg_ParseTuple(arg, "i", &ndx);
		JP_PY_CHECK();
		return self->m_Array->getItem(ndx).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPArray::getArraySlice(PyJPArray *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPArray::getArraySlice");
	int lo = -1;
	int hi = -1;
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);

		PyArg_ParseTuple(arg, "ii", &lo, &hi);
		JP_PY_CHECK();

		JPArray *a = (JPArray*) self->m_Array;
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
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPArray::setArraySlice(PyJPArray *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPArray::setArraySlice");
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);

		// Parse arguments
		PyObject *sequence;
		int lo = -1;
		int hi = -1;
		PyArg_ParseTuple(arg, "iiO", &lo, &hi, &sequence);
		JP_PY_CHECK();

		JPArray *a = (JPArray*) self->m_Array;

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
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPArray::setArrayItem(PyJPArray *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPArray::setArrayItem");
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);

		int ndx;
		PyObject *value;
		PyArg_ParseTuple(arg, "iO", &ndx, &value);
		JP_PY_CHECK();

		self->m_Array->setItem(ndx, value);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}
