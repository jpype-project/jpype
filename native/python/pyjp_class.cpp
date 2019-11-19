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
#include <Python.h>
#include <structmember.h>
#include <jpype.h>
#include <pyjp.h>

static PyMethodDef classMethods[] = {
	{"_getCanonicalName", (PyCFunction) (&PyJPClass::getCanonicalName), METH_NOARGS, ""},
	{"_getSuperClass", (PyCFunction) (&PyJPClass::getSuperClass), METH_NOARGS, ""},
	{"_getClassFields", (PyCFunction) (&PyJPClass::getClassFields), METH_NOARGS, ""},
	{"_getClassMethods", (PyCFunction) (&PyJPClass::getClassMethods), METH_NOARGS, ""},
	{"_cast", (PyCFunction) (&PyJPClass::cast), METH_VARARGS, ""},
	{"_getSuperclass", (PyCFunction) (&PyJPClass::getSuperClass), METH_NOARGS, ""},
	{"_getInterfaces", (PyCFunction) (&PyJPClass::getInterfaces), METH_NOARGS, ""},
	{"_isInterface", (PyCFunction) (&PyJPClass::isInterface), METH_NOARGS, ""},
	{"_isPrimitive", (PyCFunction) (&PyJPClass::isPrimitive), METH_NOARGS, ""},
	{"_isThrowable", (PyCFunction) (&PyJPClass::isThrowable), METH_NOARGS, ""},
	{"_isArray", (PyCFunction) (&PyJPClass::isArray), METH_NOARGS, ""},
	{"_isAbstract", (PyCFunction) (&PyJPClass::isAbstract), METH_NOARGS, ""},
	{"_isAssignableFrom", (PyCFunction) (&PyJPClass::isAssignableFrom), METH_VARARGS, ""},
	//	{"_asJavaValue", (PyCFunction) (&PyJPClass::asJavaValue), METH_NOARGS, ""},
	{"_canConvertToJava", (PyCFunction) (&PyJPClass::canConvertToJava), METH_VARARGS, ""},
	{"_convertToJava", (PyCFunction) (&PyJPClass::convertToJava), METH_VARARGS, ""},
	{"_dumpCtor", (PyCFunction) (&PyJPClass::dumpCtor), METH_VARARGS, ""},

	{NULL},
};

PyTypeObject PyJPClass::Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	/* tp_name           */ "_jpype.PyJPClass",
	/* tp_basicsize      */ sizeof (PyJPClass),
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
	/* tp_flags          */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
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
	/* tp_base           */ &PyJPValue::Type,
	/* tp_dict           */ 0,
	/* tp_descr_get      */ 0,
	/* tp_descr_set      */ 0,
	/* tp_dictoffset     */ 0,
	/* tp_init           */ (initproc) PyJPClass::__init__,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPClass::__new__

};

// Static methods

void PyJPClass::initType(PyObject *module)
{
	PyJPClass::Type.tp_base = &PyJPValue::Type;
	PyType_Ready(&PyJPClass::Type);
	Py_INCREF(&PyJPClass::Type);
	PyModule_AddObject(module, "PyJPClass", (PyObject*) (&PyJPClass::Type));
}

JPPyObject PyJPClass::alloc(PyTypeObject *type, JPContext *context, JPClass *cls)
{
	JP_TRACE_IN_C("PyJPClass::alloc");
	JPJavaFrame frame(context);
	jvalue value;
	value.l = (jobject) cls->getJavaClass();
	JPPyObject self = PyJPValue::alloc(type, context, context->_java_lang_Class, value);
	((PyJPClass*) self.get())->m_Class = cls;
	return self;
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	PyJPClass *self = (PyJPClass*) PyJPValue::__new__(type, args, kwargs);
	self->m_Class = NULL;
	return (PyObject*) self;
}

// FIXME, not clear which should win.  The JVM specified or the Class JVM
// thus let's make it an error for now.
// (jvm, str)

