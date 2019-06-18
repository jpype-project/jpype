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

struct PyGetSetDef methodGetSet[] = {
	{"__doc__", (getter) (&PyJPMethod::__doc__), NULL, NULL, NULL},
	{"__annotations__", (getter) (&PyJPMethod::__annotations__), NULL, NULL, NULL},
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
	/* tp_repr           */ (reprfunc) PyJPMethod::__repr__,
	/* tp_as_number      */ 0,
	/* tp_as_sequence    */ 0,
	/* tp_as_mapping     */ 0,
	/* tp_hash           */ 0,
	/* tp_call           */ (ternaryfunc) PyJPMethod::__call__,
	/* tp_str            */ (reprfunc) PyJPMethod::__str__,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
	/* tp_doc            */ 0,
	/* tp_traverse       */ (traverseproc) PyJPMethod::traverse,
	/* tp_clear          */ (inquiry) PyJPMethod::clear,
	/* tp_richcompare    */ 0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter           */ 0,
	/* tp_iternext       */ 0,
	/* tp_methods        */ methodMethods,
	/* tp_members        */ 0,
	/* tp_getset         */ methodGetSet,
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

JPPyObject PyJPMethod::alloc(JPMethod* m, PyObject* instance)
{
	JP_TRACE_IN("PyJPMethod::alloc");
	PyJPMethod* self = (PyJPMethod*) PyJPMethod::Type.tp_alloc(&PyJPMethod::Type, 0);;
	JP_PY_CHECK();
	self->m_Method = m;
	self->m_Instance = instance;
	res->m_Annotations = NULL;
	Py_XINCREF(self->m_Instance);
	return JPPyObject(JPPyRef::_claim, (PyObject*) self);
	JP_TRACE_OUT;
}

PyObject* PyJPMethod::__new__(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
	PyJPMethod* self = (PyJPMethod*) type->tp_alloc(type, 0);
	self->m_Method = NULL;
	self->m_Instance = NULL;
	return (PyObject*) self;
}

PyObject* PyJPMethod::__get__(PyJPMethod* self, PyObject* obj, PyObject* type)
{
	JP_TRACE_IN("PyJPMethod::__get__");
	try
	{
		ASSERT_JVM_RUNNING("PyJPMethod::__get__");
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
		ASSERT_JVM_RUNNING("PyJPMethod::__call__");
		JPJavaFrame frame;
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
	PyObject_GC_UnTrack(self);
	clear(self);
	self->m_Method = NULL;
	Py_TYPE(self)->tp_free(self);
}

int PyJPMethod::traverse(PyJPMethod *self, visitproc visit, void *arg)
{
	Py_VISIT(self->m_Instance);
	Py_VISIT(self->m_Annotations);
	return 0;
}

int PyJPMethod::clear(PyJPMethod *self)
{
	Py_CLEAR(self->m_Instance);
	Py_CLEAR(self->m_Annotations);
	return 0;
}

PyObject* PyJPMethod::__str__(PyJPMethod* self)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPMethod::__str__");
		JPJavaFrame frame;
		stringstream sout;
		sout << self->m_Method->getClass()->getCanonicalName() << "." << self->m_Method->getName();
		return JPPyString::fromStringUTF8(sout.str()).keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPMethod::__repr__(PyJPMethod* self)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPMethod::__repr__");
		stringstream ss;
		if (self->m_Instance == NULL)
			ss << "<java method `";
		else
			ss << "<java bound method `";
		ss << self->m_Method->getName() << "' of '" <<
				self->m_Method->getClass()->getCanonicalName() << "'>";
		return JPPyString::fromStringUTF8(ss.str()).keep();
	}
	PY_STANDARD_CATCH;

	return NULL;
}

PyObject *PyJPMethod::__doc__(PyJPMethod *method, void *context)
{
	JP_TRACE_IN("PyJPMethod::__doc__");
	try
	{
		ASSERT_JVM_RUNNING("PyJPMethod::__doc__");
		return JPPythonEnv::getMethodDoc(method).keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
	JP_TRACE_OUT;
}

PyObject *PyJPMethod::__annotations__(PyJPMethod *method, void *context)
{
	JP_TRACE_IN("PyJPMethod::__annotations__");
	try
	{
		ASSERT_JVM_RUNNING("PyJPMethod::__annotations__");
		if (method->m_Annotations)
		{
			Py_INCREF(method->m_Annotations);
			return method->m_Annotations;
		}
		JPPyObject annotations(JPPythonEnv::getMethodAnnotations(method));
		method->m_Annotations = annotations.get();
		Py_XINCREF(method->m_Annotations);
		return annotations.keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPMethod::isBeanAccessor(PyJPMethod* self, PyObject* arg)
{
	JP_TRACE_IN("PyJPMethod::isBeanAccessor");
	try
	{
		ASSERT_JVM_RUNNING("PyJPMethod::isBeanAccessor");
		JPJavaFrame frame;
		return PyBool_FromLong(self->m_Method->isBeanAccessor());
	}
	PY_STANDARD_CATCH;

	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPMethod::isBeanMutator(PyJPMethod* self, PyObject* arg)
{
	JP_TRACE_IN("PyJPMethod::isBeanMutator");
	try
	{
		ASSERT_JVM_RUNNING("PyJPMethod::isBeanMutator");
		JPJavaFrame frame;
		return PyBool_FromLong(self->m_Method->isBeanMutator());
	}
	PY_STANDARD_CATCH;

	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPMethod::getName(PyJPMethod* self, PyObject* arg)
{
	JP_TRACE_IN("PyJPMethod::getName");
	try
	{
		ASSERT_JVM_RUNNING("PyJPMethod::getName");
		JPJavaFrame frame;
		string name = self->m_Method->getName();
		return JPPyString::fromStringUTF8(name).keep();
	}
	PY_STANDARD_CATCH;

	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPMethod::matchReport(PyJPMethod* self, PyObject* args)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPMethod::matchReport");
		JPJavaFrame frame;
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
		ASSERT_JVM_RUNNING("PyJPMethod::matchReport");
		JPJavaFrame frame;
		string report = self->m_Method->dump();
		return JPPyString::fromStringUTF8(report).keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
}
