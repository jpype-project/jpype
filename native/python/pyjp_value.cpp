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


PyObject *PyJPClassMeta_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int       PyJPClassMeta_init(PyObject *self, PyObject *pyargs, PyObject *kwargs);
void      PyJPClassMeta_dealloc(PyObject *self);
PyObject *PyJPClassMeta_getattro(PyObject *obj, PyObject *name);
int       PyJPClassMeta_setattro(PyObject *obj, PyObject *name, PyObject *value);
PyObject* PyJPClassMeta_check(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject *PyJPClassMeta_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClassMeta_new");
	// Call the Factory
	if (PyTuple_Size(args) == 1)
	{
		// We have to handle string, PyJPValue instances and point to a class
		PyObject *factory = PyObject_GetAttrString(PyJPModule_global, "ClassFactory");
		if (factory == NULL)
			return NULL;
		return PyObject_Call(factory, args, kwargs);
	}
	return PyType_Type.tp_new(type, args, kwargs);
	JP_PY_CATCH(NULL);
}

int PyJPClassMeta_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClassMeta_init", self);
	if (PyTuple_Size(args) == 1)
	{
		return 0;
	}
	return PyType_Type.tp_init(self, args, kwargs);
	JP_PY_CATCH(-1);
}

void PyJPClassMeta_dealloc(PyObject *self)
{
	JP_PY_TRY("PyJPClassMeta_dealloc");
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH();
}

PyObject *PyJPClassMeta_getattro(PyObject *obj, PyObject *name)
{
	JP_PY_TRY("PyJPClassMeta_getattro");
	PyJPModuleState *state = PyJPModuleState_global;
	if (!PyUnicode_Check(name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				Py_TYPE(name)->tp_name);
		return NULL;
	}
	Py_ssize_t sz;
	JP_TRACE(PyUnicode_AsUTF8AndSize(name, &sz));

	// Private members are accessed directly
	PyObject* pyattr = PyType_Type.tp_getattro(obj, name);
	if (pyattr == NULL)
		return NULL;
	JPPyObject attr(JPPyRef::_accept, pyattr);

	// Private members go regardless
	if (PyUnicode_GetLength(name) && PyUnicode_ReadChar(name, 0) == '_')
		return attr.keep();

	// Methods
	if (Py_TYPE(attr.get()) == (PyTypeObject*) state->PyJPMethod_Type)
		return attr.keep();

	// Don't allow properties to be rewritten
	if (!PyObject_IsInstance(attr.get(), (PyObject*) & PyProperty_Type))
		return attr.keep();

	const char *name_str = PyUnicode_AsUTF8(name);
	PyErr_Format(PyExc_AttributeError, "Field '%s' is static", name_str);
	return NULL;
	JP_PY_CATCH(NULL);
}

int PyJPClassMeta_setattro(PyObject *o, PyObject *attr_name, PyObject *v)
{
	JP_PY_TRY("PyJPClassMeta_setattro");
	if (!PyUnicode_Check(attr_name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				attr_name->ob_type->tp_name);
		return -1;
	}

	// Private members are accessed directly
	if (PyUnicode_GetLength(attr_name) && PyUnicode_ReadChar(attr_name, 0) == '_')
		return PyType_Type.tp_setattro(o, attr_name, v);

	PyObject *f = PyType_Lookup((PyTypeObject*) o, attr_name);
	if (f == NULL)
	{
		const char *name_str = PyUnicode_AsUTF8(attr_name);
		PyErr_Format(PyExc_AttributeError, "Field '%s' is not found", name_str);
		return -1;
	}

	int res;
	descrsetfunc desc = Py_TYPE(f)->tp_descr_set;
	if (desc != NULL)
	{
		res = desc(f, attr_name, v);
		Py_DECREF(f);
		return res;
	}
	Py_DECREF(f);

	// Not a descriptor
	const char *name_str = PyUnicode_AsUTF8(attr_name);
	PyErr_Format(PyExc_AttributeError,
			"Static field '%s' is not settable on Java '%s' object",
			name_str, Py_TYPE(o)->tp_name);
	return -1;
	JP_PY_CATCH(-1);
}

