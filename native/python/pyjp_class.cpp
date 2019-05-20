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

static PyMethodDef classMethods[] = {
	{"getCanonicalName", (PyCFunction) (&PyJPClass::getCanonicalName), METH_NOARGS, ""},
	{"getSuperClass", (PyCFunction) (&PyJPClass::getSuperClass), METH_NOARGS, ""},
	{"getClassFields", (PyCFunction) (&PyJPClass::getClassFields), METH_NOARGS, ""},
	{"getClassMethods", (PyCFunction) (&PyJPClass::getClassMethods), METH_NOARGS, ""},
	{"newInstance", (PyCFunction) (&PyJPClass::newInstance), METH_VARARGS, ""},
	{"getSuperclass", (PyCFunction) (&PyJPClass::getSuperClass), METH_NOARGS, ""},
	{"getInterfaces", (PyCFunction) (&PyJPClass::getInterfaces), METH_NOARGS, ""},
	{"isInterface", (PyCFunction) (&PyJPClass::isInterface), METH_NOARGS, ""},
	{"isPrimitive", (PyCFunction) (&PyJPClass::isPrimitive), METH_NOARGS, ""},
	{"isThrowable", (PyCFunction) (&PyJPClass::isThrowable), METH_NOARGS, ""},
	{"isArray", (PyCFunction) (&PyJPClass::isArray), METH_NOARGS, ""},
	{"isAbstract", (PyCFunction) (&PyJPClass::isAbstract), METH_NOARGS, ""},
	{"isAssignableFrom", (PyCFunction) (&PyJPClass::isAssignableFrom), METH_VARARGS, ""},
	{"asJavaValue", (PyCFunction) (&PyJPClass::asJavaValue), METH_NOARGS, ""},
	{"canConvertToJava", (PyCFunction) (&PyJPClass::canConvertToJava), METH_VARARGS, ""},
	{"convertToJava", (PyCFunction) (&PyJPClass::convertToJava), METH_VARARGS, ""},

	{NULL},
};

PyTypeObject PyJPClass::Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	/* tp_name           */ "_jpype.PyJPClass",
	/* tp_basicsize      */ sizeof (PyJPClass),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPClass::__dealloc__,
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
	/* tp_flags          */ Py_TPFLAGS_DEFAULT,
	/* tp_doc            */
	"Internal representation of a Java Class.  This class can represent\n"
	"either an object, an array, or a primitive.  This type is stored as\n"
	"__javaclass__ in any wrapper Python classes or instances.",
	/* tp_traverse       */ 0,
	/* tp_clear          */ 0,
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
	/* tp_init           */ (initproc) PyJPClass::__init__,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPClass::__new__

};

// Static methods

void PyJPClass::initType(PyObject* module)
{
	PyType_Ready(&PyJPClass::Type);
	Py_INCREF(&PyJPClass::Type);
	PyModule_AddObject(module, "PyJPClass", (PyObject*) (&PyJPClass::Type));
}

bool PyJPClass::check(PyObject* o)
{
	return o->ob_type == &PyJPClass::Type;
}

JPPyObject PyJPClass::alloc(JPClass* cls)
{
	PyJPClass* res = PyObject_New(PyJPClass, &PyJPClass::Type);
	JP_PY_CHECK();
	res->m_Class = cls;
	return JPPyObject(JPPyRef::_claim, (PyObject*) res);
}

PyObject* PyJPClass::__new__(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
	PyJPClass* self = (PyJPClass*) type->tp_alloc(type, 0);
	self->m_Class = 0;
	return (PyObject*) self;
}

int PyJPClass::__init__(PyJPClass* self, PyObject* args, PyObject* kwargs)
{
	JP_TRACE_IN("PyJPClass::__init__");
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::__init__");
		JPJavaFrame frame;
		JPPyTuple tuple(JPPyRef::_use, args);
		if (tuple.size() != 1)
		{
			PyErr_SetString(PyExc_TypeError, "Classes must have one argument.");
			return (-1);
		}

		JPClass* claz = NULL;
		PyObject* arg0 = tuple.getItem(0);
		JPValue* jpvalue = JPPythonEnv::getJavaValue(arg0);
		if (jpvalue != NULL && jpvalue->getClass() == JPTypeManager::_java_lang_Class)
		{
			claz = JPTypeManager::findClass((jclass) jpvalue->getJavaObject());
		}
		else if (JPPyString::check(arg0))
		{
			string cname = JPPyString::asStringUTF8(arg0);
			claz = JPTypeManager::findClass(cname);
		}
		else
		{
			PyErr_SetString(PyExc_TypeError, "Classes require str or java.lang.Class object.");
			return (-1);
		}

		if (claz == NULL)
		{
			return -1;
		}
		self->m_Class = claz;
		return 0;
	}
	PY_STANDARD_CATCH;
	return -1;
	JP_TRACE_OUT;
}

