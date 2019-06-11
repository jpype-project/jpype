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

static PyMethodDef classMethods[] = {
	{NULL},
};

PyTypeObject PyJPProxy::Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	/* tp_name           */ "PyJPProxy",
	/* tp_basicsize      */ sizeof (PyJPProxy),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPProxy::__dealloc__,
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
	/* tp_str            */ (reprfunc) PyJPProxy::__str__,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
	/* tp_doc            */ "Java Proxy",
	/* tp_traverse       */ (traverseproc) PyJPProxy::traverse,
	/* tp_clear          */ (inquiry) PyJPProxy::clear,
	/* tp_richcompare    */ 0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter           */ 0,
	/* tp_iternext       */ 0,
	/* tp_methods        */ classMethods,
	/* tp_members        */ 0,
	/* tp_getset         */ 0,
	/* tp_base           */ 0,
	/* tp_dict           */ 0,
	/* tp_descr_get      */ 0,
	/* tp_descr_set      */ 0,
	/* tp_dictoffset     */ 0,
	/* tp_init           */ (initproc) PyJPProxy::__init__,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPProxy::__new__
};

// Static methods

void PyJPProxy::initType(PyObject *module)
{
	PyType_Ready(&PyJPProxy::Type);
	Py_INCREF(&PyJPProxy::Type);
	PyModule_AddObject(module, "PyJPProxy", (PyObject*) (&PyJPProxy::Type));
}

PyObject *PyJPProxy::__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	PyJPProxy *self = (PyJPProxy*) type->tp_alloc(type, 0);
	self->m_Proxy = NULL;
	self->m_Target = NULL;
	self->m_Context = NULL;
	return (PyObject*) self;
}

int PyJPProxy::__init__(PyJPProxy *self, PyObject *args, PyObject *kwargs)
{
	JP_TRACE_IN_C("PyJPProxy::init");
	try
	{
		// Parse arguments
		PyObject *target;
		PyObject *pyintf;
		if (!PyArg_ParseTuple(args, "OO", &target, &pyintf))
		{
			return -1;
		}

		// Pack interfaces
		if (!JPPySequence::check(pyintf))
		{
			PyErr_SetString(PyExc_TypeError, "third argument must be a list of interface");
			return -1;
		}

		JPClassList interfaces;
		JPPySequence intf(JPPyRef::_use, pyintf);
		jlong len = intf.size();
		if (len < 1)
		{
			PyErr_SetString(PyExc_TypeError, "at least one interface is required");
			return -1;
		}

		for (jlong i = 0; i < len; i++)
		{
			JPClass *cls = JPPythonEnv::getJavaClass(intf[i].get());
			if (cls == NULL)
			{
				PyErr_SetString(PyExc_TypeError, "interfaces must be object class instances");
				return -1;
			}
			interfaces.push_back(cls);
		}

		JPContext *context = interfaces[0]->getContext();
		ASSERT_JVM_RUNNING(context, "PyJPProxy::__init__");

		// FIXME if we have multiple context someone has to check that all the interfaces
		// belong to the same context.

		JPJavaFrame frame(context);

		Py_INCREF(target);
		self->m_Target = target;
		self->m_Proxy = context->getProxyFactory()->newProxy(target, interfaces);
		self->m_Context = (PyJPContext*) (context->getHost());
		Py_INCREF(self->m_Context);

		JP_TRACE("Proxy", self);
		JP_TRACE("Target", target);
		return 0;
	}
	PY_STANDARD_CATCH;
	return -1;
	JP_TRACE_OUT_C;
}

PyObject *PyJPProxy::__str__(PyJPProxy *self)
{
	try
	{
		JPContext *context = self->m_Proxy->getContext();
		ASSERT_JVM_RUNNING(context, "PyJPProxy::__init__");
		JPJavaFrame frame(context);
		stringstream sout;
		sout << "<java proxy>";
		return JPPyString::fromStringUTF8(sout.str()).keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
}

void PyJPProxy::__dealloc__(PyJPProxy *self)
{
	JP_TRACE_IN_C("PyJPProxy::dealloc");
	delete self->m_Proxy;

	if (self->m_Target != NULL)
		Py_DECREF(self->m_Target);
	if (self->m_Context != NULL)
		Py_DECREF(self->m_Context);

	self->m_Target = NULL;
	// Free self
	Py_TYPE(self)->tp_free(self);
	JP_TRACE_OUT_C;
}

int PyJPProxy::traverse(PyJPProxy *self, visitproc visit, void *arg)
{
	Py_VISIT(self->m_Target);
	return 0;
}

int PyJPProxy::clear(PyJPProxy *self)
{
	Py_CLEAR(self->m_Target);
	return 0;
}

bool PyJPProxy::check(PyObject *o)
{
	return Py_TYPE(o) == &PyJPProxy::Type;
}
