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

#ifdef __cplusplus
extern "C"
{
#endif

PyObject *PyJPArray_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
void PyJPArray_dealloc(PyJPArray *self);
PyObject *PyJPArray_repr(PyJPArray *self);
PyObject *PyJPArray_getArrayLength(PyJPArray *self, PyObject *arg);
PyObject *PyJPArray_getArrayItem(PyJPArray *self, PyObject *arg);
PyObject *PyJPArray_getArraySlice(PyJPArray *self, PyObject *arg);
PyObject *PyJPArray_setArraySlice(PyJPArray *self, PyObject *arg);
PyObject *PyJPArray_setArrayItem(PyJPArray *self, PyObject *arg);

static PyMethodDef arrayMethods[] = {
	{"__len__", (PyCFunction) (&PyJPArray_getArrayLength), METH_NOARGS, ""},
	{"_getArrayItem", (PyCFunction) (&PyJPArray_getArrayItem), METH_VARARGS, ""},
	{"_setArrayItem", (PyCFunction) (&PyJPArray_setArrayItem), METH_VARARGS, ""},
	{"_getArraySlice", (PyCFunction) (&PyJPArray_getArraySlice), METH_VARARGS, ""},
	{"_setArraySlice", (PyCFunction) (&PyJPArray_setArraySlice), METH_VARARGS, ""},
	{NULL},
};

static PyType_Slot arraySlots[] = {
	{ Py_tp_new,     PyJPArray_new},
	{ Py_tp_dealloc, (destructor) PyJPArray_dealloc},
	{ Py_tp_repr,    (reprfunc) PyJPArray_repr},
	{ Py_tp_methods, &arrayMethods},
	{0}
};

PyType_Spec PyJPArraySpec = {
	"_jpype.PyJPArray",
	sizeof (PyJPArray),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
	arraySlots
};

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
PyObject *PyJPArray_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	try
	{
		JP_TRACE_IN_C("PyJPArray_new");
		PyJPModuleState *state = PyJPModuleState_global;
		PyTypeObject *base = (PyTypeObject *) state->PyJPValue_Type;
		PyObject *self = base->tp_new(type, args, kwargs);
		((PyJPArray*) self)->m_Array = NULL;
		return self;
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

void PyJPArray_dealloc(PyJPArray *self)
{
	try
	{
		JP_TRACE_IN_C("PyJPArray_dealloc");
		PyJPModuleState *state = PyJPModuleState_global;
		PyTypeObject *base = (PyTypeObject *) state->PyJPValue_Type;
		delete self->m_Array;
		base->tp_dealloc((PyObject*) self);
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH();
}

PyObject *PyJPArray_repr(PyJPArray *self)
{
	try
	{
		JP_TRACE_IN_C("PyJPArray_repr");
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		JPJavaFrame frame(context);
		stringstream sout;

		// FIXME way too hard to get this type name.
		sout << "<java array " << self->m_Array->getClass()->toString() << ">";
		return JPPyString::fromStringUTF8(sout.str()).keep();
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(0);
}

PyObject *PyJPArray_getArrayLength(PyJPArray *self, PyObject *arg)
{
	try
	{
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		return PyInt_FromLong(self->m_Array->getLength());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPArray_getArrayItem(PyJPArray *self, PyObject *arg)
{
	try
	{
		JP_TRACE_IN_C("PyJPArray::getArrayItem");
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		JPJavaFrame frame(context);
		int ndx;
		PyArg_ParseTuple(arg, "i", &ndx);
		JP_PY_CHECK();
		return self->m_Array->getItem(ndx).keep();
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPArray_getArraySlice(PyJPArray *self, PyObject *arg)
{
	try
	{
		JP_TRACE_IN_C("PyJPArray::getArraySlice");
		int lo = -1;
		int hi = -1;
		JPContext *context = PyJPValue_GET_CONTEXT(self);
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
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPArray_setArraySlice(PyJPArray *self, PyObject *arg)
{
	try
	{
		JP_TRACE_IN_C("PyJPArray::setArraySlice");
		JPContext *context = PyJPValue_GET_CONTEXT(self);
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
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPArray_setArrayItem(PyJPArray *self, PyObject *arg)
{
	try
	{
		JP_TRACE_IN_C("PyJPArray::setArrayItem");
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		JPJavaFrame frame(context);

		int ndx;
		PyObject *value;
		PyArg_ParseTuple(arg, "iO", &ndx, &value);
		JP_PY_CHECK();

		self->m_Array->setItem(ndx, value);
		Py_RETURN_NONE;
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

#ifdef __cplusplus
}
#endif