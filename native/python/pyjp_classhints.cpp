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
#include "jpype.h"
#include "pyjp.h"

#ifdef __cplusplus
extern "C"
{
#endif

PyObject *PyJPClassHints_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClassHints_new", type);
	PyJPClassHints *self = (PyJPClassHints*) type->tp_alloc(type, 0);
	self->m_Hints = new JPClassHints();
	return (PyObject*) self;
	JP_PY_CATCH(NULL);
}

int PyJPClassHints_init(PyJPClassHints *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClassHints_init", self);
	return 0;
	JP_PY_CATCH(-1);
}

void PyJPClassHints_dealloc(PyJPClassHints *self)
{
	JP_PY_TRY("PyJPClassHints_dealloc", self);
	delete self->m_Hints;

	// Free self
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH();
}

PyObject *PyJPClassHints_str(PyJPClassHints *self)
{
	JP_PY_TRY("PyJPClassHints_str", self);
	stringstream sout;
	sout << "<java class hints>";
	return JPPyString::fromStringUTF8(sout.str()).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPClassHints_addAttributeConversion(PyJPClassHints *self, PyObject* args, PyObject* kwargs)
{
	JP_PY_TRY("PyJPClassHints_addAttributeConversion", self);
	char* attribute;
	PyObject *method;
	if (!PyArg_ParseTuple(args, "sO", &attribute, &method))
		return NULL;
	JP_TRACE(attribute);
	JP_TRACE(Py_TYPE(method)->tp_name);
	if (!PyCallable_Check(method))
		JP_RAISE(PyExc_TypeError, "callable method is required");
	self->m_Hints->addAttributeConversion(string(attribute), method);
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPClassHints_addTypeConversion(PyJPClassHints *self, PyObject* args, PyObject* kwargs)
{
	JP_PY_TRY("PyJPClassHints_addTypeConversion", self);
	PyObject *type;
	PyObject *method;
	unsigned char exact;
	if (!PyArg_ParseTuple(args, "OOb", &type, &method, &exact))
		return NULL;
	if (!PyType_Check(type))
		JP_RAISE(PyExc_TypeError, "type is required");
	if (!PyCallable_Check(method))
		JP_RAISE(PyExc_TypeError, "callable method is required");
	self->m_Hints->addTypeConversion(type, method, exact != 0);
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

static PyMethodDef classMethods[] = {
	{"addAttributeConversion", (PyCFunction) & PyJPClassHints_addAttributeConversion, METH_VARARGS, ""},
	{"addTypeConversion", (PyCFunction) & PyJPClassHints_addTypeConversion, METH_VARARGS, ""},
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

PyTypeObject *PyJPClassHints_Type = NULL;
PyType_Spec PyJPClassHintsSpec = {
	"_jpype._JClassHints",
	sizeof (PyJPClassHints),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	hintsSlots
};

#ifdef __cplusplus
}
#endif

void PyJPClassHints_initType(PyObject* module)
{
	PyJPClassHints_Type = (PyTypeObject*) PyType_FromSpec(&PyJPClassHintsSpec);
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JClassHints", (PyObject*) PyJPClassHints_Type);
	JP_PY_CHECK();
}
