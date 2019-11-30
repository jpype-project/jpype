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

#ifdef __cplusplus
extern "C"
{
#endif

PyObject *PyJPClassHints_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int PyJPClassHints_init(PyJPClassHints *self, PyObject *args, PyObject *kwargs);
void PyJPClassHints_dealloc(PyJPClassHints *self);
PyObject *PyJPClassHints_str(PyJPClassHints *self);

static PyMethodDef classMethods[] = {
	{NULL},
};

static PyType_Slot hintsSlots[] = {
	{ Py_tp_new ,    (void*) PyJPClassHints_new},
	{ Py_tp_init,    (void*) PyJPClassHints_init},
	{ Py_tp_dealloc, (void*) PyJPClassHints_dealloc},
	{ Py_tp_str,     (void*) PyJPClassHints_str},
	{ Py_tp_doc,     (void*) "Java Class Hints"},
	{ Py_tp_methods, (void*) classMethods},
	{0}
};

PyType_Spec PyJPClassHintsSpec = {
	"_jpype.PyJPClassHints",
	sizeof (PyJPClassHints),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	hintsSlots
};

PyObject *PyJPClassHints_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	PyJPClassHints *self = (PyJPClassHints*) type->tp_alloc(type, 0);
	self->m_Hints = NULL;
	return (PyObject*) self;
}

int PyJPClassHints_init(PyJPClassHints *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClassHints_init", self);
	// Parse arguments
	PyObject *target;
	PyObject *pyintf;
	if (!PyArg_ParseTuple(args, "OO", &target, &pyintf))
		return -1;

	return 0;
	JP_PY_CATCH(-1);
}

void PyJPClassHints_dealloc(PyJPClassHints *self)
{
	JP_PY_TRY("PyJPClassHints_dealloc", self)
			delete self->m_Hints;

	// Free self
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH();
}

PyObject *PyJPClassHints_str(PyJPClassHints *self)
{
	JP_PY_TRY("PyJPClassHints_str", self)
	stringstream sout;
	sout << "<java class hints>";
	return JPPyString::fromStringUTF8(sout.str()).keep();
	JP_PY_CATCH(NULL);
}

#ifdef __cplusplus
}
#endif