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

int PyJPArray_init(PyObject *self, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPArray_init");
	PyJPModuleState *state = PyJPModuleState_global;

	// First call the base type that will construct the object
	if (((PyTypeObject*) state->PyJPValue_Type)->tp_init(self, pyargs, kwargs) == -1)
		return -1;

	// Next hook up the value to the array wrapper
	JPValue& value = ((PyJPValue*) self)->m_Value;
	((PyJPArray*) self)->m_Array = new JPArray(value.getClass(), (jarray) value.getJavaObject());
	return 0;
	JP_PY_CATCH(-1);
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
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	if (self->m_Array == NULL)
		JP_RAISE_RUNTIME_ERROR("Null array");
	stringstream sout;

	// FIXME way too hard to get this type name.
	sout << "<java array " << self->m_Array->getClass()->toString() << ">";
	return JPPyString::fromStringUTF8(sout.str()).keep();
	JP_PY_CATCH(0);
}

Py_ssize_t PyJPArray_len(PyJPArray *self)
{
	JP_PY_TRY("PyJPArray_len");
	PyJPModule_getContext();
	if (self->m_Array == NULL)
		JP_RAISE_RUNTIME_ERROR("Null array");
	return self->m_Array->getLength();
	JP_PY_CATCH(-1);
}

PyObject *PyJPArray_getArrayItem(PyJPArray *self, PyObject *arg)
{
	JP_PY_TRY("PyJPArray_getArrayItem");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	if (self->m_Array == NULL)
		JP_RAISE_RUNTIME_ERROR("Null array");
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

int PyJPArray_assignItem(PyJPArray *self, Py_ssize_t item, PyObject* value)
{
	JP_PY_TRY("PyJPArray_assignItem");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	if (item >= self->m_Array->getLength())
		JP_RAISE_INDEX_ERROR("Index out of range");
	self->m_Array->setItem((jsize) item, value);
	return 0;
	JP_PY_CATCH(-1);
}

int PyJPArray_assignSubscript(PyJPArray *self, PyObject *item, PyObject* value)
{
	JP_PY_TRY("PyJPArray_setArrayItem");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	if ( value == NULL)
		JP_RAISE_VALUE_ERROR("item deletion not supported");

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

static PyMethodDef arrayMethods[] = {
	{"__getitem__", (PyCFunction) (&PyJPArray_getArrayItem), METH_O | METH_COEXIST, ""},
	{NULL},
};

static PyType_Slot arraySlots[] = {
	{ Py_tp_new,      (void*) PyJPArray_new},
	{ Py_tp_init,     (void*) PyJPArray_init},
	{ Py_tp_dealloc,  (void*) PyJPArray_dealloc},
	{ Py_tp_repr,     (void*) PyJPArray_repr},
	{ Py_tp_methods,  (void*) &arrayMethods},
	{ Py_sq_item,     (void*) &PyJPArray_getArrayItem},
	{ Py_sq_ass_item, (void*) &PyJPArray_assignItem},
	{ Py_mp_ass_subscript, (void*) &PyJPArray_assignSubscript},
	{ Py_sq_length,   (void*) &PyJPArray_len},
	{0}
};

PyType_Spec PyJPArraySpec = {
	"_jpype.PyJPArray",
	sizeof (PyJPArray),
	0,
	Py_TPFLAGS_DEFAULT  | Py_TPFLAGS_BASETYPE, //| Py_TPFLAGS_HAVE_GC,
	arraySlots
};

#ifdef __cplusplus
}
#endif

JPPyObject PyJPArray_create(PyTypeObject *wrapper, JPContext *context, const JPValue& value)
{
	JP_TRACE_IN("PyJPArray_alloc");
	JPPyObject self = PyJPValue_createInstance(wrapper, context, value);
	((PyJPArray*) self.get())->m_Array
			= new JPArray(value.getClass(), (jarray) value.getValue().l);
	return self;
	JP_TRACE_OUT;
}
