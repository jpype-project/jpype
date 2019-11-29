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
#include <structmember.h>

#ifdef __cplusplus
extern "C"
{
#endif

PyObject *PyJPProxy_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int PyJPProxy_init(PyJPProxy *self, PyObject *args, PyObject *kwargs);
void PyJPProxy_dealloc(PyJPProxy *self);
int PyJPProxy_traverse(PyJPProxy *self, visitproc visit, void *arg);
int PyJPProxy_clear(PyJPProxy *self);
PyObject *PyJPProxy_str(PyJPProxy *self);

static PyMemberDef proxyMembers[] = {
	{"__jvm__", T_OBJECT, offsetof(PyJPProxy, m_Context), READONLY},
	{0}
};

static PyType_Slot proxySlots[] = {
	{ Py_tp_new,      PyJPProxy_new},
	{ Py_tp_init,     (initproc) PyJPProxy_init},
	{ Py_tp_dealloc,  (destructor) PyJPProxy_dealloc},
	{ Py_tp_str,      (reprfunc) PyJPProxy_str},
	{ Py_tp_traverse, (traverseproc) PyJPProxy_traverse},
	{ Py_tp_clear,    (inquiry) PyJPProxy_clear},
	{ Py_tp_members,  proxyMembers},
	{0}
};

PyType_Spec PyJPProxySpec = {
	"_jpype.PyJPProxy",
	sizeof (PyObject),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
	proxySlots
};

PyObject *PyJPProxy_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	try
	{
		JP_TRACE_IN_C("PyJPProxy_new");
		PyJPProxy *self = (PyJPProxy*) type->tp_alloc(type, 0);
		self->m_Proxy = NULL;
		self->m_Target = NULL;
		self->m_Context = NULL;
		return (PyObject*) self;
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

int PyJPProxy_init(PyJPProxy *self, PyObject *args, PyObject *kwargs)
{
	try
	{
		JP_TRACE_IN_C("PyJPProxy_init", self);
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
		ASSERT_JVM_RUNNING(context);

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
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(-1);
}

void PyJPProxy_dealloc(PyJPProxy *self)
{
	try
	{
		JP_TRACE_IN_C("PyJPProxy_dealloc", self);
		delete self->m_Proxy;

		PyObject_GC_UnTrack(self);
		PyJPProxy_clear(self);
		// Free self
		Py_TYPE(self)->tp_free(self);
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH();
}

int PyJPProxy_traverse(PyJPProxy *self, visitproc visit, void *arg)
{
	Py_VISIT(self->m_Target);
	Py_VISIT(self->m_Context);
	return 0;
}

int PyJPProxy_clear(PyJPProxy *self)
{
	Py_CLEAR(self->m_Target);
	Py_CLEAR(self->m_Context);
	return 0;
}

PyObject *PyJPProxy_str(PyJPProxy *self)
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

#ifdef __cplusplus
}
#endif
