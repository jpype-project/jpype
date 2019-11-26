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

PyObject *PyJPMonitor_Type = NULL;
int PyJPMonitor_init(PyJPMonitor *self, PyObject *args);
void PyJPMonitor_dealloc(PyJPMonitor *o);
PyObject* PyJPMonitor_str(PyJPMonitor *o);
PyObject* PyJPMonitor_enter(PyJPMonitor *self, PyObject *args);
PyObject* PyJPMonitor_exit(PyJPMonitor *self, PyObject *args);
int PyJPMonitor_traverse(PyJPMonitor *self, visitproc visit, void *arg);
int PyJPMonitor_clear(PyJPMonitor *self);

static PyMethodDef monitorMethods[] = {
	{"__enter__", (PyCFunction) (&PyJPMonitor_enter), METH_NOARGS, ""},
	{"__exit__", (PyCFunction) (&PyJPMonitor_exit), METH_VARARGS, ""},
	{NULL},
};

static PyType_Slot monitorSlots[] = {
	{ Py_tp_init,     (initproc) PyJPMonitor_init},
	{ Py_tp_dealloc,  (destructor) PyJPMonitor_dealloc},
	{ Py_tp_traverse, (traverseproc) PyJPMonitor_traverse},
	{ Py_tp_clear,    (inquiry) PyJPMonitor_clear},
	{ Py_tp_str,      (reprfunc) PyJPMonitor_str},
	{ Py_tp_methods,  &monitorMethods},
	{0}
};

static PyType_Spec monitorSpec = {
	"_jpype.PyJPMonitor",
	sizeof (PyJPMonitor),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
	monitorSlots
};

// Static methods

void PyJPMonitor_initType(PyObject *module)
{
	PyModule_AddObject(module, "PyJPMonitor",
			PyJPMonitor_Type = PyType_FromSpec(&monitorSpec));
}

int PyJPMonitor_init(PyJPMonitor *self, PyObject *args)
{
	JP_TRACE_IN_C("PyJPMonitor::__init__");
	try
	{
		self->m_Monitor = NULL;

		PyObject *obj;
		if (!PyArg_ParseTuple(args, "O", &obj))
		{
			return -1;
		}

		PyJPValue *value = PyJPValue_asValue(obj);
		if (value == NULL)
			JP_RAISE_TYPE_ERROR("Must be a Java Object");

		JPValue& v1 = value->m_Value;
		JPContext *context = v1.getClass()->getContext();
		ASSERT_JVM_RUNNING(context);
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
		self->m_Context = (PyJPContext*) (context->getHost());
		Py_INCREF(self->m_Context);
		return 0;
	}
	PY_STANDARD_CATCH(-1);
	JP_TRACE_OUT_C;
}

void PyJPMonitor_dealloc(PyJPMonitor *self)
{
	JP_TRACE_IN_C("PyJPMonitor::__dealloc__");
	try
	{
		delete self->m_Monitor;
		PyObject_GC_UnTrack(self);
		PyJPMonitor_clear(self);
		Py_TYPE(self)->tp_free(self);
	}
	PY_STANDARD_CATCH();
	JP_TRACE_OUT_C;
}

int PyJPMonitor_traverse(PyJPMonitor *self, visitproc visit, void *arg)
{
	JP_TRACE_IN_C("PyJPMonitor::traverse");
	Py_VISIT(self->m_Context);
	return 0;
	JP_TRACE_OUT_C;
}

int PyJPMonitor_clear(PyJPMonitor *self)
{
	JP_TRACE_IN_C("PyJPMonitor::clear");
	Py_CLEAR(self->m_Context);
	return 0;
	JP_TRACE_OUT_C;
}

PyObject *PyJPMonitor_str(PyJPMonitor *self)
{
	JP_TRACE_IN_C("PyJPMonitor::__str__");
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		stringstream ss;
		ss << "<java monitor>";
		return JPPyString::fromStringUTF8(ss.str()).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPMonitor_enter(PyJPMonitor *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		self->m_Monitor->enter();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPMonitor_exit(PyJPMonitor *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		self->m_Monitor->exit();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH(NULL);
}
