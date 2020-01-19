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

#ifdef __cplusplus
extern "C"
{
#endif

static PyObject *PyJPProxy_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPProxy_new");
	PyJPProxy* self = (PyJPProxy*) type->tp_alloc(type, 0);
	self->m_Proxy = NULL;
	self->m_Target = NULL;
	self->m_Callable = NULL;
	return (PyObject*) self;
	JP_PY_CATCH(NULL);
}

/**
 * Initialize a PyJPProxy.
 *
 * @param self is the proxy instance.
 * @param args is a tuple holding the target and a list of interfaces.
 * @param kwargs is not used.
 * @return a new proxy instance.
 */
static int PyJPProxy_init(PyJPProxy *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPProxy_init");
	ASSERT_JVM_RUNNING("PyJPProxy_init");
	JPJavaFrame frame;

	// Parse arguments
	PyObject *target;
	PyObject *callable;
	PyObject *pyintf;
	if (!PyArg_ParseTuple(args, "OOO", &target, &callable, &pyintf))
		return -1;

	// Pack interfaces
	if (!PySequence_Check(pyintf))
		JP_RAISE_TYPE_ERROR("third argument must be a list of interface");

	JPClass::ClassList interfaces;
	JPPySequence intf(JPPyRef::_use, pyintf);
	jlong len = intf.size();

	if (len < 1)
		JP_RAISE_TYPE_ERROR("at least one interface is required");

	for (jlong i = 0; i < len; i++)
	{
		JPClass* cls = JPPythonEnv::getJavaClass(intf[i].get());
		if (cls == NULL)
		{
			PyErr_SetString(PyExc_TypeError, "interfaces must be object class instances");
			return -1;
		}
		interfaces.push_back(cls);
	}

	self->m_Proxy = new JPProxy((PyObject*) self, interfaces);
	self->m_Target = target;
	Py_INCREF(target);
	self->m_Callable = callable;
	Py_INCREF(callable);

	JP_TRACE("Proxy", self);
	JP_TRACE("Target", target);
	JP_TRACE("Callable", callable);
	return 0;
	JP_PY_CATCH(-1);
}

static PyObject *PyJPProxy_str(PyJPProxy* self)
{
	return JPPyString::fromStringUTF8("<java proxy>").keep();
}

static int PyJPProxy_traverse(PyJPProxy *self, visitproc visit, void *arg)
{
	Py_VISIT(self->m_Target);
	Py_VISIT(self->m_Callable);
	return 0;
}

static int PyJPProxy_clear(PyJPProxy *self)
{
	Py_CLEAR(self->m_Target);
	Py_CLEAR(self->m_Callable);
	return 0;
}

void PyJPProxy_dealloc(PyJPProxy* self)
{
	JP_PY_TRY("PyJPProxy_dealloc");
	delete self->m_Proxy;
	PyObject_GC_UnTrack(self);
	PyJPProxy_clear(self);
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH();
}

static PyMethodDef classMethods[] = {
	{NULL},
};

PyTypeObject* PyJPProxy_Type = 0;
PyTypeObject _PyJPProxy_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	/* tp_name           */ "PyJPProxy",
	/* tp_basicsize      */ sizeof (PyJPProxy),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPProxy_dealloc,
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
	/* tp_str            */ (reprfunc) PyJPProxy_str,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
	/* tp_doc            */ "Java Proxy",
	/* tp_traverse       */ (traverseproc) PyJPProxy_traverse,
	/* tp_clear          */ (inquiry) PyJPProxy_clear,
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
	/* tp_init           */ (initproc) PyJPProxy_init,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPProxy_new
};

#ifdef __cplusplus
}
#endif

void PyJPProxy_initType(PyObject* module)
{
	PyType_Ready(&_PyJPProxy_Type);
	Py_INCREF(&_PyJPProxy_Type);
	PyJPProxy_Type = &_PyJPProxy_Type;
	PyModule_AddObject(module, "PyJPProxy", (PyObject*) (&_PyJPProxy_Type));
}
