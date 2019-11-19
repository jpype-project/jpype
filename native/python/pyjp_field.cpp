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

static PyGetSetDef fieldGetSets[] = {
	{"__name__", (getter) (&PyJPField::getName), NULL, ""},
	{"_final", (getter) (&PyJPField::isFinal), NULL, ""},
	{"_static", (getter) (&PyJPField::isStatic), NULL, ""},
	{0}
};

PyTypeObject PyJPField::Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	/* tp_name           */ "_jpype.PyJPField",
	/* tp_basicsize      */ sizeof (PyJPField),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPValue::__dealloc__,
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
	/* tp_str            */ 0,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
	/* tp_doc            */ "Java Field",
	/* tp_traverse       */ (traverseproc) PyJPValue::traverse,
	/* tp_clear          */ (inquiry) PyJPValue::clear,
	/* tp_richcompare    */ 0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter           */ 0,
	/* tp_iternext       */ 0,
	/* tp_methods        */ 0,
	/* tp_members        */ 0,
	/* tp_getset         */ fieldGetSets,
	/* tp_base           */ &PyJPValue::Type,
	/* tp_dict           */ 0,
	/* tp_descr_get      */ (descrgetfunc) PyJPField::__get__,
	/* tp_descr_set      */ (descrsetfunc) PyJPField::__set__,
	/* tp_dictoffset     */ 0,
	/* tp_init           */ 0,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyType_GenericNew

};

// Static methods

void PyJPField::initType(PyObject *module)
{
	PyType_Ready(&PyJPField::Type);
	Py_INCREF(&PyJPField::Type);
	PyModule_AddObject(module, "PyJPField", (PyObject*) (&PyJPField::Type));
}

JPPyObject PyJPField::alloc(JPField *field)
{
	JP_TRACE_IN_C("PyJPField::alloc");
	JPContext *context = field->getContext();
	jvalue v;
	v.l = field->getJavaObject();
	JPPyObject self = PyJPValue::alloc(&PyJPField::Type, field->getContext(),
			context->_java_lang_reflect_Method, v);
	((PyJPField*) self.get())->m_Field = field;
	return self;
	JP_TRACE_OUT;
}

PyObject *PyJPField::getName(PyJPField *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPField::getName", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return JPPyString::fromStringUTF8(self->m_Field->getName()).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPField::__get__(PyJPField *self, PyObject *obj, PyObject *type)
{
	JP_TRACE_IN_C("PyJPField::__get__", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
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

int PyJPField::__set__(PyJPField *self, PyObject *obj, PyObject *pyvalue)
{
	JP_TRACE_IN_C("PyJPField::__set__", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
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

PyObject *PyJPField::isStatic(PyJPField *self, PyObject *arg)
{
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(self->m_Field->isStatic());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPField::isFinal(PyJPField *self, PyObject *arg)
{
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(self->m_Field->isFinal());
	}
	PY_STANDARD_CATCH(NULL);
}