void PyJPClass::__dealloc__(PyJPClass* self)
{
	Py_TYPE(self)->tp_free((PyObject*) self);
}

PyObject* PyJPClass::getCanonicalName(PyJPClass* self, PyObject* arg)
{
	JP_TRACE_IN("PyJPClass::getCanonicalName");
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::getName");
		JPJavaFrame frame;
		string name = self->m_Class->getCanonicalName();
		PyObject* res = JPPyString::fromStringUTF8(name).keep();
		return res;
	}
	PY_STANDARD_CATCH;
	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPClass::getSuperClass(PyJPClass* self, PyObject* arg)
{
	JP_TRACE_IN("PyJPClass::getSuperClass");
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::getBaseClass");
		JPJavaFrame frame;

		JPClass* base = self->m_Class->getSuperClass();
		if (base == NULL)
		{
			Py_RETURN_NONE;
		}

		return PyJPClass::alloc(base).keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPClass::getInterfaces(PyJPClass* self, PyObject* arg)
{
	JP_TRACE_IN("PyJPClass::getInterfaces");
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::getInterfaces");
		JPJavaFrame frame;

		const JPClass::ClassList& baseItf = self->m_Class->getInterfaces();

		// Pack into a tuple
		JPPyTuple result(JPPyTuple::newTuple(baseItf.size()));
		for (unsigned int i = 0; i < baseItf.size(); i++)
		{
			result.setItem(i, PyJPClass::alloc(baseItf[i]).get());
		}
		return result.keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPClass::getClassFields(PyJPClass* self, PyObject* arg)
{
	JP_TRACE_IN("PyJPClass::getClassFields");
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::getClassFields");
		JPJavaFrame frame;

		int i = 0;
		const JPClass::FieldList& instFields = self->m_Class->getFields();
		JPPyTuple result(JPPyTuple::newTuple(instFields.size()));
		for (JPClass::FieldList::const_iterator iter = instFields.begin(); iter != instFields.end(); iter++)
		{
			result.setItem(i++, PyJPField::alloc(*iter).get());
		}
		return result.keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPClass::getClassMethods(PyJPClass* self, PyObject* arg)
{
	JP_TRACE_IN("PyJPClass::getClassMethods");
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::getClassMethods");
		JPJavaFrame frame;

		const JPClass::MethodList& m_Methods = self->m_Class->getMethods();
		int i = 0;
		JPPyTuple result(JPPyTuple::newTuple(m_Methods.size()));
		for (JPClass::MethodList::const_iterator cur = m_Methods.begin(); cur != m_Methods.end(); cur++)
		{
			JP_TRACE("method ", *cur);
			result.setItem(i++, PyJPMethod::alloc(*cur, NULL).get());
		}

		return result.keep();

	}
	PY_STANDARD_CATCH;
	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPClass::newInstance(PyJPClass* self, PyObject* pyargs)
{
	JP_TRACE_IN("PyJPClass::newInstance");
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::newInstance");
		JPJavaFrame frame;

		if (dynamic_cast<JPArrayClass*> (self->m_Class) != NULL)
		{
			int sz;
			if (!PyArg_ParseTuple(pyargs, "i", &sz))
			{
				return NULL;
			}
			JPArrayClass* cls = (JPArrayClass*) (self->m_Class);
			return PyJPValue::alloc(cls->newInstance(sz)).keep();
		}

		JPPyObjectVector args(pyargs); // DEBUG
		for (size_t i = 0; i < args.size(); ++i)
		{
			ASSERT_NOT_NULL(args[i]);
		}
		JPClass* cls = (JPClass*) (self->m_Class);
		return PyJPValue::alloc(cls->newInstance(args)).keep();

		PyErr_SetString(PyExc_TypeError, "Unable to create an instance.");
	}
	PY_STANDARD_CATCH;
	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPClass::isAssignableFrom(PyJPClass* self, PyObject* arg)
{
	JP_TRACE_IN("PyJPClass::isSubclass");
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::isSubClass");
		JPJavaFrame frame;

		// We have to lookup the name by string here because the 
		// class wrapper may not exist.  This is used by the
		// customizers.
		PyObject* other;
		if (!PyArg_ParseTuple(arg, "O", &other))
		{
			return NULL;
		}

		JPClass* cls = JPPythonEnv::getJavaClass(other);
		if (cls != NULL)
		{
			return PyBool_FromLong(self->m_Class->isAssignableFrom(cls));
		}

		if (JPPyString::check(other))
		{
			JPClass* otherClass = JPTypeManager::findClass(JPPyString::asStringUTF8(other));
			return PyBool_FromLong(self->m_Class->isAssignableFrom(otherClass));
		}

		PyErr_SetString(PyExc_TypeError, "isAssignableFrom requires java class or string argument.");
		return NULL;
	}
	PY_STANDARD_CATCH;

	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPClass::isInterface(PyJPClass* self, PyObject* arg)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::isInterface");
		JPJavaFrame frame;
		return PyBool_FromLong(self->m_Class->isInterface());
	}
	PY_STANDARD_CATCH;

	return NULL;
}

PyObject* PyJPClass::isThrowable(PyJPClass* self, PyObject* args)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::isException");
		JPJavaFrame frame;
		return PyBool_FromLong(self->m_Class->isThrowable());
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPClass::isPrimitive(PyJPClass* self, PyObject* args)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::isPrimitive");
		JPJavaFrame frame;
		return PyBool_FromLong(dynamic_cast<JPPrimitiveType*> (self->m_Class) == self->m_Class);
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPClass::isArray(PyJPClass* self, PyObject* args)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::isArray");
		JPJavaFrame frame;
		return PyBool_FromLong(dynamic_cast<JPArrayClass*> (self->m_Class) == self->m_Class);
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPClass::isAbstract(PyJPClass* self, PyObject* args)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::isAbstract");
		JPJavaFrame frame;
		return PyBool_FromLong(self->m_Class->isAbstract());
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPClass::asJavaValue(PyJPClass* self, PyObject* args)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::asJavaValue");
		JPJavaFrame frame;
		jvalue v;
		v.l = self->m_Class->getJavaClass();
		return PyJPValue::alloc(JPTypeManager::_java_lang_Class, v).keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
}

