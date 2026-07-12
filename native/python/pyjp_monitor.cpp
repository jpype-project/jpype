// --- file: python/pyjp_monitor.cpp ---
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
#include "jpype.h"
#include "pyjp.h"
#include "jp_monitor.h"
#include "jp_stringtype.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct PyJPMonitor
{
	PyObject_HEAD
	JPMonitor *m_Monitor;
	PyJPModuleState* m_State;
} ;

static int PyJPMonitor_init(PyJPMonitor *self, PyObject *args)
{
	JP_PY_TRY("PyJPMonitor_init");
	self->m_Monitor = nullptr;
	self->m_State = nullptr;

	PyObject* value;
	if (!PyArg_ParseTuple(args, "O", &value))
		return -1;

	JPValue *v1 = PyJPValue_getJavaSlot(value);
	if (v1 == nullptr)
	{
		PyErr_SetString(PyExc_TypeError, "Java object is required.");
		return -1;
	}

	// Safely extract the module state using the fixed mro[-2] pattern
	PyTypeObject* type = Py_TYPE(self);
    PyJPModuleState* st = nullptr;
    Py_ssize_t mro_size = PyTuple_GET_SIZE(type->tp_mro);
    PyTypeObject* target_type = (PyTypeObject*)PyTuple_GET_ITEM(type->tp_mro, mro_size - 2);
    if (target_type->tp_flags & Py_TPFLAGS_HEAPTYPE) 
       st = reinterpret_cast<PyJPModuleState*>(PyType_GetModuleState(target_type));
    if (st == nullptr)
    {
        PyErr_SetString(PyExc_RuntimeError, "JPype module state is not available from proxy MRO anchor");
        return -1;
    }
    self->m_State = st;
    JPJavaFrame frame = JPJavaFrame::outer(st->context);

	if (v1->getClass() == st->context->_java_lang_String)
	{
		PyErr_SetString(PyExc_TypeError, "Java strings cannot be used to synchronize.");
		return -1;
	}

	if ((v1->getClass())->isPrimitive())
	{
		PyErr_SetString(PyExc_TypeError, "Java primitives cannot be used to synchronize.");
		return -1;
	}

	if (v1->isJavaNull())
	{
		PyErr_SetString(PyExc_TypeError, "Java null cannot be used to synchronize.");
		return -1;
	}

	self->m_Monitor = new JPMonitor(frame, v1->getJavaObject(frame));
	return 0;
	JP_PY_CATCH(-1);
}

static void PyJPMonitor_dealloc(PyJPMonitor *self)
{
	JP_PY_TRY("PyJPMonitor_dealloc");
	delete self->m_Monitor;
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH(); // GCOVR_EXCL_LINE
}

static PyObject *PyJPMonitor_str(PyJPMonitor *self)
{
	JP_PY_TRY("PyJPMonitor_str");
	return PyUnicode_FromFormat("<java monitor>");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPMonitor_enter(PyJPMonitor *self, PyObject *args)
{
	JP_PY_TRY("PyJPMonitor_enter");
	JPJavaFrame frame = JPJavaFrame::outer(self->m_State->context);
	self->m_Monitor->enter();
	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPMonitor_exit(PyJPMonitor *self, PyObject *args)
{
	JP_PY_TRY("PyJPMonitor_exit");
	JPJavaFrame frame = JPJavaFrame::outer(self->m_State->context);
	self->m_Monitor->exit();
	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static PyMethodDef monitorMethods[] = {
	{"__enter__", (PyCFunction) (&PyJPMonitor_enter), METH_NOARGS, ""},
	{"__exit__", (PyCFunction) (&PyJPMonitor_exit), METH_VARARGS, ""},
	{nullptr},
};

static PyType_Slot monitorSlots[] = {
	{ Py_tp_init,	 (void*) PyJPMonitor_init},
	{ Py_tp_dealloc,  (void*) PyJPMonitor_dealloc},
	{ Py_tp_str,	  (void*) PyJPMonitor_str},
	{ Py_tp_methods,  (void*) &monitorMethods},
	{0}
};

PyType_Spec PyJPMonitorSpec = {
	"_jpype._JMonitor",
	sizeof (PyJPMonitor),
	0,
	Py_TPFLAGS_DEFAULT,
	monitorSlots
};

void PyJPMonitor_initType(PyObject* module, PyJPModuleState* st)
{
#if PY_VERSION_HEX >= 0x030A0000
    st->PyJPMonitor_Type = (PyTypeObject*) PyType_FromModuleAndSpec(module, &PyJPMonitorSpec, nullptr);
#else
    st->PyJPMonitor_Type = (PyTypeObject*) PyType_FromSpecWithBases(&PyJPMonitorSpec, nullptr);
#endif
    JP_PY_CHECK(); // GCOVR_EXCL_LINE
    Py_INCREF((PyObject*) st->PyJPMonitor_Type);
    PyModule_AddObject(module, "_JMonitor", (PyObject*) st->PyJPMonitor_Type);
    JP_PY_CHECK(); // GCOVR_EXCL_LINE
}

#ifdef __cplusplus
}
#endif


