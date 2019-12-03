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
void PyJPClass_dealloc(PyObject *self);

PyObject *PyJPClass_getBases(PyJPClass *self, void *closure);
PyObject *PyJPClass_getCanonicalName(PyJPClass *self, void *closure);
PyObject *PyJPClass_getClassFields(PyJPClass *self, void *closure);
PyObject *PyJPClass_getClassMethods(PyJPClass *self, void *closure);

PyObject *PyJPClass_cast(PyJPClass *self, PyObject *args);

// Query
PyObject *PyJPClass_isAssignableFrom(PyJPClass *self, PyObject *arg);
PyObject *PyJPClass_isInterface(PyJPClass *self, PyObject *arg);
PyObject *PyJPClass_isThrowable(PyJPClass *self, PyObject *args);
PyObject *PyJPClass_isPrimitive(PyJPClass *self, PyObject *args);
PyObject *PyJPClass_isArray(PyJPClass *self, PyObject *args);
PyObject *PyJPClass_isAbstract(PyJPClass *self, PyObject *args);

// Debugging
PyObject *PyJPClass_canConvertToJava(PyJPClass *self, PyObject *args);
PyObject *PyJPClass_convertToJava(PyJPClass *self, PyObject *args);
PyObject *PyJPClass_dumpCtor(PyJPClass *self, PyObject *args);

const char *classDoc =
		"Internal representation of a Java Class.  This class can represent\n"
		"either an object, an array, or a primitive.  This type is stored as\n"
		"__javaclass__ in any wrapper Python classes or instances.";

static PyMethodDef classMethods[] = {
	{"_cast", (PyCFunction) (&PyJPClass_cast), METH_VARARGS, ""},
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

static PyGetSetDef classGetSets[] = {
	{"__name__", (getter) (&PyJPClass_getCanonicalName), NULL, ""},
	{"_fields", (getter) (&PyJPClass_getClassFields), NULL, ""},
	{"_methods", (getter) (&PyJPClass_getClassMethods), NULL, ""},
	{"_bases", (getter) (&PyJPClass_getBases), NULL, ""},
	{0}
};

static PyType_Slot classSlots[] = {
	{ Py_tp_new,     (void*) PyJPClass_new},
	{ Py_tp_init,    (void*) PyJPClass_init},
	{ Py_tp_dealloc, (void*) PyJPClass_dealloc},
	{ Py_tp_methods, (void*) classMethods},
	{ Py_tp_doc,     (void*) classDoc},
	{ Py_tp_getset,  (void*) &classGetSets},
	{0}
};

PyType_Spec PyJPClassSpec = {
	"_jpype.PyJPClass",
	sizeof (PyJPClass),
	0,
	Py_TPFLAGS_DEFAULT  | Py_TPFLAGS_BASETYPE, //| Py_TPFLAGS_HAVE_GC
	classSlots
};

PyObject *PyJPClass_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClass_new", type)
	PyJPModuleState *state = PyJPModuleState_global;
	PyTypeObject *base = (PyTypeObject *) state->PyJPValue_Type;
	PyObject *self = base->tp_new(type, args, kwargs);
	((PyJPClass*) self)->m_Class = NULL;
	return self;
	JP_PY_CATCH(NULL);
}

// FIXME, not clear which should win.  The JVM specified or the Class JVM
// thus let's make it an error for now.
// (jvm, str)

int PyJPClass_init(PyJPClass *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClass_init", self)
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
	//JPValue *jpvalue = JPPythonEnv::getJavaValue(arg0);
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
	JP_PY_CATCH(-1);
}

void PyJPClass_dealloc(PyObject *self)
{
	JP_PY_TRY("PyJPClass_dealloc", self)
	PyTypeObject *type = Py_TYPE(self);
	type->tp_base->tp_dealloc(self);
	JP_PY_CATCH();
}