// Added for auditing

PyObject* PyJPClass::canConvertToJava(PyJPClass* self, PyObject* args)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::asJavaValue");
		JPJavaFrame frame;

		PyObject* other;
		if (!PyArg_ParseTuple(args, "O", &other))
		{
			return NULL;
		}
		JPClass* cls = self->m_Class;

		// Test the conversion
		JPMatch::Type match = cls->canConvertToJava(other);

		// Report to user
		if (match == JPMatch::_none)
			return JPPyString::fromStringUTF8("none", false).keep();
		if (match == JPMatch::_explicit)
			return JPPyString::fromStringUTF8("explicit", false).keep();
		if (match == JPMatch::_implicit)
			return JPPyString::fromStringUTF8("implicit", false).keep();
		if (match == JPMatch::_exact)
			return JPPyString::fromStringUTF8("exact", false).keep();

		// Not sure how this could happen
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;
	return NULL;
}

// Added for auditing

PyObject* PyJPClass::convertToJava(PyJPClass* self, PyObject* args)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPClass::asJavaValue");
		JPJavaFrame frame;

		PyObject* other;
		if (!PyArg_ParseTuple(args, "O", &other))
		{
			return NULL;
		}
		JPClass* cls = self->m_Class;

		// Test the conversion
		JPMatch::Type match = cls->canConvertToJava(other);

		// If there is no conversion report a failure
		if (match == JPMatch::_none)
		{
			PyErr_SetString(PyExc_TypeError, "Unable to create an instance.");
			return 0;
		}

		// Otherwise give back a PyJPValue
		jvalue v = cls->convertToJava(other);
		return PyJPValue::alloc(cls, v).keep();
	}
	PY_STANDARD_CATCH;
	return NULL;
}
