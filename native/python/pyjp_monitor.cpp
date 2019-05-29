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

static PyMethodDef methods[] = {
	{"__enter__", (PyCFunction) (&PyJPMonitor::__enter__), METH_NOARGS, ""},
	{"__exit__", (PyCFunction) (&PyJPMonitor::__exit__), METH_VARARGS, ""},
	{NULL},
};

PyTypeObject PyJPMonitor::Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	/* tp_name           */ "_jpype.PyJPMonitor",
	/* tp_basicsize      */ sizeof (PyJPMonitor),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPMonitor::__dealloc__,
	/* tp_print          */ 0,
	/* tp_getattr        */ 0,
	/* tp_setattr        */ 0,
	/* tp_compare        */ 0,
	/* tp_repr           */ 0,
	/* tp_as_number      */ 0,
	/* tp_as_sequence    */ 0,
	/* tp_as_mapping     */ 0,
	/* tp_hash           */ 0,
	/* tp_call           */ 0,
	/* tp_str            */ (reprfunc) PyJPMonitor::__str__,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT,
	/* tp_doc            */ "Java Monitor",
	/* tp_traverse       */ 0,
	/* tp_clear          */ 0,
	/* tp_richcompare    */ 0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter           */ 0,
	/* tp_iternext       */ 0,
	/* tp_methods        */ methods,
	/* tp_members        */ 0,
	/* tp_getset         */ 0,
	/* tp_base           */ 0,
	/* tp_dict           */ 0,
	/* tp_descr_get      */ 0,
	/* tp_descr_set      */ 0,
	/* tp_dictoffset     */ 0,
	/* tp_init           */ (initproc) PyJPMonitor::__init__,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyType_GenericNew
};

// Static methods

void PyJPMonitor::initType(PyObject* module)
{
	PyType_Ready(&PyJPMonitor::Type);
	Py_INCREF(&PyJPMonitor::Type);
	PyModule_AddObject(module, "PyJPMonitor", (PyObject*) (&PyJPMonitor::Type));
}

int PyJPMonitor::__init__(PyJPMonitor* self, PyObject* args)
{
	JP_TRACE_IN("PyJPMonitor::__init__");
	try
	{
		self->m_Monitor = NULL;
		ASSERT_JVM_RUNNING("PyJPMonitor::__init__");
		JPJavaFrame frame;

		PyObject* value;

		if (!PyArg_ParseTuple(args, "O!", &PyJPValue::Type, &value))
		{
			return -1;
		}

		JPValue& v1 = ((PyJPValue*) value)->m_Value;

		// FIXME should these be runtime or type error.
		// it is legitimately the wrong "type" of object.
		if (v1.getClass() == JPTypeManager::_java_lang_String)
		{
			PyErr_SetString(PyExc_TypeError, "Strings cannot be used to synchronize.");
			return -1;
		}

		if (dynamic_cast<JPPrimitiveType*> (v1.getClass()) != 0)
		{
			PyErr_SetString(PyExc_TypeError, "Primitives cannot be used to synchronize.");
			return -1;
		}

		if (v1.getValue().l == NULL)
		{
			PyErr_SetString(PyExc_TypeError, "Null cannot be used to synchronize.");
			return -1;
		}

		self->m_Monitor = new JPMonitor(v1.getValue().l);
		return 0;
	}
	PY_STANDARD_CATCH;
	return -1;
	JP_TRACE_OUT;
}

void PyJPMonitor::__dealloc__(PyJPMonitor* self)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPMonitor::__dealloc__");
		JPJavaFrame frame(self->m_Monitor->getContext());
		delete self->m_Monitor;
		Py_TYPE(self)->tp_free(self);
	}
	PY_STANDARD_CATCH
}

PyObject* PyJPMonitor::__str__(PyJPMonitor* self)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPMonitor::__str__");
		stringstream ss;
		ss << "<java monitor>";
		return JPPyString::fromStringUTF8(ss.str()).keep();
	}
	PY_STANDARD_CATCH
	return NULL;
}

PyObject* PyJPMonitor::__enter__(PyJPMonitor* self, PyObject* args)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPMonitor::__enter__");
		self->m_Monitor->enter();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH
	return NULL;
}

PyObject* PyJPMonitor::__exit__(PyJPMonitor* self, PyObject* args)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPMonitor::__exit__");
		self->m_Monitor->exit();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH
	return NULL;
}
