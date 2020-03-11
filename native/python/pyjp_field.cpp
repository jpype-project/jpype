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
} ;

static void PyJPField_dealloc(PyJPField *self)
{
	self->m_Field = NULL;
	Py_TYPE(self)->tp_free(self);
}

static PyObject *PyJPField_get(PyJPField *self, PyObject *obj, PyObject *type)
{
	JP_PY_TRY("PyJPField_get");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	if (self->m_Field->isStatic())
		return self->m_Field->getStaticField().keep();
	if (obj == NULL)
		JP_RAISE(PyExc_AttributeError, "Field is not static");
	JPValue *jval = PyJPValue_getJavaSlot(obj);
	if (jval == NULL)
		JP_RAISE(PyExc_AttributeError, "Field requires instance value");

	return self->m_Field->getField(jval->getValue().l).keep();
	JP_PY_CATCH(NULL);
}

static int PyJPField_set(PyJPField *self, PyObject *obj, PyObject *pyvalue)
{
	JP_PY_TRY("PyJPField_set");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	if (self->m_Field->isFinal())
		JP_RAISE(PyExc_AttributeError, "Field is final");
	if (self->m_Field->isStatic())
	{
		self->m_Field->setStaticField(pyvalue);
		return 0;
	}
	if (obj == Py_None || PyJPClass_Check(obj))
		JP_RAISE(PyExc_AttributeError, "Field is not static");
	JPValue *jval = PyJPValue_getJavaSlot(obj);
	if (jval == NULL)
	{
		stringstream ss;
		ss << "Field requires instance value, not " << Py_TYPE(obj)->tp_name;
		JP_RAISE(PyExc_AttributeError,  ss.str().c_str());
	}
	self->m_Field->setField(jval->getValue().l, pyvalue);
	return 0;
	JP_PY_CATCH(-1);
}

static PyObject *PyJPField_repr(PyJPField *self)
{
	JP_PY_TRY("PyJPField_repr");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	stringstream ss;
	ss << "<java field `";
	ss << self->m_Field->getName() << "' of '" <<
			self->m_Field->getClass()->getCanonicalName() << "'>";
	return JPPyString::fromStringUTF8(ss.str()).keep();
	JP_PY_CATCH(NULL);
}

static PyGetSetDef fieldGetSets[] = {
	{0}
};

static PyType_Slot fieldSlots[] = {
	{ Py_tp_dealloc,   (void*) PyJPField_dealloc},
	{ Py_tp_descr_get, (void*) PyJPField_get},
	{ Py_tp_descr_set, (void*) PyJPField_set},
	{ Py_tp_repr,      (void*) &PyJPField_repr},
	{ Py_tp_getset,    (void*) &fieldGetSets},
	{0}
};

PyTypeObject *PyJPField_Type = NULL;
PyType_Spec PyJPFieldSpec = {
	"_jpype._JField",
	sizeof (PyJPField),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	fieldSlots
};

#ifdef __cplusplus
}
#endif

void PyJPField_initType(PyObject* module)
{
	PyJPField_Type = (PyTypeObject*) PyType_FromSpec(&PyJPFieldSpec);
	JP_PY_CHECK_INIT();
	PyModule_AddObject(module, "_JField", (PyObject*) PyJPField_Type);
	JP_PY_CHECK_INIT();
}

JPPyObject PyJPField_create(JPField* m)
{
	JP_TRACE_IN("PyJPField_create");
	PyJPField* self = (PyJPField*) PyJPField_Type->tp_alloc(PyJPField_Type, 0);
	JP_PY_CHECK();
	self->m_Field = m;
	return JPPyObject(JPPyRef::_claim, (PyObject*) self);
	JP_TRACE_OUT;
}
