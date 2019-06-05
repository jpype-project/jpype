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

static PyMethodDef methodMethods[] = {
	{"getName", (PyCFunction) (&PyJPMethod::getName), METH_NOARGS, ""},
	{"isBeanAccessor", (PyCFunction) (&PyJPMethod::isBeanAccessor), METH_NOARGS, ""},
	{"isBeanMutator", (PyCFunction) (&PyJPMethod::isBeanMutator), METH_NOARGS, ""},
	{"matchReport", (PyCFunction) (&PyJPMethod::matchReport), METH_VARARGS, ""},
	{"dump", (PyCFunction) (&PyJPMethod::dump), METH_NOARGS, ""},
	{NULL},
};

PyTypeObject PyJPMethod::Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	/* tp_name           */ "_jpype.PyJPMethod",
	/* tp_basicsize      */ sizeof (PyJPMethod),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPMethod::__dealloc__,
	/* tp_print          */ 0,
	/* tp_getattr        */ 0,
	/* tp_setattr        */ 0,
	/* tp_compare        */ 0,
	/* tp_repr           */ 0,
	/* tp_as_number      */ 0,
	/* tp_as_sequence    */ 0,
	/* tp_as_mapping     */ 0,
	/* tp_hash           */ 0,
	/* tp_call           */ (ternaryfunc) PyJPMethod::__call__,
	/* tp_str            */ (reprfunc) PyJPMethod::__str__,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT,
	/* tp_doc            */ "Java Method",
	/* tp_traverse       */ 0,
	/* tp_clear          */ 0,
	/* tp_richcompare    */ 0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter           */ 0,
	/* tp_iternext       */ 0,
	/* tp_methods        */ methodMethods,
	/* tp_members        */ 0,
	/* tp_getset         */ 0,
	/* tp_base           */ 0,
	/* tp_dict           */ 0,
	/* tp_descr_get      */ (descrgetfunc) PyJPMethod::__get__,
	/* tp_descr_set      */ 0,
	/* tp_dictoffset     */ 0,
	/* tp_init           */ 0,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPMethod::__new__
};

// Static methods

void PyJPMethod::initType(PyObject* module)
{
	PyType_Ready(&PyJPMethod::Type);
	Py_INCREF(&PyJPMethod::Type);
	PyModule_AddObject(module, "PyJPMethod", (PyObject*) (&PyJPMethod::Type));
}

JPPyObject PyJPMethod::alloc(JPMethodDispatch* m, PyObject* instance)
{
	JP_TRACE_IN("PyJPMethod::alloc");
	PyJPMethod* self = PyObject_New(PyJPMethod, &PyJPMethod::Type);
	JP_PY_CHECK();
	self->m_Method = m;
	self->m_Instance = instance;
	if (instance != NULL)
	{
		JP_TRACE_PY("method alloc (inc)", instance);
		Py_INCREF(instance);
	}
	self->m_Context = (PyJPContext*) (m->getContext()->getHost());
	Py_INCREF(self->m_Context);
	return JPPyObject(JPPyRef::_claim, (PyObject*) self);
	JP_TRACE_OUT;
}

PyObject* PyJPMethod::__new__(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
	PyJPMethod* self = (PyJPMethod*) type->tp_alloc(type, 0);
	self->m_Method = 0;
	self->m_Instance = 0;
	self->m_Context = 0;
	return (PyObject*) self;
}

PyObject* PyJPMethod::__get__(PyJPMethod* self, PyObject* obj, PyObject* type)
{
	JP_TRACE_IN("PyJPMethod::__get__");
	try
	{
		JPContext *context = self->m_Method->getContext();
		ASSERT_JVM_RUNNING(context, "PyJPMethod::__get__");
		if (obj == NULL)
		{
			Py_INCREF((PyObject*) self);
			JP_TRACE_PY("method get (inc)", (PyObject*) self);
			return (PyObject*) self;
		}
		return PyJPMethod::alloc(self->m_Method, obj).keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPMethod::__call__(PyJPMethod* self, PyObject* args, PyObject* kwargs)
{
	JP_TRACE_IN("PyJPMethod::__call__");
	try
	{
		JPContext *context = self->m_Method->getContext();
		ASSERT_JVM_RUNNING(context, "PyJPMethod::__call__");
		JPJavaFrame frame(context);
		JP_TRACE(self->m_Method->getName());
		if (self->m_Instance == NULL)
		{
			JPPyObjectVector vargs(args);
			return self->m_Method->invoke(vargs, false).keep();
		}
		else
		{
			JPPyObjectVector vargs(self->m_Instance, args);
			return self->m_Method->invoke(vargs, true).keep();
		}
	}
	PY_STANDARD_CATCH;

	return NULL;
	JP_TRACE_OUT;
}

void PyJPMethod::__dealloc__(PyJPMethod* self)
{
	if (self->m_Instance != NULL)
	{
		JP_TRACE_PY("method dealloc (dec)", self->m_Instance);
		Py_DECREF(self->m_Instance);
	}
	self->m_Instance = NULL;
	self->m_Method = NULL;
	if (self->m_Context != NULL)
		Py_DECREF(self->m_Context);
	self->m_Context = NULL;
	Py_TYPE(self)->tp_free(self);
}

PyObject* PyJPMethod::__str__(PyJPMethod* self)
{
	try
	{
		JPContext *context = self->m_Method->getContext();
		ASSERT_JVM_RUNNING(context, "PyJPMethod::__str__");
		stringstream sout;
		if (self->m_Instance == NULL)
			sout << "<java method ";
		else
			sout << "<java bound method ";
		sout << self->m_Method->getClassName() << "." << self->m_Method->getName() << ">";
		return JPPyString::fromStringUTF8(sout.str()).keep();
	}
	PY_STANDARD_CATCH;

	return NULL;
}

PyObject* PyJPMethod::isBeanAccessor(PyJPMethod* self, PyObject* arg)
{
	try
	{
		JPContext *context = self->m_Method->getContext();
		ASSERT_JVM_RUNNING(context, "PyJPMethod::isBeanAccessor");
		return PyBool_FromLong(self->m_Method->isBeanAccessor());
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPMethod::isBeanMutator(PyJPMethod* self, PyObject* arg)
{
	try
	{
		JPContext *context = self->m_Method->getContext();
		ASSERT_JVM_RUNNING(context, "PyJPMethod::isBeanMutator");
		return PyBool_FromLong(self->m_Method->isBeanMutator());
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPMethod::getName(PyJPMethod* self, PyObject* arg)
{
	try
	{
		JPContext *context = self->m_Method->getContext();
		ASSERT_JVM_RUNNING(context, "PyJPMethod::getName");
		string name = self->m_Method->getName();
		return JPPyString::fromStringUTF8(name).keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPMethod::matchReport(PyJPMethod* self, PyObject* args)
{
	try
	{
		JPContext *context = self->m_Method->getContext();
		ASSERT_JVM_RUNNING(context, "PyJPMethod::matchReport");
		JPJavaFrame frame(context);
		JPPyObjectVector vargs(args);
		string report = self->m_Method->matchReport(vargs);
		return JPPyString::fromStringUTF8(report).keep();
	}
	PY_STANDARD_CATCH;

	return NULL;
}

PyObject* PyJPMethod::dump(PyJPMethod* self, PyObject* args)
{
	try
	{
		JPContext *context = self->m_Method->getContext();
		ASSERT_JVM_RUNNING(context, "PyJPMethod::matchReport");
		JPJavaFrame frame(context);
		string report = self->m_Method->dump();
		return JPPyString::fromStringUTF8(report).keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
}
