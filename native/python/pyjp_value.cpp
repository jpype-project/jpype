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
#include <structmember.h>

#ifdef __cplusplus
extern "C"
{
#endif

const char* JAVA_VALUE = "__javavalue__";

PyObject *PyJPValue_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);

PyObject* PyJPValueBase_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValueBase_new");
	return type->tp_alloc(type, 0);
	JP_PY_CATCH(NULL);
}

int PyJPValue_construct(PyJPValue *self, JPClass* cls, PyObject *pyargs, PyObject *kwargs)
{
	JPContext* context = PyJPModule_getContext();
	JPPyObjectVector args(pyargs);

	// Create an instance (this may fail)
	JPValue value = cls->newInstance(args);

	// If we succeed then we can hook up the context
	if (context != NULL && !cls->isPrimitive())
	{
		// Reference the object
		JP_TRACE("Reference object", cls->getCanonicalName());
		JPJavaFrame frame(context);
		value.getValue().l = frame.NewGlobalRef(value.getValue().l);
	}
	self->m_Value = value;
	return 0;
}

int PyJPValueBase_init(PyObject *self, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValueBase_init", self);
	// Attempt to create inner object first
	PyJPModuleState *state = PyJPModuleState_global;
	PyObject *inner = PyJPValue_new((PyTypeObject*) state->PyJPValue_Type, pyargs, kwargs);
	if (inner == NULL)
		return -1;
	PyObject_SetAttrString(self, __javavalue__, inner);

	// Access the context first so we ensure JVM is running
	PyJPModule_getContext();

	// Get the Java class from the type.
	JPPyObject type = JPPyObject(JPPyRef::_call,
			PyObject_GetAttrString((PyObject*) Py_TYPE(self), "__javaclass__"));
	if (!PyJPClass_Check(type.get()))
		JP_RAISE(PyExc_TypeError, "__javaclass__ type is incorrect");

	JPClass *cls = ((PyJPClass*) type.get())->m_Class;
	JP_TRACE("type", cls->getCanonicalName());
	return PyJPValue_construct((PyJPValue*) inner, cls, pyargs, kwargs);
	JP_PY_CATCH(-1);
}

PyObject *PyJPValue_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValue_new");
	PyJPValue *self = (PyJPValue*) type->tp_alloc(type, 0);
	self->m_Value = JPValue();
	return (PyObject*) self;
	JP_PY_CATCH(NULL);
}

int PyJPValue_init(PyJPValue *self, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValue_init", self);
	//	PyJPModuleState *state = PyJPModuleState_global;

	// Access the context first so we ensure JVM is running
	PyJPModule_getContext();

	// Check if we are already initialized.
	if (self->m_Value.getClass() != 0)
		return 0;

	// Get the Java class from the type.
	JPPyObject type = JPPyObject(JPPyRef::_call,
			PyObject_GetAttrString((PyObject*) Py_TYPE(self), "__javaclass__"));
	if (!PyJPClass_Check(type.get()))
		JP_RAISE(PyExc_TypeError, "__javaclass__ type is incorrect");

	JPClass *cls = ((PyJPClass*) type.get())->m_Class;
	return PyJPValue_construct(self, cls, pyargs, kwargs);
	JP_PY_CATCH(-1);
}

void PyJPValue_dealloc(PyJPValue *self)
{
	JP_PY_TRY("PyJPValue_dealloc", self);
	// We have to handle partially constructed objects that result from
	// fails in __init__, thus lots of inits
	JPValue& value = self->m_Value;
	JPClass *cls = value.getClass();

	PyJPModuleState *state = PyJPModuleState_global;
	JPContext* context = state->m_Context;
	if (context->isRunning() && cls != NULL && !cls->isPrimitive())
	{
		// If the JVM has shutdown then we don't need to free the resource
		// FIXME there is a problem with initializing the system twice.
		// Once we shut down the cls type goes away so this will fail.  If
		// we then reinitialize we will access this bad resource.  Not sure
		// of an easy solution.
		JP_TRACE("Dereference object", cls->getCanonicalName());
		context->ReleaseGlobalRef(value.getValue().l);
	}

	PyTypeObject *type = Py_TYPE(self);
	type->tp_free((PyObject*) self);
	JP_PY_CATCH();
}

/**
 * Check if it is a PyJPValueBase instance.
 *
 * @param self is the PyJPValueBase class.
 * @param other is the Python object to check.
 * @return true value if it derives from PyJPValueBase.
 */