PyObject* PyJPClassMeta_check(PyObject *self, PyObject *args, PyObject *kwargs)
{
	PyJPModuleState *state = PyJPModuleState_global;

	int ret = 0;

	PyObject *other;
	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;

	PyObject *mro = Py_TYPE(other)->tp_mro;
	Py_ssize_t n = PyTuple_Size(mro);
	if (n >= 3)
	{
		ret = (PyTuple_GetItem(mro, n - 3) == state->PyJPClassMeta_Type);
	}

	return PyBool_FromLong(ret);
}

static struct PyMethodDef metaMethods[] = {
	{"_isinstance", (PyCFunction) & PyJPClassMeta_check, METH_VARARGS | METH_CLASS, ""},
	//	{"__instancecheck__", XXXX, METH_VARARGS, ""},
	//	{"__subclasscheck__", XXXX, METH_VARARGS, ""},
	{0}
};

static PyType_Slot metaSlots[] = {
	{Py_tp_new,      (void*) &PyJPClassMeta_new},
	{Py_tp_init,     (void*) &PyJPClassMeta_init},
	{Py_tp_dealloc,  (void*) &PyJPClassMeta_dealloc},
	{Py_tp_getattro, (void*) &PyJPClassMeta_getattro},
	{Py_tp_setattro, (void*) &PyJPClassMeta_setattro},
	{Py_tp_methods,  (void*) &metaMethods},
	{0}
};

PyType_Spec PyJPClassMetaSpec = {
	"_jpype.PyJPClassMeta",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	metaSlots
};

//============================================================

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
	if (!PyObject_IsInstance(type.get(), (PyObject*) state->PyJPClass_Type))
		JP_RAISE_TYPE_ERROR("__javaclass__ type is incorrect");

	JPClass *cls = ((PyJPClass*) type.get())->m_Class;
	JP_TRACE("type", cls->getCanonicalName());
	return PyJPValue_construct((PyJPValue*) inner, cls, pyargs, kwargs);
	JP_PY_CATCH(-1);
}

PyObject *PyJPValue_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValue_new");
	PyJPValue *self = (PyJPValue*) type->tp_alloc(type, 0);
	jvalue v;
	self->m_Value = JPValue(NULL, v);
	return (PyObject*) self;
	JP_PY_CATCH(NULL);
}

