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

PyObject *PyJPField_Type = NULL;
PyObject *PyJPField_name(PyJPField *self, PyObject *arg);
PyObject *PyJPField_get(PyJPField *self, PyObject *obj, PyObject *type);
int       PyJPField_set(PyJPField *self, PyObject *obj, PyObject *pyvalue);
PyObject *PyJPField_isStatic(PyJPField *self, PyObject *arg);
PyObject *PyJPField_isFinal(PyJPField *self, PyObject *arg);

static PyGetSetDef fieldGetSets[] = {
	{"__name__", (getter) (&PyJPField_name), NULL, ""},
	{"_final", (getter) (&PyJPField_isFinal), NULL, ""},
	{"_static", (getter) (&PyJPField_isStatic), NULL, ""},
	{0}
};

static PyType_Slot fieldSlots[] = {
	{ Py_tp_dealloc,   (destructor) PyJPValue_dealloc},
	{ Py_tp_traverse,  (traverseproc) PyJPValue_traverse},
	{ Py_tp_clear,     (inquiry) PyJPValue_clear},
	{ Py_tp_descr_get, (descrgetfunc) PyJPField_get},
	{ Py_tp_descr_set, (descrsetfunc) PyJPField_set},
	{ Py_tp_getset,    &fieldGetSets},
	{0}
};

static PyType_Spec fieldSpec = {
	"_jpype.PyJPField",
	sizeof (PyJPField),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
	fieldSlots
};

void PyJPField::initType(PyObject *module)
{
	PyObject *bases = PyTuple_Pack(1, PyJPValue_Type);
	PyModule_AddObject(module, "PyJPField",
			PyJPField_Type = PyType_FromSpecWithBases(&fieldSpec, bases));
	Py_DECREF(bases);
}

JPPyObject PyJPField::alloc(JPField *field)
{
	JP_TRACE_IN_C("PyJPField::alloc");
	JPContext *context = field->getContext();
	jvalue v;
	v.l = field->getJavaObject();
	JPPyObject self = PyJPValue::create(PyJPField_Type, field->getContext(),
			context->_java_lang_reflect_Field, v);
	((PyJPField*) self.get())->m_Field = field;
	return self;
	JP_TRACE_OUT;
}

PyObject *PyJPField_name(PyJPField *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPField::getName", self);
	try
	{
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		ASSERT_JVM_RUNNING(context);
		return JPPyString::fromStringUTF8(self->m_Field->getName()).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPField_get(PyJPField *self, PyObject *obj, PyObject *type)
{
	JP_TRACE_IN_C("PyJPField_get", self);
	try
	{
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		ASSERT_JVM_RUNNING(context);
		if (self->m_Field->isStatic())
			return self->m_Field->getStaticField().keep();
		if (obj == NULL)
			JP_RAISE_ATTRIBUTE_ERROR("Field is not static");
		JPValue *jval = JPPythonEnv::getJavaValue(obj);
		if (jval == NULL)
			JP_RAISE_ATTRIBUTE_ERROR("Field requires instance value");

		return self->m_Field->getField(jval->getValue().l).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

int PyJPField_set(PyJPField *self, PyObject *obj, PyObject *pyvalue)
{
	JP_TRACE_IN_C("PyJPField_set", self);
	try
	{
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		ASSERT_JVM_RUNNING(context);
		if (self->m_Field->isFinal())
			JP_RAISE_ATTRIBUTE_ERROR("Field is final");
		if (self->m_Field->isStatic())
		{
			self->m_Field->setStaticField(pyvalue);
			return 0;
		}
		if (obj == Py_None)
			JP_RAISE_ATTRIBUTE_ERROR("Field is not static");
		JPValue *jval = JPPythonEnv::getJavaValue(obj);
		if (jval == NULL)
			JP_RAISE_ATTRIBUTE_ERROR("Field requires instance value");
		self->m_Field->setField(jval->getValue().l, pyvalue);
		return 0;
	}
	PY_STANDARD_CATCH(-1);
	JP_TRACE_OUT_C;
}

PyObject *PyJPField_isStatic(PyJPField *self, PyObject *arg)
{
	try
	{
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(self->m_Field->isStatic());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPField_isFinal(PyJPField *self, PyObject *arg)
{
	try
	{
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(self->m_Field->isFinal());
	}
	PY_STANDARD_CATCH(NULL);
}