PyObject* PyJPValueBase_check(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValueBase_check", self);
	PyJPModuleState *state = PyJPModuleState_global;
	int ret = 0;

	PyObject *other;
	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;

	PyObject *mro = Py_TYPE(other)->tp_mro;
	Py_ssize_t n = PyTuple_Size(mro);
	if (n >= 2)
		ret = PyTuple_GetItem(mro, n - 2) == state->PyJPValueBase_Type;

	return PyBool_FromLong(ret);
	JP_PY_CATCH(NULL);
}

/**
 * Check if it is a PyJPValue instance.
 *
 * @param self is the PyJPValue class.
 * @param other is the Python object to check.
 * @return true value if it derives from PyJPValue.
 */
PyObject* PyJPValue_check(PyObject *self, PyObject *other)
{
	JP_PY_TRY("PyJPValue_check", self);
	PyJPModuleState *state = PyJPModuleState_global;
	int ret = 0;
	PyObject *mro = Py_TYPE(other)->tp_mro;
	Py_ssize_t n = PyTuple_Size(mro);
	if (n >= 3)
	{
		ret = PyTuple_GetItem(mro, n - 3) == state->PyJPValue_Type;
	}

	return PyBool_FromLong(ret);
	JP_PY_CATCH(NULL);
}

//============================================================

// Type neutral methods

/**
 * Fetches a PyJPObject from a Python object.
 *
 * Uses the MRO order to check types.
 *
 * @param self is a Python object.
 * @return a borrowed reference to PyJPValue or NULL if not a PyJPValue type.
 */
PyJPValue* PyJPValue_asValue(PyObject *self)
{
	PyJPModuleState *state = PyJPModuleState_global;
	PyObject* mro = Py_TYPE(self)->tp_mro;
	if (mro == NULL)
		return NULL;
	Py_ssize_t n = PyTuple_Size(mro);

	// Check for a Java value
	if (n >= 3 && PyTuple_GetItem(mro, n - 3) == state->PyJPValue_Type)
		return (PyJPValue*) self;

	// Check for a pointer to a Java value
	if (n >= 2 && PyTuple_GetItem(mro, n - 2) == state->PyJPValueBase_Type)
	{
		PyObject* dict = PyObject_GenericGetDict(self, NULL);
		PyObject* proxy = PyDict_GetItemString(dict, __javavalue__); // borrowed
		Py_DECREF(dict);
		return (PyJPValue*) proxy;
	}
	return NULL;
}

/**
 * Set an attribute on a Java value.
 *
 * This enforced the closed nature of Java objects.  We cannot set any
 * attribute that is not defined as a field with the exception of private
 * Python fields.
 *
 * @param self is the Java value.
 * @param attr_name is the attribute name.
 * @param value
 * @return 0 on success, -1 otherwise.
 */
int PyJPValue_setattro(PyObject *self, PyObject *attr_name, PyObject *value)
{
	JP_PY_TRY("PyJPValue_setattro", self);
	if (!PyUnicode_Check(attr_name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				Py_TYPE(attr_name)->tp_name);
		return -1;
	}

	// Private members are accessed directly
	if (PyUnicode_GetLength(attr_name) && PyUnicode_ReadChar(attr_name, 0) == '_')
		return PyObject_GenericSetAttr(self, attr_name, value);
	JPPyObject f = JPPyObject(JPPyRef::_accept, PyType_Lookup(Py_TYPE(self), attr_name));
	if (f.isNull())
	{
		const char *name_str = PyUnicode_AsUTF8(attr_name);
		PyErr_Format(PyExc_AttributeError, "Field '%s' is not found", name_str);
		return -1;
	}
	descrsetfunc desc = Py_TYPE(f.get())->tp_descr_set;
	if (desc != NULL)
		return desc(f.get(), self, value);

	// Not a descriptor
	const char *name_str = PyUnicode_AsUTF8(attr_name);
	PyErr_Format(PyExc_AttributeError,
			"Field '%s' is not settable on Java '%s' object", name_str, Py_TYPE(self)->tp_name);
	return -1;
	JP_PY_CATCH(-1);
}

/**
 * Convert a Java value to Python string.
 *
 * This uses a cache for java.lang.String as they are immutable.
 * All others call to Java.  This is overridden by PyJPClass as it requires
 * a special location to deal with primitives.
 *
 * @param pyself is either an instance of PyJPValue or PyJPValueBase.
 * @return a new Python string or NULL on an error.
 */