int PyJPValue_init(PyJPValue *self, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValue_init", self);
	PyJPModuleState *state = PyJPModuleState_global;

	// Access the context first so we ensure JVM is running
	PyJPModule_getContext();

	// Check if we are already initialized.
	if (self->m_Value.getClass() != 0)
		return 0;

	// Get the Java class from the type.
	JPPyObject type = JPPyObject(JPPyRef::_call,
			PyObject_GetAttrString((PyObject*) Py_TYPE(self), "__javaclass__"));
	if (!PyObject_IsInstance(type.get(), (PyObject*) state->PyJPClass_Type))
		JP_RAISE_TYPE_ERROR("__javaclass__ type is incorrect");

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

PyObject* PyJPValue_check(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValue_check", self);
	PyJPModuleState *state = PyJPModuleState_global;
	int ret = 0;

	PyObject *other;
	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;

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
 * Returns a borrowed reference.
 *
 * @param self
 * @return
 */
PyJPValue* PyJPValue_asValue(PyObject *self)
{
	PyJPModuleState *state = PyJPModuleState_global;
	PyObject* mro = Py_TYPE(self)->tp_mro;
	if (mro == NULL)
		return NULL;
	Py_ssize_t n = PyTuple_Size(mro);

	if (n >= 3 && PyTuple_GetItem(mro, n - 3) == state->PyJPValue_Type)
		return (PyJPValue*) self;
	if (n >= 2 && PyTuple_GetItem(mro, n - 2) == state->PyJPValueBase_Type)
	{
		PyObject* dict = PyObject_GenericGetDict(self, NULL);
		PyObject* proxy = PyDict_GetItemString(dict, __javavalue__); // borrowed
		Py_DECREF(dict);
		return (PyJPValue*) proxy;
	}
	return NULL;
}

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
	PyObject *f = PyType_Lookup(Py_TYPE(self), attr_name);
	if (f == NULL)
	{
		const char *name_str = PyUnicode_AsUTF8(attr_name);
		PyErr_Format(PyExc_AttributeError, "Field '%s' is not found", name_str);
		return -1;
	}
	descrsetfunc desc = Py_TYPE(f)->tp_descr_set;
	if (desc != NULL)
	{
		int res = desc(f, attr_name, value);
		Py_DECREF(f);
		return res;
	}
	Py_DECREF(f);

	// Not a descriptor
	const char *name_str = PyUnicode_AsUTF8(attr_name);
	PyErr_Format(PyExc_AttributeError,
			"Field '%s' is not settable on Java '%s' object", name_str, Py_TYPE(self)->tp_name);
	return -1;
	JP_PY_CATCH(-1);
}

PyObject *PyJPValue_str(PyObject *pyself)
{
	JP_PY_TRY("PyJPValue_toString", pyself);
	// Make this work for both PyJPValueBase and PyJPValue
	PyJPValue* self = PyJPValue_asValue(pyself);
	if (self == NULL)
		JP_RAISE_TYPE_ERROR("Must be Java value");

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
			JP_RAISE_VALUE_ERROR("null string");
		string cstring = frame.toStringUTF8(str);
		PyDict_SetItemString(dict.get(), "str", out = JPPyString::fromStringUTF8(cstring).keep());
		Py_INCREF(out);
		return out;
	}

	// In general toString is not immutable, so we won't cache it.
	return JPPyString::fromStringUTF8(frame.toString(self->m_Value.getValue().l)).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPValue_repr(PyObject *pyself)
{
	JP_PY_TRY("PyJPValue_repr", pyself);
	// Make this work for both PyJPValueBase and PyJPValue
	PyJPValue *self = PyJPValue_asValue(pyself);
	if (self == NULL)
		JP_RAISE_TYPE_ERROR("Must be Java value");

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
	{"_isinstance", (PyCFunction) & PyJPValueBase_check, METH_VARARGS | METH_CLASS, ""},
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
	{"_isinstance", (PyCFunction) & PyJPValue_check, METH_VARARGS | METH_CLASS, ""},
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

//=========================================================================

PyObject* PyJPException_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPException_new");
	return ((PyTypeObject*) PyExc_BaseException)->tp_new(type, args, kwargs);
	JP_PY_CATCH(NULL);
}

int PyJPException_init(PyObject *self, PyObject *pyargs, PyObject *kwargs)
{
	// This method is here to ensure that we hit the right area of the mro
	JP_PY_TRY("PyJPException_init", self);
	if (PyJPValueBase_init(self, pyargs, kwargs) == -1)
		return -1;
	return ((PyTypeObject*) PyExc_BaseException)->tp_init(self, pyargs, kwargs);
	JP_PY_CATCH(-1);
}

void PyJPException_dealloc(PyObject *self)
{
	JP_PY_TRY("PyJPException_dealloc", self);
	((PyTypeObject*) PyExc_BaseException)->tp_dealloc(self);
	JP_PY_CATCH();
}

int PyJPException_traverse(PyObject *self, visitproc visit, void *arg)
{
	JP_PY_TRY("PyJPException_traverse", self);
	return ((PyTypeObject*) PyExc_BaseException)->tp_traverse(self, visit, arg);
	JP_PY_CATCH(-1);
}

int PyJPException_clear(PyObject *self)
{
	JP_PY_TRY("PyJPException_clear", self);
	return ((PyTypeObject*) PyExc_BaseException)->tp_clear(self);
	JP_PY_CATCH(-1);
}

static PyType_Slot valueExcSlots[] = {
	{ Py_tp_new,      (void*) PyJPException_new},
	{ Py_tp_init,     (void*) PyJPException_init},
	{ Py_tp_dealloc,  (void*) PyJPException_dealloc},
	{ Py_tp_traverse, (void*) PyJPException_traverse},
	{ Py_tp_clear,    (void*) PyJPException_clear},
	{0}
};

PyType_Spec PyJPExceptionSpec = {
	"_jpype.PyJPException",
	0, // sizeof (PyBaseExceptionObject),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	valueExcSlots
};

#ifdef __cplusplus
}
#endif
