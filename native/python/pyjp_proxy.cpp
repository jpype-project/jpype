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
#include <Python.h>
#include <structmember.h>
#include "jpype.h"
#include "pyjp.h"
#include "jp_proxy.h"


#ifdef __cplusplus
extern "C"
{
#endif

static PyObject *PyJPProxy_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPProxy_new");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	PyJPProxy *self = (PyJPProxy*) type->tp_alloc(type, 0);
	JP_PY_CHECK();

	// Parse arguments
	PyObject *target;
	PyObject *pyintf;
	if (!PyArg_ParseTuple(args, "OO", &target, &pyintf))
		return NULL;

	// Pack interfaces
	if (!PySequence_Check(pyintf))
		JP_RAISE(PyExc_TypeError, "third argument must be a list of interface");

	JPClassList interfaces;
	JPPySequence intf(JPPyRef::_use, pyintf);
	jlong len = intf.size();
	if (len < 1)
		JP_RAISE(PyExc_TypeError, "at least one interface is required");

	for (jlong i = 0; i < len; i++)
	{
		JPClass *cls = PyJPClass_getJPClass(intf[i].get());
		if (cls == NULL)
			JP_RAISE(PyExc_TypeError, "interfaces must be object class instances");
		interfaces.push_back(cls);
	}

	self->m_Proxy = context->getProxyFactory()->newProxy((PyObject*) self, interfaces);
	self->m_Target = target;
	Py_INCREF(target);

	JP_TRACE("Proxy", self);
	JP_TRACE("Target", target);
	return (PyObject*) self;
	JP_PY_CATCH(NULL);
}

static int PyJPProxy_traverse(PyJPProxy *self, visitproc visit, void *arg)
{
	Py_VISIT(self->m_Target);
	return 0;
}

static int PyJPProxy_clear(PyJPProxy *self)
{
	Py_CLEAR(self->m_Target);
	return 0;
}

void PyJPProxy_dealloc(PyJPProxy* self)
{
	JP_PY_TRY("PyJPProxy_dealloc");
	delete self->m_Proxy;
	PyObject_GC_UnTrack(self);
	PyJPProxy_clear(self);
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH_NONE();
}

static PyObject *PyJPProxy_class(PyJPProxy *self, void *context)
{
	JPJavaFrame frame(self->m_Proxy->getContext());
	JPClass* cls = self->m_Proxy->getInterfaces()[0];
	return PyJPClass_create(frame, cls).keep();
}

static PyGetSetDef proxyGetSets[] = {
	{"__javaclass__", (getter) PyJPProxy_class, NULL, ""},
	{0}
};

static PyType_Slot proxySlots[] = {
	{ Py_tp_new,      (void*) PyJPProxy_new},
	{ Py_tp_dealloc,  (void*) PyJPProxy_dealloc},
	{ Py_tp_traverse, (void*) PyJPProxy_traverse},
	{ Py_tp_clear,    (void*) PyJPProxy_clear},
	{ Py_tp_getset,   (void*) proxyGetSets},
	{0}
};

PyTypeObject *PyJPProxy_Type = NULL;
PyType_Spec PyJPProxySpec = {
	"_jpype._JProxy",
	sizeof (PyJPProxy),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	proxySlots
};

#ifdef __cplusplus
}
#endif

void PyJPProxy_initType(PyObject* module)
{
	JPPyTuple tuple = JPPyTuple::newTuple(1);
	tuple.setItem(0, (PyObject*) & PyBaseObject_Type);
	PyJPProxy_Type = (PyTypeObject*) PyType_FromSpecWithBases(&PyJPProxySpec, tuple.get());
	JP_PY_CHECK_INIT();
	PyModule_AddObject(module, "_JProxy", (PyObject*) PyJPProxy_Type);
	JP_PY_CHECK_INIT();
}

JPProxy *PyJPProxy_getJPProxy(PyObject* obj)
{
	if (PyObject_IsInstance(obj, (PyObject*) PyJPProxy_Type))
		return ((PyJPProxy*) obj)->m_Proxy;
	return NULL;
}

JPPyObject PyJPProxy_getCallable(PyObject *obj, const string& name)
{
	JP_TRACE_IN("PyJPProxy_getCallable");
	if (Py_TYPE(obj) != PyJPProxy_Type
			&& Py_TYPE(obj)->tp_base != PyJPProxy_Type)
		JP_RAISE(PyExc_TypeError, "Incorrect type passed to proxy lookup");
	PyJPProxy *proxy = (PyJPProxy*) obj;
	if (proxy->m_Target != Py_None)
		obj = proxy->m_Target;
	return JPPyObject(JPPyRef::_accept, PyObject_GetAttrString(obj, name.c_str()));
	JP_TRACE_OUT;
}
