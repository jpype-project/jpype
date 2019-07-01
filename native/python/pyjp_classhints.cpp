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
#include <pyjp_classhints.h>

	static PyMethodDef classMethods[] = {
	{NULL},
};

PyTypeObject PyJPClassHints::Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	/* tp_name           */ "PyJPClassHints",
	/* tp_basicsize      */ sizeof (PyJPClassHints),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPClassHints::__dealloc__,
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
	/* tp_str            */ (reprfunc) PyJPClassHints::__str__,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
	/* tp_doc            */ "Java Class Hints",
	/* tp_traverse       */ (traverseproc) PyJPClassHints::traverse,
	/* tp_clear          */ (inquiry) PyJPClassHints::clear,
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
	/* tp_init           */ (initproc) PyJPClassHints::__init__,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPClassHints::__new__
};

// Static methods

void PyJPClassHints::initType(PyObject *module)
{
	PyType_Ready(&PyJPClassHints::Type);
	Py_INCREF(&PyJPClassHints::Type);
	PyModule_AddObject(module, "PyJPClassHints", (PyObject*) (&PyJPClassHints::Type));
}

PyObject *PyJPClassHints::__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_TRACE_IN("PyJPProxy::new");
	PyJPProxy* self = (PyJPProxy*) type->tp_alloc(type, 0);
	self->m_Proxy = NULL;
	self->m_Target = NULL;
	self->m_Context = NULL;
	return (PyObject*) self;
	JP_TRACE_OUT;
}

int PyJPClassHints::__init__(PyJPClassHints *self, PyObject *args, PyObject *kwargs)
{
	JP_TRACE_IN_C("PyJPProxy::init", self);
	try
	{
		// Parse arguments
		PyObject *target;
		PyObject *pyintf;
		if (!PyArg_ParseTuple(args, "OO", &target, &pyintf))
		{
			return -1;
		}

		//		// Pack interfaces
		//		if (!JPPySequence::check(pyintf))
		//		{
		//			PyErr_SetString(PyExc_TypeError, "third argument must be a list of interface");
		//			return -1;
		//		}
		//
		//		JPClassList interfaces;
		//		JPPySequence intf(JPPyRef::_use, pyintf);
		//		jlong len = intf.size();
		//		if (len < 1)
		//		{
		//			PyErr_SetString(PyExc_TypeError, "at least one interface is required");
		//			return -1;
		//		}
		//
		//		for (jlong i = 0; i < len; i++)
		//		{
		//			JPClass *cls = JPPythonEnv::getJavaClass(intf[i].get());
		//			if (cls == NULL)
		//			{
		//				PyErr_SetString(PyExc_TypeError, "interfaces must be object class instances");
		//				return -1;
		//			}
		//			interfaces.push_back(cls);
		//		}
		//
		//		JPContext *context = interfaces[0]->getContext();
		//		ASSERT_JVM_RUNNING(context);
		//
		//		// FIXME if we have multiple context someone has to check that all the interfaces
		//		// belong to the same context.
		//
		//		JPJavaFrame frame(context);
		//
		//		Py_INCREF(target);
		//		self->m_Target = target;
		//		self->m_Proxy = context->getProxyFactory()->newProxy(target, interfaces);
		//		self->m_Context = (PyJPContext*) (context->getHost());
		//		Py_INCREF(self->m_Context);

		JP_TRACE("Proxy", self);
		JP_TRACE("Target", target);
		return 0;
	}
	PY_STANDARD_CATCH(-1);
	JP_TRACE_OUT_C;
}

void PyJPClassHints::__dealloc__(PyJPProxy *self)
{
	JP_TRACE_IN_C("PyJPProxy::dealloc", self);
	delete self->m_Proxy;

	PyObject_GC_UnTrack(self);
	clear(self);
	// Free self
	Py_TYPE(self)->tp_free(self);
	JP_TRACE_OUT_C;
}

int PyJPClassHints::traverse(PyJPProxy *self, visitproc visit, void *arg)
{
	JP_TRACE_IN_C("PyJPProxy::traverse", self);
	Py_VISIT(self->m_Target);
	Py_VISIT(self->m_Context);
	return 0;
	JP_TRACE_OUT_C;
}

int PyJPClassHints::clear(PyJPProxy *self)
{
	JP_TRACE_IN_C("PyJPProxy::clear", self);
	Py_CLEAR(self->m_Target);
	Py_CLEAR(self->m_Context);
	return 0;
	JP_TRACE_OUT_C;
}

PyObject *PyJPClassHints::__str__(PyJPProxy *self)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		stringstream sout;
		sout << "<java proxy>";
		return JPPyString::fromStringUTF8(sout.str()).keep();
	}
	PY_STANDARD_CATCH(NULL);
}

bool PyJPClassHints::check(PyObject *o)
{
	return Py_TYPE(o) == &PyJPClassHints::Type;
}
