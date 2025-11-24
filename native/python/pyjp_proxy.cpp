/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
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
	auto *self = (PyJPProxy*) type->tp_alloc(type, 0);
	JP_PY_CHECK();

	// Parse arguments
	PyObject *instance;
	PyObject *dispatch;
	PyObject *pyintf;
	int convert = 0;
	if (!PyArg_ParseTuple(args, "OOO|p", &instance, &dispatch, &pyintf, &convert))
		return nullptr;

	// Pack interfaces
	if (!PySequence_Check(pyintf))
	{
		PyErr_SetString(PyExc_TypeError, "third argument must be a list of interface");
		return nullptr;
	}

	JPJavaFrame frame = JPJavaFrame::outer();
	JPClassList interfaces;
	JPPySequence intf = JPPySequence::use(pyintf);
	jlong len = intf.size();
	if (len < 1)
		JP_RAISE(PyExc_TypeError, "at least one interface is required");

	for (jlong i = 0; i < len; i++)
	{
		JPClass *cls = PyJPClass_getJPClass(intf[i].get());
		if (cls == nullptr)
		{
			PyErr_SetString(PyExc_TypeError, "interfaces must be object class instances");
			return nullptr;
		}
		interfaces.push_back(cls);
	}

	if (dispatch == Py_None)
		self->m_Proxy = new JPProxyDirect(self, interfaces);
	else
		self->m_Proxy = new JPProxyIndirect(self, interfaces);
	self->m_Target = instance;
	self->m_Dispatch = dispatch;
	self->m_Convert = (convert != 0);
	Py_INCREF(self->m_Target);
	Py_INCREF(self->m_Dispatch);

	JP_TRACE("Proxy", self);
	JP_TRACE("Target", instance);
	JP_TRACE("Dispatch", dispatch);
	return (PyObject*) self;
	JP_PY_CATCH(nullptr);
}

static int PyJPProxy_traverse(PyJPProxy *self, visitproc visit, void *arg)
{
	Py_VISIT(self->m_Target);
	Py_VISIT(self->m_Dispatch);
	return 0;
}

static int PyJPProxy_clear(PyJPProxy *self)
{
	Py_CLEAR(self->m_Target);
	Py_CLEAR(self->m_Dispatch);
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
	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass* cls = self->m_Proxy->getInterfaces()[0];
	return PyJPClass_create(frame, cls).keep();
}

static PyObject *PyJPProxy_inst(PyJPProxy *self, void *context)
{
	PyObject *out = self->m_Dispatch;
	if (out == Py_None)
		out = (PyObject*) self;
	Py_INCREF(out);
	return out;
}

static PyObject *PyJPProxy_equals(PyJPProxy *self, PyObject *other)
{
	return PyObject_RichCompare((PyObject*) self, other, Py_EQ);
}

static PyObject *PyJPProxy_hash(PyJPProxy *self)
{
	if (self->m_Target != Py_None)
		return PyLong_FromLong((int) PyObject_Hash(self->m_Target));
	return PyLong_FromLong((int) PyObject_Hash((PyObject*) self));
}

static PyObject *PyJPProxy_toString(PyJPProxy *self)
{
	if (self->m_Target != Py_None)
		return PyObject_Str(self->m_Target);
	return PyObject_Str((PyObject*) self);
}

static PyMethodDef proxyMethods[] = {
	{"equals", (PyCFunction) (&PyJPProxy_equals), METH_O, ""},
	{"hashCode", (PyCFunction) (&PyJPProxy_hash), METH_NOARGS, ""},
	{"toString", (PyCFunction) (&PyJPProxy_toString), METH_NOARGS, ""},
	{nullptr},
};

static PyGetSetDef proxyGetSets[] = {
	{"__javainst__", (getter) PyJPProxy_inst, nullptr, ""},
	{"__javaclass__", (getter) PyJPProxy_class, nullptr, ""},
	{nullptr}
};

static PyType_Slot proxySlots[] = {
	{ Py_tp_new,      (void*) PyJPProxy_new},
	{ Py_tp_dealloc,  (void*) PyJPProxy_dealloc},
	{ Py_tp_traverse, (void*) PyJPProxy_traverse},
	{ Py_tp_clear,    (void*) PyJPProxy_clear},
	{ Py_tp_getset,   (void*) proxyGetSets},
	{ Py_tp_methods,  (void*) proxyMethods},
	{0}
};

PyTypeObject *PyJPProxy_Type = nullptr;
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
	JPPyObject bases = JPPyTuple_Pack(&PyBaseObject_Type);
	PyJPProxy_Type = (PyTypeObject*) PyType_FromSpecWithBases(&PyJPProxySpec, bases.get());
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JProxy", (PyObject*) PyJPProxy_Type);
	JP_PY_CHECK();
}

JPProxy *PyJPProxy_getJPProxy(PyObject* obj)
{
	if (PyObject_IsInstance(obj, (PyObject*) PyJPProxy_Type))
		return ((PyJPProxy*) obj)->m_Proxy;
	return nullptr;
}
