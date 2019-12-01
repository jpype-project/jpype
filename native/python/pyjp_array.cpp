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
int PyJPArray_setArrayItem(PyJPArray *self, PyObject *item, PyObject* value);

static PyMethodDef arrayMethods[] = {
	{"__getitem__", (PyCFunction) (&PyJPArray_getArrayItem), METH_O | METH_COEXIST, ""},
	{NULL},
};

static PyType_Slot arraySlots[] = {
	{ Py_tp_new,      (void*) PyJPArray_new},
	{ Py_tp_dealloc,  (void*) PyJPArray_dealloc},
	{ Py_tp_repr,     (void*) PyJPArray_repr},
	{ Py_tp_methods,  (void*) &arrayMethods},
	{ Py_sq_item,     (void*) &PyJPArray_getArrayItem},
	{ Py_sq_ass_item, (void*) &PyJPArray_setArrayItem},
	{ Py_sq_length,   (void*) &PyJPArray_getArrayLength},
	{0}
};

PyType_Spec PyJPArraySpec = {
	"_jpype.PyJPArray",
	sizeof (PyJPArray),
	0,
	Py_TPFLAGS_DEFAULT  | Py_TPFLAGS_BASETYPE, //| Py_TPFLAGS_HAVE_GC,
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
	JP_PY_TRY("PyJPArray_new");
	PyJPModuleState *state = PyJPModuleState_global;
	PyTypeObject *base = (PyTypeObject *) state->PyJPValue_Type;
	PyObject *self = base->tp_new(type, args, kwargs);
	((PyJPArray*) self)->m_Array = NULL;
	return self;
	JP_PY_CATCH(NULL);
}

void PyJPArray_dealloc(PyJPArray *self)
{
	JP_PY_TRY("PyJPArray_dealloc");
	PyJPModuleState *state = PyJPModuleState_global;
	PyTypeObject *base = (PyTypeObject *) state->PyJPValue_Type;
	delete self->m_Array;
	base->tp_dealloc((PyObject*) self);
	JP_PY_CATCH();
}

PyObject *PyJPArray_repr(PyJPArray *self)
{
	JP_PY_TRY("PyJPArray_repr");
	JPContext *context = PyJPValue_GET_CONTEXT(self);
	JPJavaFrame frame(context);
	stringstream sout;

	// FIXME way too hard to get this type name.
	sout << "<java array " << self->m_Array->getClass()->toString() << ">";
	return JPPyString::fromStringUTF8(sout.str()).keep();
	JP_PY_CATCH(0);
}

PyObject *PyJPArray_getArrayLength(PyJPArray *self, PyObject *arg)
{
	JP_PY_TRY("PyJPArray_len")
	PyJPValue_GET_CONTEXT(self);
	return PyInt_FromLong(self->m_Array->getLength());
	JP_PY_CATCH(NULL);
}

PyObject *PyJPArray_getArrayItem(PyJPArray *self, PyObject *arg)
{
	JP_PY_TRY("PyJPArray_getArrayItem")
	JPContext *context = PyJPValue_GET_CONTEXT(self);
	JPJavaFrame frame(context);
	if (PyIndex_Check(arg))
	{
		Py_ssize_t i = PyNumber_AsSsize_t(arg, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return NULL;
		if (i < 0)
			i += self->m_Array->getLength();
		return self->m_Array->getItem((jsize) i).keep();
	}

	if (PySlice_Check(arg))
	{
		Py_ssize_t start, stop, step, slicelength;
		if (PySlice_Unpack(arg, &start, &stop, &step) < 0)
			return NULL;

		if (step != 1)
			JP_RAISE_VALUE_ERROR("Slicing step not implemented");

		slicelength = PySlice_AdjustIndices((Py_ssize_t) self->m_Array->getLength(),
				&start, &stop, step);

		if (slicelength <= 0)
			return PyList_New(0);
		return self->m_Array->getRange((jsize) start, (jsize) stop).keep();
	}
	JP_RAISE_TYPE_ERROR("array indices must be indexes or slices");
	JP_PY_CATCH(NULL);
}

int PyJPArray_setArrayItem(PyJPArray *self, PyObject *item, PyObject* value)
{
	JP_PY_TRY("PyJPArray_setArrayItem")
	JPContext *context = PyJPValue_GET_CONTEXT(self);
	JPJavaFrame frame(context);

	if (PyIndex_Check(item))
	{
		Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return -1;
		if (i < 0)
			i += self->m_Array->getLength();
		self->m_Array->setItem((jsize) i, value);
		return 0;
	}

	if (PySlice_Check(item))
	{
		Py_ssize_t start, stop, step, slicelength;

		if (PySlice_Unpack(item, &start, &stop, &step) < 0)
			return -1;

		if (step != 1)
			JP_RAISE_VALUE_ERROR("Slicing step not implemented");

		slicelength = PySlice_AdjustIndices((Py_ssize_t) self->m_Array->getLength(),
				&start, &stop, step);

		if (slicelength <= 0)
			return 0;

		self->m_Array->setRange((jsize) start, (jsize) stop, value);
		return 0;
	}
	JP_PY_CATCH(-1);
}

#ifdef __cplusplus
}
#endif