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

#ifdef __cplusplus
extern "C"
{
#endif

static PyObject *PyJPObject_new(PyTypeObject *type, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPObject_init");
	ASSERT_JVM_RUNNING();
	PyObject *self = type->tp_alloc(type, 0);
	JP_PY_CHECK();
	JPJavaFrame frame;
	JPPyObjectVector args(pyargs);

	// Get the Java class from the type.
	JPClass *cls = PyJPClass_getJPClass((PyObject*) Py_TYPE(self));
	if (cls == NULL)
		JP_RAISE(PyExc_TypeError, "Java class type is incorrect");

	// Create an instance (this may fail)
	PyJPValue_assignJavaSlot(self, cls->newInstance(frame, args));
	return self;
	JP_PY_CATCH(NULL);
}

static PyType_Slot objectSlots[] = {
	{Py_tp_new,      (void*) &PyJPObject_new},
	{Py_tp_free,     (void*) &PyJPValue_free},
	{Py_tp_getattro, (void*) &PyJPValue_getattro},
	{Py_tp_setattro, (void*) &PyJPValue_setattro},
	{Py_tp_str,      (void*) &PyJPValue_str},
	{0}
};

PyTypeObject *PyJPObject_Type = NULL;
static PyType_Spec objectSpec = {
	"_jpype._JObject",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	objectSlots
};


PyTypeObject *PyJPException_Type = NULL;
static PyType_Slot excSlots[] = {
	{Py_tp_new,      (void*) &PyJPObject_new},
	{Py_tp_getattro, (void*) &PyJPValue_getattro},
	{Py_tp_setattro, (void*) &PyJPValue_setattro},
	{0}
};

static PyType_Spec excSpec = {
	"_jpype._JException",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	excSlots
};

#ifdef __cplusplus
}
#endif

void PyJPObject_initType(PyObject* module)
{
	PyObject *bases;
	PyJPObject_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&objectSpec, NULL);
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JObject", (PyObject*) PyJPObject_Type);
	JP_PY_CHECK();

	bases = PyTuple_Pack(2, PyExc_Exception, PyJPObject_Type);
	PyJPException_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&excSpec, bases);
	Py_DECREF(bases);
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JException", (PyObject*) PyJPException_Type);
	JP_PY_CHECK();
}