PyObject *PyJPClass_getCanonicalName(PyJPClass *self, void *closure)
{
	JP_PY_TRY("PyJPClass_getCanonicalName", self)
	PyJPValue_GET_CONTEXT(self);
	string name = self->m_Class->getCanonicalName();
	PyObject *res = JPPyString::fromStringUTF8(name).keep();
	return res;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPClass_getBases(PyJPClass *self, void *closure)
{
	JP_PY_TRY("PyJPClass_getBases", self)
	JPContext *context = PyJPValue_GET_CONTEXT(self);
	if (context == NULL)
		Py_RETURN_NONE;
	JPJavaFrame frame(context);

	// Decide the base for this object
	JPPyObject baseType;
	PyJPModuleState *state = PyJPModuleState_global;
	JPClass *super = self->m_Class->getSuperClass();
	if (self->m_Class == context->_java_lang_Object)
	{
		baseType = JPPyObject(JPPyRef::_use, state->PyJPValue_Type);
	}
	if (self->m_Class == context->_java_lang_Class)
	{
		baseType = JPPyObject(JPPyRef::_use, state->PyJPClass_Type);
	} else if (self->m_Class == context->_java_lang_Throwable)
	{
		baseType = JPPyObject(JPPyRef::_use, state->PyJPValueExc_Type);
	} else if (dynamic_cast<JPArrayClass*> (self->m_Class) == self->m_Class)
	{
		baseType = JPPyObject(JPPyRef::_claim,
				PyObject_GetAttrString(PyJPModule_global, "_JArrayBase"));
	} else if (self->m_Class->isPrimitive())
	{
		JP_RAISE_TYPE_ERROR("primitives are not objects");
	} else if (self->m_Class == context->_java_lang_Boolean ||
			self->m_Class == context->_java_lang_Byte ||
			self->m_Class == context->_java_lang_Short ||
			self->m_Class == context->_java_lang_Integer ||
			self->m_Class == context->_java_lang_Long)
	{
		super = NULL;
		baseType = JPPyObject(JPPyRef::_use, state->PyJPValueLong_Type);
	} else if (self->m_Class == context->_java_lang_Float ||
			self->m_Class == context->_java_lang_Double)
	{
		super = NULL;
		baseType = JPPyObject(JPPyRef::_use, state->PyJPValueFloat_Type);
	} else if (self->m_Class->isInterface())
	{
		baseType = JPPyObject(JPPyRef::_use, state->PyJPValueBase_Type);
	}

	const JPClassList& baseItf = self->m_Class->getInterfaces();
	int count = baseItf.size() + (!baseType.isNull() ? 1 : 0) + (super != NULL ? 1 : 0);

	// Pack into a tuple
	JPPyTuple result(JPPyTuple::newTuple(count));
	unsigned int i = 0;
	for (; i < baseItf.size(); i++)
	{
		result.setItem(i, JPPythonEnv::newJavaClass(baseItf[i]).get());
	}
	if (super != NULL)
	{
		result.setItem(i++, JPPythonEnv::newJavaClass(super).get());
	}
	if (!baseType.isNull())
	{
		result.setItem(i++, baseType.get());
	}
	return result.keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPClass_getClassFields(PyJPClass *self, void *closure)
{
	JP_PY_TRY("PyJPClass_getClassFields", self)
	// Special case for primitives
	JPContext *context = PyJPValue_GET_CONTEXT(self);
	if (context == NULL)
		Py_RETURN_NONE;
	JPJavaFrame frame(context);

	int i = 0;
	const JPFieldList& instFields = self->m_Class->getFields();
	JPPyTuple result(JPPyTuple::newTuple(instFields.size()));
	for (JPFieldList::const_iterator iter = instFields.begin(); iter != instFields.end(); iter++)
	{
		result.setItem(i++, PyJPField_create(*iter).get());
	}
	return result.keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPClass_getClassMethods(PyJPClass *self, void *closure)
{
	JP_PY_TRY("PyJPClass_getClassMethods", self)
	JPContext *context = PyJPValue_GET_CONTEXT(self);
	if (context == NULL)
		Py_RETURN_NONE;
	JPJavaFrame frame(context);

	// Boxed and Throwable are special cases that require the object
	// methods copied in as well as their own.
	const JPMethodDispatchList& m_Methods = self->m_Class->getMethods();
	size_t count = m_Methods.size();
	if (self->m_Class == context->_java_lang_Throwable ||
			dynamic_cast<JPBoxedType*> (self->m_Class) != 0)
	{
		count += context->_java_lang_Object->getMethods().size();
	}

	// Add the methods of the class
	int i = 0;
	JPPyTuple result(JPPyTuple::newTuple(count));
	for (JPMethodDispatchList::const_iterator cur = m_Methods.begin(); cur != m_Methods.end(); cur++)
	{
		result.setItem(i++, PyJPMethod_create(*cur, NULL).get());
	}

	// Add the object methods if not inherited
	if (count != m_Methods.size())
	{
		const JPMethodDispatchList& m_ObjectMethods = context->_java_lang_Object->getMethods();
		for (JPMethodDispatchList::const_iterator cur = m_ObjectMethods.begin(); cur != m_ObjectMethods.end(); cur++)
		{
			result.setItem(i++, PyJPMethod_create(*cur, NULL).get());
		}
	}
	return result.keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPClass_cast(PyJPClass *self, PyObject *args)
{
	JP_PY_TRY("PyJPClass_cast", self)
	JPContext *context = PyJPValue_GET_CONTEXT(self);
	if (context == NULL)
		Py_RETURN_NONE;
	JPJavaFrame frame(context);

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
	if (jval != NULL && type->isInstance(frame, *jval))
	{
		return PyJPValue_create((PyTypeObject*) wrapper.get(), context, JPValue(type, jval->getValue()))
				.keep();
	}

	// Otherwise, see if we can convert it
	{
		JPJavaFrame frame(context);
		JPMatch match;
		type->getJavaConversion(&frame, match, value);
		if (match.type == JPMatch::_none)
		{
			stringstream ss;
			ss << "Unable to convert " << Py_TYPE(value)->tp_name << " to java type " << type->toString();
			PyErr_SetString(PyExc_TypeError, ss.str().c_str());
			return 0;
		}

		jvalue v = match.conversion->convert(&frame, type, value);
		return PyJPValue_create((PyTypeObject*) wrapper.get(), context, JPValue(type, v)).keep();
	}
	JP_PY_CATCH(NULL);
}

PyObject *PyJPClass_isAssignableFrom(PyJPClass *self, PyObject *arg)
{
	JP_PY_TRY("PyJPClass_isAssignableFrom", self)
	JPContext *context = PyJPValue_GET_CONTEXT(self);
	if (context == NULL)
		Py_RETURN_NONE;
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
		return PyBool_FromLong(self->m_Class->isAssignableFrom(frame, cls));
	}

	if (JPPyString::check(other))
	{
		JPClass *otherClass = context->getTypeManager()
				->findClassByName(JPPyString::asStringUTF8(other));
		return PyBool_FromLong(self->m_Class->isAssignableFrom(frame, otherClass));
	}

	PyErr_SetString(PyExc_TypeError, "isAssignableFrom requires java class or string argument.");
	return NULL;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPClass_isInterface(PyJPClass *self, PyObject *arg)
{
	PyJPValue_GET_CONTEXT(self);
	return PyBool_FromLong(self->m_Class->isInterface());
}

PyObject *PyJPClass_isThrowable(PyJPClass *self, PyObject *args)
{
	PyJPValue_GET_CONTEXT(self);
	return PyBool_FromLong(self->m_Class->isThrowable());
}

PyObject *PyJPClass_isPrimitive(PyJPClass *self, PyObject *args)
{
	PyJPValue_GET_CONTEXT(self);
	return PyBool_FromLong((self->m_Class)->isPrimitive());
}

PyObject *PyJPClass_isArray(PyJPClass *self, PyObject *args)
{
	PyJPValue_GET_CONTEXT(self);
	return PyBool_FromLong(dynamic_cast<JPArrayClass*> (self->m_Class) == self->m_Class);
}

PyObject *PyJPClass_isAbstract(PyJPClass *self, PyObject *args)
{
	PyJPValue_GET_CONTEXT(self);
	return PyBool_FromLong(self->m_Class->isAbstract());
}

PyObject *PyJPClass_canConvertToJava(PyJPClass *self, PyObject *args)
{
	JP_PY_TRY("PyJPClass_canConvertToJava", self);
	JPContext *context = PyJPValue_GET_CONTEXT(self);
	if (context == NULL)
		Py_RETURN_NONE;

	JPJavaFrame frame(context);

	PyObject *other;
	if (!PyArg_ParseTuple(args, "O", &other))
	{
		return NULL;
	}
	JPClass *cls = self->m_Class;

	// Test the conversion
	JPMatch match;
	cls->getJavaConversion(&frame, match, other);

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
	JP_PY_CATCH(NULL);
}

// Added for auditing

PyObject *PyJPClass_convertToJava(PyJPClass *self, PyObject *args)
{
	JP_PY_TRY("PyJPClass_convertToJava", self)
	PyJPModuleState *state = PyJPModuleState_global;
	JPContext *context = PyJPValue_GET_CONTEXT(self);
	if (context == NULL)
		Py_RETURN_NONE;
	JPJavaFrame frame(context);

	PyObject *other;
	if (!PyArg_ParseTuple(args, "O", &other))
	{
		return NULL;
	}
	JPClass *cls = self->m_Class;

	// Test the conversion
	JPMatch match;
	cls->getJavaConversion(&frame, match, other);

	// If there is no conversion report a failure
	if (match.type == JPMatch::_none)
	{
		PyErr_SetString(PyExc_TypeError, "Unable to create an instance.");
		return 0;
	}

	// Otherwise give back a PyJPValue
	jvalue v = match.conversion->convert(&frame, cls, other);
	return PyJPValue_create((PyTypeObject*) state->PyJPValue_Type, context, JPValue(cls, v)).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPClass_dumpCtor(PyJPClass *self, PyObject *args)
{
	JP_PY_TRY("PyJPClass_dumpCtor", self)
	JPContext *context = PyJPValue_GET_CONTEXT(self);
	if (context == NULL)
		Py_RETURN_NONE;
	JPJavaFrame frame(context);
	string report = self->m_Class->getCtor()->dump();
	return JPPyString::fromStringUTF8(report).keep();
	JP_PY_CATCH(NULL);
}

#ifdef __cplusplus
}
#endif