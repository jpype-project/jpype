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
#include "jp_class.h"
#include "jpype.h"
#include "pyjp.h"
#include "jp_field.h"
#include <string_view>

struct PyJPField
{
	PyObject_HEAD
	JPField* m_Field;
} ;

static void PyJPField_dealloc(PyJPField *self)
{
	self->m_Field = nullptr;
	Py_TYPE(self)->tp_free(self);
}

static PyObject *PyJPField_get(PyJPField *self, PyObject *obj, PyObject *type)
{
	JP_PY_TRY("PyJPField_get");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	// Clear any pending interrupts if we are on the main thread.
	if (hasInterrupt())
		frame.clearInterrupt(false);
	if (self->m_Field->isStatic())
		return self->m_Field->getStaticField().keep();
	if (obj == nullptr)
		JP_RAISE(PyExc_AttributeError, "Field is not static");
	JPValue *jval = PyJPValue_getJavaSlot(obj);
	if (jval == nullptr)
		JP_RAISE(PyExc_AttributeError, "Field requires instance value");

	return self->m_Field->getField(jval->getValue().l).keep();
	JP_PY_CATCH(nullptr);
}

static bool isInitializingFinalField(JPField &field) {
	if (field.isStatic()) {
		// not applicable
		// static final fields can use the default value
		return false;
	}

	JPClass *cls = field.getClass();
	if (!cls->isExtension()) {
		return false;
	}

	PyObject *locals = PyEval_GetLocals();
	if (locals == nullptr) {
		// access denied
		return false;
	}

	JPPyObject obj = JPPyObject::call(PyMapping_GetItemString(locals, "self"));
	if (obj.get() != nullptr) {
		obj = JPPyObject::use((PyObject *) Py_TYPE(obj.get()));
	} else {
		obj = JPPyObject::call(PyMapping_GetItemString(locals, "cls"));
	}

	if (obj.isNull()) {
		return false;
	}

	if (cls != PyJPClass_getJPClass(obj.get())) {
		// it may only be initialized in the constructor for this class
		return false;
	}

	// frame cannot be null or locals would have been null
	// code cannot be null
	JPPyObject code = JPPyObject::accept((PyObject*)PyFrame_GetCode(PyEval_GetFrame()));

	Py_ssize_t size = 0;
	const char *name = PyUnicode_AsUTF8AndSize(((PyCodeObject *)code.get())->co_name, &size);
	if (name == nullptr) {
		JP_RAISE_PYTHON();
	}
	return std::string_view{name, (size_t)size} == "__init__"sv;
}

static int PyJPField_set(PyJPField *self, PyObject *obj, PyObject *pyvalue)
{
	JP_PY_TRY("PyJPField_set");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	if (self->m_Field->isFinal())
	{
		if (self->m_Field->isStatic() || !isInitializingFinalField(*self->m_Field)) {
			PyErr_SetString(PyExc_AttributeError, "Field is final");
			return -1;
		}
	}
	if (self->m_Field->isStatic())
	{
		self->m_Field->setStaticField(pyvalue);
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
	self->m_Field->setField(jval->getValue().l, pyvalue);
	return 0;
	JP_PY_CATCH(-1);
}

static PyObject *PyJPField_repr(PyJPField *self)
{
	JP_PY_TRY("PyJPField_repr");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	return PyUnicode_FromFormat("<java field '%s' of '%s'>",
			self->m_Field->getName().data(),
			self->m_Field->getClass()->getCanonicalName().data()
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
	{ Py_tp_repr,      (void*) &PyJPField_repr},
	{ Py_tp_getset,    (void*) &fieldGetSets},
	{0}
};

PyTypeObject *PyJPField_Type = nullptr;
PyType_Spec PyJPFieldSpec = {
	"_jpype._JField",
	sizeof (PyJPField),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	fieldSlots
};

void PyJPField_initType(PyObject* module)
{
	PyJPField_Type = (PyTypeObject*) PyType_FromSpec(&PyJPFieldSpec);
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JField", (PyObject*) PyJPField_Type);
	JP_PY_CHECK();
}

JPPyObject PyJPField_create(JPField* m)
{
	JP_TRACE_IN("PyJPField_create");
	auto* self = (PyJPField*) PyJPField_Type->tp_alloc(PyJPField_Type, 0);
	JP_PY_CHECK();
	self->m_Field = m;
	return JPPyObject::claim((PyObject*) self);
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}
