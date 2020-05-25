/*
 * Copyright 2020 nelson85.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "jpype.h"
#include "pyjp.h"
#include <structmember.h>
#include "jp_boxedtype.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct PyJPChar2
{
	PyObject_HEAD
	int m_Value;
} ;

static PyObject *PyJPChar2_new(PyTypeObject *type, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPObject_new");
	// Get the Java class from the type.
	JPClass *cls = PyJPClass_getJPClass((PyObject*) type);
	if (cls == NULL)
		JP_RAISE(PyExc_TypeError, "Java class type is incorrect");

	JPContext *context = PyJPModule_getContext();

	// Create an instance (this may fail)
	JPJavaFrame frame(context);
	if (PyTuple_Size(pyargs) != 1)
	{
		PyErr_Format(PyExc_TypeError, "Java chars require one argument");
		return 0;
	}

	JPValue jv;
	PyObject *in = PyTuple_GetItem(pyargs, 0);
	if (PyIndex_Check(in))
	{
		JPPyObjectVector args(pyargs);
		jv = cls->newInstance(frame, args);
	} else if (PyUnicode_Check(in))
	{
		if (PyUnicode_GetLength(in) != 1)
		{
			PyErr_Format(PyExc_TypeError, "Java require index or str with length 1");
			return 0;
		}
		if (PyUnicode_READY(in) == -1)
			return 0;
		Py_UCS4 cv = PyUnicode_READ_CHAR(in, 0);
		JPPyObject v(JPPyRef::_call, PyLong_FromLong(cv));
		JPPyTuple args0 = JPPyTuple::newTuple(1);
		args0.setItem(0, v.get());
		JPPyObjectVector args(args0.get());
		jv = cls->newInstance(frame, args);
	} else if (PyFloat_Check(in))
	{
		JPPyObject v(JPPyRef::_call, PyNumber_Long(in));
		JPPyTuple args0 = JPPyTuple::newTuple(1);
		args0.setItem(0, v.get());
		JPPyObjectVector args(args0.get());
		jv = cls->newInstance(frame, args);
	} else
	{
		// This is not strictly true as we can cast a float to a char
		PyErr_Format(PyExc_TypeError, "Java require index or str with length 1");
		return 0;
	}

	PyObject *self = type->tp_alloc(type, 0);
	JP_PY_CHECK();
	PyJPValue_assignJavaSlot(frame, self, jv);
	JP_FAULT_RETURN("PyJPChar2_init.null", self);
	if (cls->isPrimitive())
		((PyJPChar2*) self)->m_Value = jv.getValue().c;
	else
	{
		JPPrimitiveType* pcls = ((JPBoxedType*) cls)->getPrimitive();
		if (jv.getValue().l == 0)
			((PyJPChar2*) self)->m_Value = 0;
		else
			((PyJPChar2*) self)->m_Value = pcls->getValueFromObject(jv).getValue().c;
	}
	return self;
	JP_PY_CATCH(NULL);
}

static PyMethodDef charMethods[] = {
	//	{"thing", (PyCFunction) PyJPMethod_matchReport, METH_VARARGS, ""},
	{NULL},
};

struct PyGetSetDef charGetSet[] = {
	//	{"thing", (getter) PyJPMethod_getSelf, NULL, NULL, NULL},
	{NULL},
};

static PyType_Slot charSlots[] = {
	{Py_tp_new,       (void*) PyJPChar2_new},
	{Py_tp_methods,   (void*) charMethods},
	{Py_tp_getset,    (void*) charGetSet},
	{0}
};

PyTypeObject *PyJPChar2_Type = NULL;
static PyType_Spec charSpec = {
	"_jpype._JChar2",
	sizeof (PyJPChar2),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // | Py_TPFLAGS_HAVE_GC,
	charSlots
};

#ifdef __cplusplus
}
#endif

void PyJPChar2_initType(PyObject* module)
{
	// We will inherit from str and JObject
	PyObject *bases;
	bases = PyTuple_Pack(1, PyJPObject_Type);
	PyJPChar2_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&charSpec, bases);
	Py_DECREF(bases);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JChar2", (PyObject*) PyJPChar2_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
}