PyObject *PyJPValue_str(PyObject *pyself)
{
	JP_PY_TRY("PyJPValue_toString", pyself);
	// Make this work for both PyJPValueBase and PyJPValue
	PyJPValue* self = PyJPValue_asValue(pyself);
	if (self == NULL)
		JP_RAISE(PyExc_TypeError, "Must be Java value");

	JPContext* context = PyJPModule_getContext();
	JPClass *cls = self->m_Value.getClass();
	if (cls == NULL)
		return JPPyString::fromStringUTF8("null").keep();

	// Special handling for primitives.
	if (cls->isPrimitive())
	{
		return JPPyString::fromStringUTF8(
				((JPPrimitiveType*) cls)->asString(self->m_Value.getValue())).keep();
	}

	JPJavaFrame frame(context);
	if (cls == context->_java_lang_String && Py_TYPE(self)->tp_dictoffset != 0)
	{
		// Java strings are immutable so we will cache them.
		PyObject *out;
		JPPyObject dict(JPPyRef::_call, PyObject_GenericGetDict((PyObject*) self, NULL));
		PyObject *res = PyDict_GetItemString(dict.get(), "__jstr__");
		if (res != NULL)
		{
			Py_INCREF(res);
			return res;
		}

		// Convert it
		jstring str = (jstring) self->m_Value.getValue().l;
		if (str == NULL)
			JP_RAISE(PyExc_ValueError, "null string");
		string cstring = frame.toStringUTF8(str);
		PyDict_SetItemString(dict.get(), "str", out = JPPyString::fromStringUTF8(cstring).keep());
		Py_INCREF(out);
		return out;
	}

	// In general toString is not immutable, so we won't cache it.
	return JPPyString::fromStringUTF8(frame.toString(self->m_Value.getValue().l)).keep();
	JP_PY_CATCH(NULL);
}

/**
 * Get a representation of a Java value.
 *
 * @param pyself is either an instance of PyJPValue or PyJPValueBase.
 * @return a new Python string or NULL on an error.
 */
PyObject *PyJPValue_repr(PyObject *pyself)
{
	JP_PY_TRY("PyJPValue_repr", pyself);
	// Make this work for both PyJPValueBase and PyJPValue
	PyJPValue *self = PyJPValue_asValue(pyself);
	if (self == NULL)
		JP_RAISE(PyExc_TypeError, "Must be Java value");

	PyJPModuleState *state = PyJPModuleState_global;
	JPContext* context = state->m_Context;
	if (!context->isRunning())
		return JPPyString::fromStringUTF8("<java value>").keep();
	JPJavaFrame frame(context);
	stringstream sout;
	sout << "<java value " << self->m_Value.getClass()->toString();

	// FIXME Remove these extra diagnostic values
	if (dynamic_cast<JPPrimitiveType*> (self->m_Value.getClass()) != NULL)
		sout << endl << "  value = primitive";
	else
	{
		jobject jo = self->m_Value.getJavaObject();
		sout << "  value = " << jo << " " << frame.toString(jo);
	}

	sout << ">";
	return JPPyString::fromStringUTF8(sout.str()).keep();
	JP_PY_CATCH(NULL);
}


static struct PyMethodDef baseMethods[] = {
	{"_check", (PyCFunction) & PyJPValueBase_check, METH_VARARGS | METH_CLASS, ""},
	{0}
};

static PyType_Slot baseSlots[] = {
	{Py_tp_new,      (void*) PyJPValueBase_new},
	{Py_tp_init,     (void*) PyJPValueBase_init},
	{Py_tp_setattro, (void*) PyJPValue_setattro},
	{Py_tp_str,      (void*) PyJPValue_str},
	{Py_tp_repr,     (void*) PyJPValue_repr},
	{Py_tp_methods,  (void*) &baseMethods},
	{0}
};

PyType_Spec PyJPValueBaseSpec = {
	"_jpype.PyJPValueBase",
	sizeof (PyObject),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	baseSlots
};

static struct PyMethodDef valueMethods[] = {
	{"_check", (PyCFunction) & PyJPValue_check, METH_O | METH_CLASS, ""},
	{0}
};

static PyType_Slot valueSlots[] = {
	{ Py_tp_new,      (void*) PyJPValue_new},
	{ Py_tp_init,     (void*) PyJPValue_init},
	{ Py_tp_dealloc,  (void*) PyJPValue_dealloc},
	{ Py_tp_methods,  (void*) &valueMethods},
	{0}
};

PyType_Spec PyJPValueSpec = {
	"_jpype.PyJPValue",
	sizeof (PyJPValue),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	valueSlots
};

#ifdef __cplusplus
}
#endif

