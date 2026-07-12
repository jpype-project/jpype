// --- file: python/pyjp_field.cpp ---
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
#include "jp_field.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct PyJPField
{
	PyObject_HEAD
	JPField* m_Field;
	PyJPModuleState* m_State;
} ;

static inline int PyJPField_checkContext(PyJPField* self)
{
	if (self == nullptr || self->m_State == nullptr || self->m_State->context == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "JPype module context is not available");
		return 0;
	}
	return 1;
}

static void PyJPField_dealloc(PyJPField *self)
{
	self->m_Field = nullptr;
	Py_TYPE(self)->tp_free(self);
}

static PyObject *PyJPField_get(PyJPField *self, PyObject *obj, PyObject *type)
{
	JP_PY_TRY("PyJPField_get");
	if (!PyJPField_checkContext(self))
		return nullptr;
	JPContext* context = self->m_State->context;
	JPJavaFrame frame = JPJavaFrame::outer(context);
	// Clear any pending interrupts if we are on the main thread.
	if (context->hasInterrupt())
		frame.clearInterrupt(false);
	if (self->m_Field->isStatic())
		return self->m_Field->getStaticField(frame).keep();
	if (obj == nullptr)
		JP_RAISE(PyExc_AttributeError, "Field is not static");
	JPValue *jval = PyJPValue_getJavaSlot(obj);
	if (jval == nullptr)
		JP_RAISE(PyExc_AttributeError, "Field requires instance value");

	return self->m_Field->getField(frame, jval->getJavaObject(frame)).keep();
	JP_PY_CATCH(nullptr);
}

static int PyJPField_set(PyJPField *self, PyObject *obj, PyObject *pyvalue)
{
	JP_PY_TRY("PyJPField_set");
	if (!PyJPField_checkContext(self))
		return -1;
	JPJavaFrame frame = JPJavaFrame::outer(self->m_State->context);
	if (self->m_Field->isFinal())
	{
		PyErr_SetString(PyExc_AttributeError, "Field is final");
		return -1;
	}
	if (self->m_Field->isStatic())
	{
		self->m_Field->setStaticField(frame, pyvalue);
		return 0;
	}
	if (obj == Py_None || PyJPClass_Check(obj))
	{
		PyErr_SetString(PyExc_AttributeError, "Field is not static");
		return -1;
	}
	JPValue *jval = PyJPValue_getJavaSlot(obj);
	if (jval == nullptr)
	{
		PyErr_Format(PyExc_AttributeError, "Field requires instance value, not '%s'", Py_TYPE(obj)->tp_name);
		return -1;
	}
	self->m_Field->setField(frame, jval->getJavaObject(frame), pyvalue);
	return 0;
	JP_PY_CATCH(-1);
}

static PyObject *PyJPField_repr(PyJPField *self)
{
	JP_PY_TRY("PyJPField_repr");
	if (!PyJPField_checkContext(self))
		return nullptr;
	JPJavaFrame frame = JPJavaFrame::outer(self->m_State->context);
	return PyUnicode_FromFormat("<java field '%s' of '%s'>",
			self->m_Field->getName().c_str(),
			self->m_Field->getClass()->getCanonicalName(frame).c_str()
			);
	JP_PY_CATCH(nullptr);
}

static PyGetSetDef fieldGetSets[] = {
	{nullptr}
};

static PyType_Slot fieldSlots[] = {
	{ Py_tp_dealloc,   (void*) PyJPField_dealloc},
	{ Py_tp_descr_get, (void*) PyJPField_get},
	{ Py_tp_descr_set, (void*) PyJPField_set},
	{ Py_tp_repr,	  (void*) &PyJPField_repr},
	{ Py_tp_getset,	(void*) &fieldGetSets},
	{0}
};

PyType_Spec PyJPFieldSpec = {
	"_jpype._JField",
	sizeof (PyJPField),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	fieldSlots
};

void PyJPField_initType(PyObject* module, PyJPModuleState* st)
{
	st->PyJPField_Type = (PyTypeObject*) PyType_FromSpec(&PyJPFieldSpec);
	JP_PY_CHECK();
	Py_INCREF((PyObject*) st->PyJPField_Type);
	PyModule_AddObject(module, "_JField", (PyObject*) st->PyJPField_Type);
	JP_PY_CHECK();
}

JPPyObject PyJPField_create(JPJavaFrame& frame, JPField* m)
{
	JP_TRACE_IN("PyJPField_create");
	PyJPModuleState* st = frame.getContext()->modulestate;
	auto* self = (PyJPField*) st->PyJPField_Type->tp_alloc(st->PyJPField_Type, 0);
	JP_PY_CHECK();
	self->m_Field = m;
	self->m_State = frame.getContext()->modulestate;
	return JPPyObject::claim((PyObject*) self);
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

#ifdef __cplusplus
}
#endif


