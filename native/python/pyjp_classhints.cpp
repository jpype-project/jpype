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
#include <pyjp.h>

PyObject *PyJPClassHints_Type = NULL;
PyObject *PyJPClassHints_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int PyJPClassHints_init(PyJPClassHints *self, PyObject *args, PyObject *kwargs);
void PyJPClassHints_dealloc(PyJPClassHints *self);
PyObject *PyJPClassHints_str(PyJPClassHints *self);

static PyMethodDef classMethods[] = {
	{NULL},
};

static PyType_Slot hintsSlots[] = {
	{ Py_tp_new ,    PyJPClassHints_new},
	{ Py_tp_init,    (initproc) PyJPClassHints_init},
	{ Py_tp_dealloc, (destructor) PyJPClassHints_dealloc},
	{ Py_tp_str,     (reprfunc) PyJPClassHints_str},
	{ Py_tp_doc,     "Java Class Hints"},
	{ Py_tp_methods, classMethods},
	{0}
};

static PyType_Spec hintsSpec = {
	"_jpype.PyJPClassHints",
	sizeof (PyJPClassHints),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	hintsSlots
};

void PyJPClassHints_initType(PyObject *module)
{
	PyModule_AddObject(module, "PyJPClassHints",
			PyJPClassHints_Type = PyType_FromSpec(&hintsSpec));
}

PyObject *PyJPClassHints_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_TRACE_IN("PyJPProxy_new");
	PyJPClassHints *self = (PyJPClassHints*) type->tp_alloc(type, 0);
	self->m_Hints = NULL;
	return (PyObject*) self;
	JP_TRACE_OUT;
}

int PyJPClassHints_init(PyJPClassHints *self, PyObject *args, PyObject *kwargs)
{
	JP_TRACE_IN_C("PyJPClassHints::init", self);
	try
	{
		// Parse arguments
		PyObject *target;
		PyObject *pyintf;
		if (!PyArg_ParseTuple(args, "OO", &target, &pyintf))
		{
			return -1;
		}

		return 0;
	}
	PY_STANDARD_CATCH(-1);
	JP_TRACE_OUT_C;
}

void PyJPClassHints_dealloc(PyJPClassHints *self)
{
	JP_TRACE_IN_C("PyJPClassHints_dealloc", self);
	delete self->m_Hints;

	// Free self
	Py_TYPE(self)->tp_free(self);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClassHints_str(PyJPClassHints *self)
{
	try
	{
		stringstream sout;
		sout << "<java class hints>";
		return JPPyString::fromStringUTF8(sout.str()).keep();
	}
	PY_STANDARD_CATCH(NULL);
}