/**
 * Create a JPValue wrapper with the appropriate type.
 *
 * This method dodges the __new__ method, but does involve
 * __init__.  It is called when returning a Java object back to
 * Python.
 *
 * @param type is the type of the resulting wrapper.
 * @param context is the java virtual machine.
 * @param value is the java value to wrap.
 * @return
 */
JPPyObject PyJPValue_create(PyTypeObject *type, JPContext *context, const JPValue& value)
{
	JP_TRACE_IN("PyJPValue_createInstance");
	PyJPModuleState *state = PyJPModuleState_global;
	// dispatch by type so we will create the right wrapper type
	JPPyObject out;
	JPClass* cls = value.getClass();

	if (type == (PyTypeObject *) state->PyJPValue_Type)
		out = PyJPValue_createInstance(type, context, value);
	else if (cls->isThrowable() || cls->isInterface())
		out = PyJPValue_createBase(type, context, value);
	else if (cls == context->_java_lang_Class)
	{
		JPClass *cls2 = context->getTypeManager()->findClass((jclass) value.getValue().l);
		out = PyJPClass_create(type, context, cls2);
	} else if (dynamic_cast<JPBoxedType*> (cls) != 0)
		out = PyJPValue_createBoxed(type, context, value);
	else if (dynamic_cast<JPArrayClass*> (cls) != 0)
		out = PyJPArray_create(type, context, value);
	else
		out = PyJPValue_createInstance(type, context, value);

	return out;
	JP_TRACE_OUT;
}

JPPyObject PyJPValue_createInstance(PyTypeObject *wrapper, JPContext *context, const JPValue& value)
{
	JP_TRACE_IN("PyJPValue_createInstance");

	if (value.getClass() == NULL && value.getValue().l != NULL)
		JP_RAISE(PyExc_RuntimeError, "Value inconsistency");

	PyJPValue *self = (PyJPValue*) ((PyTypeObject*) wrapper)->tp_alloc(wrapper, 0);
	JP_PY_CHECK();

	// If it is not a primitive, we need to reference it
	if (context != NULL && !value.getClass()->isPrimitive())
	{
		JPJavaFrame frame(context);
		self->m_Value = JPValue(value.getClass(), value.getValue().l).global(frame);
	} else
	{
		// New value instance
		self->m_Value = value;
	}
	return JPPyObject(JPPyRef::_claim, (PyObject*) self);
	JP_TRACE_OUT;
}

JPPyObject PyJPValue_createBase(PyTypeObject *wrapper, JPContext *context, const JPValue& value)
{
	PyJPModuleState *state = PyJPModuleState_global;
	JP_TRACE_IN("PyJPValue_createbase");
	JPPyObject self(JPPyRef::_claim, ((PyTypeObject*) wrapper)->tp_alloc(wrapper, 0));
	PyObject_SetAttrString(self.get(), __javavalue__,
			PyJPValue_createInstance((PyTypeObject*) state->PyJPValue_Type, context, value).get());
	return self;
	JP_TRACE_OUT;
}

JPPyObject PyJPValue_createBoxed(PyTypeObject *wrapper, JPContext *context, const JPValue& value)
{
	PyJPModuleState *state = PyJPModuleState_global;
	// Find the primitive type
	JPPrimitiveType *primitive = ((JPBoxedType*) value.getClass())->getPrimitive();

	// Convert object to primitive type
	JPValue jcontents = primitive->getValueFromObject(value);
	JPPyObject pycontents;

	// Convert to python based on the type
	if (primitive == context->_boolean)
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong(jcontents.getValue().z));
	else if (primitive == context->_byte)
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong(jcontents.getValue().b));
	else if (primitive == context->_short)
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong(jcontents.getValue().s));
	else if (primitive == context->_int)
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong(jcontents.getValue().i));
	else if (primitive == context->_long)
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong((long) jcontents.getValue().j));
	else if (primitive == context->_float)
		pycontents = JPPyObject(JPPyRef::_claim, PyFloat_FromDouble(jcontents.getValue().f));
	else if (primitive == context->_double)
		pycontents = JPPyObject(JPPyRef::_claim, PyFloat_FromDouble(jcontents.getValue().d));
	else if (primitive == context->_char)
		return PyJPValue_createInstance(wrapper, context, value);

	JPPyTuple tuple = JPPyTuple::newTuple(1);
	tuple.setItem(0, pycontents.get());
	JPPyObject self(JPPyRef::_call, PyObject_Call((PyObject*) wrapper, tuple.get(), NULL));
	PyObject_SetAttrString(self.get(), __javavalue__,
			PyJPValue_createInstance((PyTypeObject*) state->PyJPValue_Type, context, value).get());
	return self;
}