int PyJPClass::__init__(PyJPClass *self, PyObject *args, PyObject *kwargs)
{
	JP_TRACE_IN_C("PyJPClass::__init__", self);
	try
	{
		// Check if we are already initialized.
		if ((PyJPValue*) self->m_Class != 0)
			return 0;

		PyObject *arg0;
		PyObject *jvm = 0;

		if (!PyArg_ParseTuple(args, "OO", &jvm, &arg0))
		{
			return -1;
		}

		if (!PyJPContext::check(jvm))
		{
			PyErr_SetString(PyExc_TypeError, "JContext must be supplied as first argument");
			return -1;
		}

		JPContext *context = ((PyJPContext*) jvm)->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);

		JPClass *cls;
		JPValue *jpvalue = JPPythonEnv::getJavaValue(arg0);
		if (JPPyString::check(arg0))
		{
			if (!PyJPContext::check(jvm))
			{
				PyErr_SetString(PyExc_TypeError, "Classes from string require a Java context.");
				return -1;
			}
			string cname = JPPyString::asStringUTF8(arg0);
			cls = context->getTypeManager()->findClassByName(cname);
		} else
		{
			PyErr_SetString(PyExc_TypeError, "Classes require str object.");
			//			PyErr_SetString(PyExc_TypeError, "Classes require str or java.lang.Class object.");
			return (-1);
		}

		jvalue value;
		value.l = frame.NewGlobalRef((jobject) cls->getJavaClass());
		self->m_Value.m_Value = JPValue(context->_java_lang_Class, value);
		self->m_Value.m_Context = (PyJPContext*) (context->getHost());
		Py_INCREF(self->m_Value.m_Context);

		self->m_Class = cls;
		return 0;
	}
	PY_STANDARD_CATCH(-1);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::getCanonicalName(PyJPClass *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPClass::getCanonicalName", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		string name = self->m_Class->getCanonicalName();
		PyObject *res = JPPyString::fromStringUTF8(name).keep();
		return res;
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::getSuperClass(PyJPClass *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPClass::getSuperClass", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		JPClass *base = self->m_Class->getSuperClass();
		if (base == NULL)
		{
			Py_RETURN_NONE;
		}

		return PyJPClass::alloc(&PyJPClass::Type, context, base).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::getInterfaces(PyJPClass *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPClass::getInterfaces", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		const JPClassList& baseItf = self->m_Class->getInterfaces();

		// Pack into a tuple
		JPPyTuple result(JPPyTuple::newTuple(baseItf.size()));
		for (unsigned int i = 0; i < baseItf.size(); i++)
		{
			result.setItem(i, PyJPClass::alloc(&PyJPClass::Type, context, baseItf[i]).get());
		}
		return result.keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::getClassFields(PyJPClass *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPClass::getClassFields", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);

		int i = 0;
		const JPFieldList& instFields = self->m_Class->getFields();
		JPPyTuple result(JPPyTuple::newTuple(instFields.size()));
		for (JPFieldList::const_iterator iter = instFields.begin(); iter != instFields.end(); iter++)
		{
			result.setItem(i++, PyJPField::alloc(*iter).get());
		}
		return result.keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::getClassMethods(PyJPClass *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPClass::getClassMethods", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);

		const JPMethodDispatchList& m_Methods = self->m_Class->getMethods();
		int i = 0;
		JPPyTuple result(JPPyTuple::newTuple(m_Methods.size()));
		for (JPMethodDispatchList::const_iterator cur = m_Methods.begin(); cur != m_Methods.end(); cur++)
		{
			JP_TRACE("method ", *cur);
			result.setItem(i++, PyJPMethod::alloc(*cur, NULL).get());
		}

		return result.keep();

	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::cast(PyJPClass *self, PyObject *args)
{
	JP_TRACE_IN_C("PyJPClass::cast", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);

		PyObject *value;
		if (!PyArg_ParseTuple(args, "O", &value))
		{
			return 0;
		}

		JPClass *type = self->m_Class;
		ASSERT_NOT_NULL(value);
		ASSERT_NOT_NULL(type);
		JPPyObject wrapper = JPPythonEnv::newJavaClass(type);

		// If it is already a Java object, then let Java decide
		// if the cast is possible
		JPValue *jval = JPPythonEnv::getJavaValue(value);
		if (jval != NULL && type->isInstance(*jval))
		{
			return PyJPValue::create((PyTypeObject*) wrapper.get(), context, type, jval->getValue()).keep();
		}

		// Otherwise, see if we can convert it
		{
			JPJavaFrame frame(context);
			JPMatch match;
			type->getJavaConversion(frame, match, value);
			if (match.type == JPMatch::_none)
			{
				stringstream ss;
				ss << "Unable to convert " << Py_TYPE(value)->tp_name << " to java type " << type->toString();
				PyErr_SetString(PyExc_TypeError, ss.str().c_str());
				return 0;
			}

			jvalue v = match.conversion->convert(frame, type, value);
			return PyJPValue::create((PyTypeObject*) wrapper.get(), context, type, v).keep();
		}
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::newInstance(PyJPClass *self, PyObject *pyargs)
{
	JP_TRACE_IN_C("PyJPClass::newInstance", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);

		if (dynamic_cast<JPArrayClass*> (self->m_Class) != NULL)
		{
			int sz;
			if (!PyArg_ParseTuple(pyargs, "i", &sz))
			{
				return NULL;
			}
			JPArrayClass *cls = (JPArrayClass*) (self->m_Class);
			JPValue value = cls->newInstance(sz);
			return JPPythonEnv::newJavaObject(value).keep();
		}

		JPPyObjectVector args(pyargs); // DEBUG
		for (size_t i = 0; i < args.size(); ++i)
		{
			ASSERT_NOT_NULL(args[i]);
		}
		JPClass *cls = (JPClass*) (self->m_Class);
		JPValue value = cls->newInstance(args);
		return JPPythonEnv::newJavaObject(value).keep();

		PyErr_SetString(PyExc_TypeError, "Unable to create an instance.");
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::isAssignableFrom(PyJPClass *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPClass::isSubclass", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);

		// We have to lookup the name by string here because the
		// class wrapper may not exist.  This is used by the
		// customizers.
		PyObject *other;
		if (!PyArg_ParseTuple(arg, "O", &other))
		{
			return NULL;
		}

		JPClass *cls = JPPythonEnv::getJavaClass(other);
		if (cls != NULL)
		{
			return PyBool_FromLong(self->m_Class->isAssignableFrom(cls));
		}

		if (JPPyString::check(other))
		{
			JPClass *otherClass = context->getTypeManager()
					->findClassByName(JPPyString::asStringUTF8(other));
			return PyBool_FromLong(self->m_Class->isAssignableFrom(otherClass));
		}

		PyErr_SetString(PyExc_TypeError, "isAssignableFrom requires java class or string argument.");
		return NULL;
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::isInterface(PyJPClass *self, PyObject *arg)
{
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(self->m_Class->isInterface());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass::isThrowable(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(self->m_Class->isThrowable());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass::isPrimitive(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong((self->m_Class)->isPrimitive());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass::isArray(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(dynamic_cast<JPArrayClass*> (self->m_Class) == self->m_Class);
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass::isAbstract(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(self->m_Class->isAbstract());
	}
	PY_STANDARD_CATCH(NULL);
}

//PyObject *PyJPClass::asJavaValue(PyJPClass *self, PyObject *args)
//{
//	try
//	{
//		JPContext *context = self->m_Value.m_Context->m_Context;
//		ASSERT_JVM_RUNNING(context);
//		JPJavaFrame frame(context);
//		jvalue v;
//		v.l = self->m_Class->getJavaClass();
//		return PyJPValue::alloc(context, context->_java_lang_Class, v).keep();
//	}
//	PY_STANDARD_CATCH(NULL);
//}

// Added for auditing

PyObject *PyJPClass::canConvertToJava(PyJPClass *self, PyObject *args)
{
	JP_TRACE_IN_C("PyJPClass::canConvertToJava", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);

		PyObject *other;
		if (!PyArg_ParseTuple(args, "O", &other))
		{
			return NULL;
		}
		JPClass *cls = self->m_Class;

		// Test the conversion
		JPMatch match;
		cls->getJavaConversion(frame, match, other);

		// Report to user
		if (match.type == JPMatch::_none)
			return JPPyString::fromStringUTF8("none", false).keep();
		if (match.type == JPMatch::_explicit)
			return JPPyString::fromStringUTF8("explicit", false).keep();
		if (match.type == JPMatch::_implicit)
			return JPPyString::fromStringUTF8("implicit", false).keep();
		if (match.type == JPMatch::_exact)
			return JPPyString::fromStringUTF8("exact", false).keep();

		// Not sure how this could happen
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

// Added for auditing

PyObject *PyJPClass::convertToJava(PyJPClass *self, PyObject *args)
{
	JP_TRACE_IN_C("PyJPClass::convertToJava", self);
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);

		PyObject *other;
		if (!PyArg_ParseTuple(args, "O", &other))
		{
			return NULL;
		}
		JPClass *cls = self->m_Class;

		// Test the conversion
		JPMatch match;
		cls->getJavaConversion(frame, match, other);

		// If there is no conversion report a failure
		if (match.type == JPMatch::_none)
		{
			PyErr_SetString(PyExc_TypeError, "Unable to create an instance.");
			return 0;
		}

		// Otherwise give back a PyJPValue
		jvalue v = match.conversion->convert(frame, cls, other);
		return PyJPValue::alloc(&PyJPValue::Type, context, cls, v).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::dumpCtor(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		string report = self->m_Class->getCtor()->dump();
		return JPPyString::fromStringUTF8(report).keep();
	}
	PY_STANDARD_CATCH(NULL);
}
