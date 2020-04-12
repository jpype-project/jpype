/*****************************************************************************
   Copyright 2004 Steve Ménard

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
#include "jp_monitor.h"
#include "jp_stringtype.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct PyJPMonitor
{
	PyObject_HEAD
	JPMonitor *m_Monitor;
} ;

static int PyJPMonitor_init(PyJPMonitor *self, PyObject *args)
{
	JP_PY_TRY("PyJPMonitor_init");
	self->m_Monitor = NULL;
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);

	PyObject* value;

	if (!PyArg_ParseTuple(args, "O", &value))
	{
		return -1;
	}

	JPValue *v1 = PyJPValue_getJavaSlot(value);
	if (v1 == NULL)
		JP_RAISE(PyExc_TypeError, "Java object is required.");

	if (v1->getClass() == context->_java_lang_String)
		JP_RAISE(PyExc_TypeError, "Java strings cannot be used to synchronize.");

	if ((v1->getClass())->isPrimitive())
		JP_RAISE(PyExc_TypeError, "Java primitives cannot be used to synchronize.");

	if (v1->getValue().l == NULL)
		JP_RAISE(PyExc_TypeError, "Java null cannot be used to synchronize.");

	self->m_Monitor = new JPMonitor(context, v1->getValue().l);
	return 0;
	JP_PY_CATCH(-1);
}

static void PyJPMonitor_dealloc(PyJPMonitor *self)
{
	JP_PY_TRY("PyJPMonitor_dealloc");
	delete self->m_Monitor;
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH();
}

static PyObject *PyJPMonitor_str(PyJPMonitor *self)
{
	JP_PY_TRY("PyJPMonitor_str");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	stringstream ss;
	ss << "<java monitor>";
	return JPPyString::fromStringUTF8(ss.str()).keep();
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPMonitor_enter(PyJPMonitor *self, PyObject *args)
{
	JP_PY_TRY("PyJPMonitor_enter");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	self->m_Monitor->enter();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPMonitor_exit(PyJPMonitor *self, PyObject *args)
{
	JP_PY_TRY("PyJPMonitor_exit");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	self->m_Monitor->exit();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

static PyMethodDef monitorMethods[] = {
	{"__enter__", (PyCFunction) (&PyJPMonitor_enter), METH_NOARGS, ""},
	{"__exit__", (PyCFunction) (&PyJPMonitor_exit), METH_VARARGS, ""},
	{NULL},
};

static PyType_Slot monitorSlots[] = {
	{ Py_tp_init,     (void*) PyJPMonitor_init},
	{ Py_tp_dealloc,  (void*) PyJPMonitor_dealloc},
	{ Py_tp_str,      (void*) PyJPMonitor_str},
	{ Py_tp_methods,  (void*) &monitorMethods},
	{0}
};

PyType_Spec PyJPMonitorSpec = {
	"_jpype._JMonitor",
	sizeof (PyJPMonitor),
	0,
	Py_TPFLAGS_DEFAULT,
	monitorSlots
};

PyTypeObject* PyJPMonitor_Type = NULL;

#ifdef __cplusplus
}
#endif

void PyJPMonitor_initType(PyObject* module)
{
	PyJPMonitor_Type = (PyTypeObject*) PyType_FromSpec(&PyJPMonitorSpec);
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JMonitor", (PyObject*) PyJPMonitor_Type);
	JP_PY_CHECK();
}
