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
#include <pyjp.h>

#ifdef __cplusplus
extern "C"
{
#endif

int PyJPMonitor_init(PyJPMonitor *self, PyObject *args)
{
	JP_PY_TRY("PyJPMonitor_init");
	self->m_Monitor = NULL;

	PyObject *obj;
	if (!PyArg_ParseTuple(args, "O", &obj))
		return -1;

	PyJPValue *value = PyJPValue_asValue(obj);
	if (value == NULL)
		JP_RAISE_TYPE_ERROR("Must be a Java Object");

	JPValue& v1 = value->m_Value;
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);

	// FIXME should these be runtime or type error.
	// it is legitimately the wrong "type" of object.
	if (v1.getClass() == context->_java_lang_String)
	{
		PyErr_SetString(PyExc_TypeError, "Strings cannot be used to synchronize.");
		return -1;
	}

	if ((v1.getClass())->isPrimitive())
	{
		PyErr_SetString(PyExc_TypeError, "Primitives cannot be used to synchronize.");
		return -1;
	}

	if (v1.getValue().l == NULL)
	{
		PyErr_SetString(PyExc_TypeError, "Null cannot be used to synchronize.");
		return -1;
	}

	self->m_Monitor = new JPMonitor(context, v1.getValue().l);
	return 0;
	JP_PY_CATCH(-1);
}

void PyJPMonitor_dealloc(PyJPMonitor *self)
{
	JP_PY_TRY("PyJPMonitor_dealloc");
	delete self->m_Monitor;
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH();
}

PyObject *PyJPMonitor_str(PyJPMonitor *self)
{
	JP_PY_TRY("PyJPMonitor_str");
	PyJPModule_getContext();
	stringstream ss;
	ss << "<java monitor>";
	return JPPyString::fromStringUTF8(ss.str()).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMonitor_enter(PyJPMonitor *self, PyObject *args)
{
	JP_PY_TRY("PyJPMonitor_enter", self);
	PyJPModule_getContext();
	self->m_Monitor->enter();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMonitor_exit(PyJPMonitor *self, PyObject *args)
{
	JP_PY_TRY("PyJPMonitor_exit", self);
	PyJPModule_getContext();
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
	"_jpype.PyJPMonitor",
	sizeof (PyJPMonitor),
	0,
	Py_TPFLAGS_DEFAULT,
	monitorSlots
};

#ifdef __cplusplus
}
#endif
