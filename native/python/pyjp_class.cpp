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

#ifdef __cplusplus
extern "C"
{
#endif

PyObject *PyJPClass_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int PyJPClass_init(PyJPClass *self, PyObject *args, PyObject *kwargs);
PyObject *PyJPClass_getCanonicalName(PyJPClass *self, PyObject *arg);
PyObject *PyJPClass_getSuperClass(PyJPClass *self, PyObject *arg);
PyObject *PyJPClass_getInterfaces(PyJPClass *self, PyObject *arg);
PyObject *PyJPClass_getClassFields(PyJPClass *self, PyObject *arg);
PyObject *PyJPClass_getClassMethods(PyJPClass *self, PyObject *arg);
PyObject *PyJPClass_cast(PyJPClass *self, PyObject *args);
PyObject *PyJPClass_newInstance(PyJPClass *self, PyObject *pyargs);
PyObject *PyJPClass_isAssignableFrom(PyJPClass *self, PyObject *arg);
PyObject *PyJPClass_isInterface(PyJPClass *self, PyObject *arg);
PyObject *PyJPClass_isThrowable(PyJPClass *self, PyObject *args);
PyObject *PyJPClass_isPrimitive(PyJPClass *self, PyObject *args);
PyObject *PyJPClass_isArray(PyJPClass *self, PyObject *args);
PyObject *PyJPClass_isAbstract(PyJPClass *self, PyObject *args);
PyObject *PyJPClass_canConvertToJava(PyJPClass *self, PyObject *args);
PyObject *PyJPClass_convertToJava(PyJPClass *self, PyObject *args);
PyObject *PyJPClass_dumpCtor(PyJPClass *self, PyObject *args);

const char *classDoc =
		"Internal representation of a Java Class.  This class can represent\n"
		"either an object, an array, or a primitive.  This type is stored as\n"
		"__javaclass__ in any wrapper Python classes or instances.";

static PyMethodDef classMethods[] = {
	{"_getCanonicalName", (PyCFunction) (&PyJPClass_getCanonicalName), METH_NOARGS, ""},
	{"_getSuperClass", (PyCFunction) (&PyJPClass_getSuperClass), METH_NOARGS, ""},
	{"_getClassFields", (PyCFunction) (&PyJPClass_getClassFields), METH_NOARGS, ""},
	{"_getClassMethods", (PyCFunction) (&PyJPClass_getClassMethods), METH_NOARGS, ""},
	{"_cast", (PyCFunction) (&PyJPClass_cast), METH_VARARGS, ""},
	{"_getSuperclass", (PyCFunction) (&PyJPClass_getSuperClass), METH_NOARGS, ""},
	{"_getInterfaces", (PyCFunction) (&PyJPClass_getInterfaces), METH_NOARGS, ""},
	{"_isInterface", (PyCFunction) (&PyJPClass_isInterface), METH_NOARGS, ""},
	{"_isPrimitive", (PyCFunction) (&PyJPClass_isPrimitive), METH_NOARGS, ""},
	{"_isThrowable", (PyCFunction) (&PyJPClass_isThrowable), METH_NOARGS, ""},
	{"_isArray", (PyCFunction) (&PyJPClass_isArray), METH_NOARGS, ""},
	{"_isAbstract", (PyCFunction) (&PyJPClass_isAbstract), METH_NOARGS, ""},
	{"_isAssignableFrom", (PyCFunction) (&PyJPClass_isAssignableFrom), METH_VARARGS, ""},
	{"_canConvertToJava", (PyCFunction) (&PyJPClass_canConvertToJava), METH_VARARGS, ""},
	{"_convertToJava", (PyCFunction) (&PyJPClass_convertToJava), METH_VARARGS, ""},
	{"_dumpCtor", (PyCFunction) (&PyJPClass_dumpCtor), METH_VARARGS, ""},
	{NULL},
};

static PyType_Slot classSlots[] = {
	{ Py_tp_new,     PyJPClass_new},
	{ Py_tp_init,    (initproc) PyJPClass_init},
	{ Py_tp_methods, classMethods},
	{ Py_tp_doc,     (void*) classDoc},
	{0}
};

PyType_Spec PyJPClassSpec = {
	"_jpype.PyJPClass",
	sizeof (PyJPClass),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
	classSlots
};

PyObject *PyJPClass_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	try
	{
		JP_TRACE_IN_C("PyJPClass_new", self);
		PyJPModuleState *state = PyJPModuleState_global;
		PyTypeObject *base = (PyTypeObject *) state->PyJPValue_Type;
		PyObject *self = base->tp_new(type, args, kwargs);
		((PyJPClass*) self)->m_Class = NULL;
		return self;
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

// FIXME, not clear which should win.  The JVM specified or the Class JVM
// thus let's make it an error for now.
// (jvm, str)

int PyJPClass_init(PyJPClass *self, PyObject *args, PyObject *kwargs)
{
	try
	{
		JP_TRACE_IN_C("PyJPClass_init", self);
		PyJPModuleState *state = PyJPModuleState_global;
		// Check if we are already initialized.
		if ((PyJPValue*) self->m_Class != 0)
			return 0;

		PyObject *arg0;
		PyObject *jvm = 0;

		if (!PyArg_ParseTuple(args, "O!O", state->PyJPContext_Type, &jvm, &arg0))
		{
			return -1;
		}

		JPContext *context = ((PyJPContext*) jvm)->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);

		JPClass *cls;
		JPValue *jpvalue = JPPythonEnv::getJavaValue(arg0);
		if (JPPyString::check(arg0))
		{
			string cname = JPPyString::asStringUTF8(arg0);
			cls = context->getTypeManager()->findClassByName(cname);
		} else
		{
			PyErr_SetString(PyExc_TypeError, "Classes require str object.");
			return (-1);
		}

		jvalue value;
		value.l = frame.NewGlobalRef((jobject) cls->getJavaClass());
		self->m_Value.m_Value = JPValue(context->_java_lang_Class, value);
		self->m_Value.m_Context = (PyJPContext*) (context->getHost());
		Py_INCREF(self->m_Value.m_Context);
		self->m_Class = cls;
		return 0;
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(-1);
}

PyObject *PyJPClass_getCanonicalName(PyJPClass *self, PyObject *arg)
{
	try
	{
		JP_TRACE_IN_C("PyJPClass_getCanonicalName", self);
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		JPJavaFrame frame(context);
		string name = self->m_Class->getCanonicalName();
		PyObject *res = JPPyString::fromStringUTF8(name).keep();
		return res;
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_getSuperClass(PyJPClass *self, PyObject *arg)
{
	try
	{
		JP_TRACE_IN_C("PyJPClass_getSuperClass", self);
		PyJPModuleState *state = PyJPModuleState_global;
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		JPJavaFrame frame(context);
		JPClass *base = self->m_Class->getSuperClass();
		if (base == NULL)
		{
			Py_RETURN_NONE;
		}

		return PyJPClass_create((PyTypeObject*) state->PyJPClass_Type, context, base).keep();
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_getInterfaces(PyJPClass *self, PyObject *arg)
{
	try
	{
		JP_TRACE_IN_C("PyJPClass_getInterfaces", self);
		PyJPModuleState *state = PyJPModuleState_global;
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		JPJavaFrame frame(context);
		const JPClassList& baseItf = self->m_Class->getInterfaces();

		// Pack into a tuple
		JPPyTuple result(JPPyTuple::newTuple(baseItf.size()));
		for (unsigned int i = 0; i < baseItf.size(); i++)
		{
			result.setItem(i, PyJPClass_create((PyTypeObject*) state->PyJPClass_Type, context, baseItf[i]).get());
		}
		return result.keep();
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_getClassFields(PyJPClass *self, PyObject *arg)
{
	try
	{
		JP_TRACE_IN_C("PyJPClass_getClassFields", self);
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		JPJavaFrame frame(context);

		int i = 0;
		const JPFieldList& instFields = self->m_Class->getFields();
		JPPyTuple result(JPPyTuple::newTuple(instFields.size()));
		for (JPFieldList::const_iterator iter = instFields.begin(); iter != instFields.end(); iter++)
		{
			result.setItem(i++, PyJPField_create(*iter).get());
		}
		return result.keep();
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_getClassMethods(PyJPClass *self, PyObject *arg)
{
	try
	{
		JP_TRACE_IN_C("PyJPClass_getClassMethods", self);
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		JPJavaFrame frame(context);

		const JPMethodDispatchList& m_Methods = self->m_Class->getMethods();
		int i = 0;
		JPPyTuple result(JPPyTuple::newTuple(m_Methods.size()));
		for (JPMethodDispatchList::const_iterator cur = m_Methods.begin(); cur != m_Methods.end(); cur++)
		{
			JP_TRACE("method ", *cur);
			result.setItem(i++, PyJPMethod_create(*cur, NULL).get());
		}

		return result.keep();
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_cast(PyJPClass *self, PyObject *args)
{
	try
	{
		JP_TRACE_IN_C("PyJPClass_cast", self);
		JPContext *context = PyJPValue_GET_CONTEXT(self);

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
			return PyJPValue_create((PyTypeObject*) wrapper.get(), context, JPValue(type, jval->getValue()))
					.keep();
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
			return PyJPValue_create((PyTypeObject*) wrapper.get(), context, JPValue(type, v)).keep();
		}
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_newInstance(PyJPClass *self, PyObject *pyargs)
{
	try
	{
		JP_TRACE_IN_C("PyJPClass_newInstance", self);
		JPContext *context = PyJPValue_GET_CONTEXT(self);
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
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_isAssignableFrom(PyJPClass *self, PyObject *arg)
{
	try
	{
		JP_TRACE_IN_C("PyJPClass_isAssignableFrom", self);
		JPContext *context = PyJPValue_GET_CONTEXT(self);
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
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_isInterface(PyJPClass *self, PyObject *arg)
{
	try
	{
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		return PyBool_FromLong(self->m_Class->isInterface());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_isThrowable(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Value.m_Context->m_Context;
		return PyBool_FromLong(self->m_Class->isThrowable());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_isPrimitive(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		return PyBool_FromLong((self->m_Class)->isPrimitive());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_isArray(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		return PyBool_FromLong(dynamic_cast<JPArrayClass*> (self->m_Class) == self->m_Class);
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_isAbstract(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		return PyBool_FromLong(self->m_Class->isAbstract());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_canConvertToJava(PyJPClass *self, PyObject *args)
{
	try
	{
		JP_TRACE_IN_C("PyJPClass_canConvertToJava", self);
		JPContext *context = PyJPValue_GET_CONTEXT(self);
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
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

// Added for auditing

PyObject *PyJPClass_convertToJava(PyJPClass *self, PyObject *args)
{
	try
	{
		JP_TRACE_IN_C("PyJPClass_convertToJava", self);
		PyJPModuleState *state = PyJPModuleState_global;
		JPContext *context = PyJPValue_GET_CONTEXT(self);
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
		return PyJPValue_create((PyTypeObject*) state->PyJPValue_Type, context, JPValue(cls, v)).keep();
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPClass_dumpCtor(PyJPClass *self, PyObject *args)
{
	try
	{
		JPContext *context = PyJPValue_GET_CONTEXT(self);
		JPJavaFrame frame(context);
		string report = self->m_Class->getCtor()->dump();
		return JPPyString::fromStringUTF8(report).keep();
	}
	PY_STANDARD_CATCH(NULL);
}

#ifdef __cplusplus
}
#endif
