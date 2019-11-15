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
	{"dumpCtor", (PyCFunction) (&PyJPClass::dumpCtor), METH_VARARGS, ""},

	{NULL},
};

static PyMemberDef classMembers[] = {
	{"__jvm__", T_OBJECT, offsetof(PyJPClass, m_Context), READONLY},
	{0}
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
	/* tp_flags          */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
	/* tp_doc            */
	"Internal representation of a Java Class.  This class can represent\n"
	"either an object, an array, or a primitive.  This type is stored as\n"
	"__javaclass__ in any wrapper Python classes or instances.",
	/* tp_traverse       */ (traverseproc) PyJPClass::traverse,
	/* tp_clear          */ (inquiry) PyJPClass::clear,
	/* tp_richcompare    */ 0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter           */ 0,
	/* tp_iternext       */ 0,
	/* tp_methods        */ classMethods,
	/* tp_members        */ classMembers,
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

void PyJPClass::initType(PyObject *module)
{
	PyType_Ready(&PyJPClass::Type);
	Py_INCREF(&PyJPClass::Type);
	PyModule_AddObject(module, "PyJPClass", (PyObject*) (&PyJPClass::Type));
}

bool PyJPClass::check(PyObject *o)
{
	return o != NULL && Py_TYPE(o) == &PyJPClass::Type;
}

JPPyObject PyJPClass::alloc(JPClass *cls)
{
	JP_TRACE_IN_C("PyJPClass::alloc");
	PyJPClass *self = (PyJPClass*) PyJPClass::Type.tp_alloc(&PyJPClass::Type, 0);
	JP_PY_CHECK();
	self->m_Class = cls;
	self->m_Context = (PyJPContext*) (cls->getContext()->getHost());
	Py_INCREF(self->m_Context);
	JP_TRACE("self", self);
	return JPPyObject(JPPyRef::_claim, (PyObject*) self);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	PyJPClass *self = (PyJPClass*) type->tp_alloc(type, 0);
	self->m_Class = NULL;
	self->m_Context = NULL;
	return (PyObject*) self;
}

int PyJPClass::__init__(PyJPClass *self, PyObject *args, PyObject *kwargs)
{
	JP_TRACE_IN_C("PyJPClass::__init__", self);
	try
	{
		PyObject *arg0;
		PyObject *jvm = 0;

		if (!PyArg_ParseTuple(args, "O|O", &arg0, &jvm))
		{
			return -1;
		}

		JPContext *context;
		JPClass *cls;
		JPValue *jpvalue = JPPythonEnv::getJavaValue(arg0);
		if (jpvalue != NULL)
		{
			context = jpvalue->getClass()->getContext();
			if (jpvalue->getClass() != context->_java_lang_Class)
			{
				stringstream err;
				err << "Incorrect Java class " << jpvalue->getClass()->toString()
					<< " vs " << context->_java_lang_Class->toString();
				PyErr_SetString(PyExc_TypeError, err.str().c_str());
				return -1;
			}
			ASSERT_JVM_RUNNING(context);
			JPJavaFrame frame(context);
			cls = context->getTypeManager()->findClass((jclass) jpvalue->getJavaObject());
		}
		else if (JPPyString::check(arg0))
		{
			if (!PyJPContext::check(jvm))
			{
				PyErr_SetString(PyExc_TypeError, "Classes from string require a Java context.");
				return -1;
			}
			context = ((PyJPContext*) jvm)->m_Context;
			ASSERT_JVM_RUNNING(context);
			JPJavaFrame frame(context);
			string cname = JPPyString::asStringUTF8(arg0);
			cls = context->getTypeManager()->findClassByName(cname);
		}
		else
		{
			PyErr_SetString(PyExc_TypeError, "Classes require str or java.lang.Class object.");
			return (-1);
		}
		self->m_Context = (PyJPContext*) (context->getHost());
		Py_INCREF(self->m_Context);

		self->m_Class = cls;
		return 0;
	}
	PY_STANDARD_CATCH(-1);
	JP_TRACE_OUT_C;
}

void PyJPClass::__dealloc__(PyJPClass *self)
{
	JP_TRACE_IN_C("PyJPClass::__dealloc__", self);
	PyObject_GC_UnTrack(self);
	clear(self);
	Py_TYPE(self)->tp_free((PyObject*) self);
	JP_TRACE_OUT_C;
}

int PyJPClass::traverse(PyJPClass *self, visitproc visit, void *arg)
{
	JP_TRACE_IN_C("PyJPClass::traverse", self);
	Py_VISIT(self->m_Context);
	return 0;
	JP_TRACE_OUT_C;
}

int PyJPClass::clear(PyJPClass *self)
{
	JP_TRACE_IN_C("PyJPClass::clear", self);
	JP_TRACE("context", self->m_Context);
	Py_CLEAR(self->m_Context);
	return 0;
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::getCanonicalName(PyJPClass *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPClass::getCanonicalName", self);
	try
	{
		JPContext *context = self->m_Context->m_Context;
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
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		JPClass *base = self->m_Class->getSuperClass();
		if (base == NULL)
		{
			Py_RETURN_NONE;
		}

		return PyJPClass::alloc(base).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::getInterfaces(PyJPClass *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPClass::getInterfaces", self);
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		const JPClassList& baseItf = self->m_Class->getInterfaces();

		// Pack into a tuple
		JPPyTuple result(JPPyTuple::newTuple(baseItf.size()));
		for (unsigned int i = 0; i < baseItf.size(); i++)
		{
			result.setItem(i, PyJPClass::alloc(baseItf[i]).get());
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
		JPContext *context = self->m_Context->m_Context;
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
		JPContext *context = self->m_Context->m_Context;
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

PyObject *PyJPClass::newInstance(PyJPClass *self, PyObject *pyargs)
{
	JP_TRACE_IN_C("PyJPClass::newInstance", self);
	try
	{
		JPContext *context = self->m_Context->m_Context;
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
			return PyJPValue::alloc(cls->newInstance(sz)).keep();
		}

		JPPyObjectVector args(pyargs); // DEBUG
		for (size_t i = 0; i < args.size(); ++i)
		{
			ASSERT_NOT_NULL(args[i]);
		}
		JPClass *cls = (JPClass*) (self->m_Class);
		return PyJPValue::alloc(cls->newInstance(args)).keep();

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
		JPContext *context = self->m_Context->m_Context;
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
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(self->m_Class->isInterface());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass::isThrowable(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(self->m_Class->isThrowable());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass::isPrimitive(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong((self->m_Class)->isPrimitive());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass::isArray(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(dynamic_cast<JPArrayClass*> (self->m_Class) == self->m_Class);
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass::isAbstract(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(self->m_Class->isAbstract());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass::asJavaValue(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		jvalue v;
		v.l = self->m_Class->getJavaClass();
		return PyJPValue::alloc(context->_java_lang_Class, v).keep();
	}
	PY_STANDARD_CATCH(NULL);
}

// Added for auditing

PyObject *PyJPClass::canConvertToJava(PyJPClass *self, PyObject *args)
{
	JP_TRACE_IN_C("PyJPClass::canConvertToJava", self);
	try
	{
		JPContext *context = self->m_Context->m_Context;
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
		JPContext *context = self->m_Context->m_Context;
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
		return PyJPValue::alloc(cls, v).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPClass::dumpCtor(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		string report = self->m_Class->getCtor()->dump();
		return JPPyString::fromStringUTF8(report).keep();
	}
	PY_STANDARD_CATCH(NULL);
}
