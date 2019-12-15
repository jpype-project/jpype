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
#include <pyjp.h>

#ifdef __cplusplus
extern "C"
{
#endif

PyObject *PyJPField_name(PyJPField *self, PyObject *arg)
{
	JP_PY_TRY("PyJPField_name", self);
	PyJPModule_getContext();
	return JPPyString::fromStringUTF8(self->m_Field->getName()).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPField_get(PyJPField *self, PyObject *obj, PyObject *type)
{
	JP_PY_TRY("PyJPField_get", self);
	PyJPModule_getContext();
	if (self->m_Field->isStatic())
		return self->m_Field->getStaticField().keep();
	if (obj == NULL)
		JP_RAISE(PyExc_AttributeError, "Field is not static");
	JPValue *jval = JPPythonEnv::getJavaValue(obj);
	if (jval == NULL)
		JP_RAISE(PyExc_AttributeError, "Field requires instance value");

	return self->m_Field->getField(jval->getValue().l).keep();
	JP_PY_CATCH(NULL);
}

int PyJPField_set(PyJPField *self, PyObject *obj, PyObject *pyvalue)
{
	JP_PY_TRY("PyJPField_set", self);
	PyJPModule_getContext();
	if (self->m_Field->isFinal())
		JP_RAISE(PyExc_AttributeError, "Field is final");
	if (self->m_Field->isStatic())
	{
		self->m_Field->setStaticField(pyvalue);
		return 0;
	}
	if (obj == Py_None)
		JP_RAISE(PyExc_AttributeError, "Field is not static");
	JPValue *jval = JPPythonEnv::getJavaValue(obj);
	if (jval == NULL)
	{
		stringstream ss;
		ss << "Field requires instance value, not " << Py_TYPE(obj)->tp_name;
		JP_RAISE(PyExc_AttributeError, ss.str().c_str());
	}
	self->m_Field->setField(jval->getValue().l, pyvalue);
	return 0;
	JP_PY_CATCH(-1);
}

PyObject *PyJPField_isStatic(PyJPField *self, PyObject *arg)
{
	JP_PY_TRY("PyJPField_isStatic", self);
	PyJPModule_getContext();
	return PyBool_FromLong(self->m_Field->isStatic());
	JP_PY_CATCH(NULL);
}

PyObject *PyJPField_isFinal(PyJPField *self, PyObject *arg)
{
	JP_PY_TRY("PyJPField_isFinal", self);
	PyJPModule_getContext();
	return PyBool_FromLong(self->m_Field->isFinal());
	JP_PY_CATCH(NULL);
}

PyObject *PyJPField_repr(PyJPField *self)
{
	JP_PY_TRY("PyJPField_repr", self);
	PyJPModule_getContext();
	stringstream ss;
	ss << "<java field `";
	ss << self->m_Field->getName() << "' of '" <<
			self->m_Field->getClass()->getCanonicalName() << "'>";
	return JPPyString::fromStringUTF8(ss.str()).keep();
	JP_PY_CATCH(NULL);
}

// These are hard to get to as any attempt to access them
// through getattr will dereference them due to getset.
// Thus these methods are only accessable directly during
// class initialization.
//
// After then use  obj.__class__.__dict__['fieldname']._final
static PyGetSetDef fieldGetSets[] = {
	{"__name__", (getter) (&PyJPField_name), NULL, ""},
	{"_final", (getter) (&PyJPField_isFinal), NULL, ""},
	{"_static", (getter) (&PyJPField_isStatic), NULL, ""},
	{0}
};

void PyJPValue_dealloc(PyJPValue *self);

static PyType_Slot fieldSlots[] = {
	{ Py_tp_dealloc,   (void*) PyJPValue_dealloc},
	{ Py_tp_descr_get, (void*) PyJPField_get},
	{ Py_tp_descr_set, (void*) PyJPField_set},
	{ Py_tp_repr,      (void*) &PyJPField_repr},
	{ Py_tp_getset,    (void*) &fieldGetSets},
	{0}
};

PyType_Spec PyJPFieldSpec = {
	"_jpype.PyJPField",
	sizeof (PyJPField),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	fieldSlots
};

#ifdef __cplusplus
}
#endif

JPPyObject PyJPField_create(JPField *field)
{
	JP_TRACE_IN("PyJPField_create");
	PyJPModuleState *state = PyJPModuleState_global;
	JPContext *context = field->getContext();
	jvalue v;
	v.l = field->getJavaObject();
	if (state->PyJPField_Type == NULL)
		JP_RAISE(PyExc_RuntimeError, "PyJPField type is not defined.");
	if (context->_java_lang_reflect_Field == NULL)
		JP_RAISE(PyExc_RuntimeError, "java.lang.reflect.Field not loaded.");
	JPPyObject self = PyJPValue_createInstance((PyTypeObject*) state->PyJPField_Type, field->getContext(),
			JPValue(context->_java_lang_reflect_Field, v));
	((PyJPField*) self.get())->m_Field = field;
	return self;
	JP_TRACE_OUT;
}
