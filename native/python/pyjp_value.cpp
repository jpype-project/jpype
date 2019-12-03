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
PyObject *PyJPClassMeta_getattro(PyObject *obj, PyObject *name);
int       PyJPClassMeta_setattro(PyObject *obj, PyObject *name, PyObject *value);
PyObject* PyJPClassMeta_check(PyObject *self, PyObject *args, PyObject *kwargs);

static struct PyMethodDef metaMethods[] = {
	{"_isinstance", (PyCFunction) & PyJPClassMeta_check, METH_VARARGS | METH_CLASS, ""},
	//	{"__instancecheck__", XXXX, METH_VARARGS, ""},
	//	{"__subclasscheck__", XXXX, METH_VARARGS, ""},
	{0}
};

static PyType_Slot metaSlots[] = {
	{Py_tp_new,      (void*) &PyJPClassMeta_new},
	{Py_tp_init,     (void*) &PyJPClassMeta_init},
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

PyObject *PyJPClassMeta_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
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
}

int PyJPClassMeta_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
	if (PyTuple_Size(args) == 1)
	{
		return 0;
	}
	return PyType_Type.tp_init(self, args, kwargs);
}

PyObject *PyJPClassMeta_getattro(PyObject *obj, PyObject *name)
{
	PyJPModuleState *state = PyJPModuleState_global;
	return PyType_Type.tp_getattro(obj, name);
	if (!PyUnicode_Check(name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				Py_TYPE(name)->tp_name);
		return NULL;
	}

	// Private members are accessed directly
	PyObject *attr = Py_TYPE(obj)->tp_getattro(obj, name);
	if (attr == NULL)
		return NULL;

	// Private members go regardless
	if (PyUnicode_GetLength(name) && PyUnicode_ReadChar(name, 0) == '_')
		return attr;

	// Methods
	if (Py_TYPE(attr) == (PyTypeObject*) state->PyJPMethod_Type)
		return attr;

	// Don't allow properties to be rewritten
	if (!PyObject_IsInstance(attr, (PyObject*) & PyProperty_Type))
		return attr;

	const char *name_str = PyUnicode_AsUTF8(name);
	PyErr_Format(PyExc_AttributeError, "Field '%s' is static", name_str);
	Py_DECREF(attr);
	return NULL;
}

int PyJPClassMeta_setattro(PyObject *o, PyObject *attr_name, PyObject *v)
{
	if (!PyUnicode_Check(attr_name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				attr_name->ob_type->tp_name);
		return -1;
	}

	// Private members are accessed directly
	if (PyUnicode_GetLength(attr_name) && PyUnicode_ReadChar(attr_name, 0) == '_')
		return Py_TYPE(o)->tp_setattro(o, attr_name, v);

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

const char* JAVA_VALUE = "__javavalue__";

PyObject *PyJPValueBase_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
PyObject *PyJPValue_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int PyJPValue_init(PyJPValue *self, PyObject *pyargs, PyObject *kwargs);
void PyJPValue_dealloc(PyJPValue *self);
int PyJPValue_clear(PyJPValue *self);
int PyJPValue_traverse(PyJPValue *self, visitproc visit, void *arg);
int PyJPValue_setattro(PyObject *o, PyObject *attr_name, PyObject *v);
PyObject* PyJPValueBase_check(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* PyJPValue_check(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *PyJPValue_str(PyObject *self);
PyObject *PyJPValue_repr(PyObject *self);
PyObject *PyJPValue_getJVM(PyObject *pyself, void *closure);

//============================================================

static struct PyMethodDef baseMethods[] = {
	{"_isinstance", (PyCFunction) & PyJPValueBase_check, METH_VARARGS | METH_CLASS, ""},
	{0}
};

static struct PyGetSetDef baseGetSet[] = {
	{"__jvm__", (getter) PyJPValue_getJVM, NULL, "the Java context for this object"},
	{0}
};

static PyType_Slot baseSlots[] = {
	{Py_tp_new,      (void*) PyJPValueBase_new},
	{Py_tp_init,     (void*) PyJPValue_init},
	{Py_tp_setattro, (void*) PyJPValue_setattro},
	{Py_tp_str,      (void*) PyJPValue_str},
	{Py_tp_repr,     (void*) PyJPValue_repr},
	{Py_tp_methods,  (void*) &baseMethods},
	{Py_tp_getset,   (void*) &baseGetSet},
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

static PyMemberDef valueMembers[] = {
	{"__jvm__", T_OBJECT, offsetof(PyJPValue, m_Context), READONLY},
	{0}
};

static PyType_Slot valueSlots[] = {
	{ Py_tp_new,      (void*) PyJPValue_new},
	{ Py_tp_init,     (void*) PyJPValue_init},
	{ Py_tp_dealloc,  (void*) PyJPValue_dealloc},
	{ Py_tp_traverse, (void*) PyJPValue_traverse},
	{ Py_tp_clear,    (void*) PyJPValue_clear},
	{ Py_tp_methods,  (void*) &valueMethods},
	{ Py_tp_members,  (void*) &valueMembers},
	{0}
};

PyType_Spec PyJPValueSpec = {
	"_jpype.PyJPValue",
	sizeof (PyJPValue),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC ,
	valueSlots
};

static PyType_Slot valueExcSlots[] = {
	{0}
};

PyType_Spec PyJPValueExceptionSpec = {
	"_jpype.PyJPValueException",
	sizeof (PyObject),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	valueExcSlots
};


//============================================================

PyObject* PyJPValueBase_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValueBase_new")
	PyJPModuleState *state = PyJPModuleState_global;
	PyObject *inner = PyJPValue_new((PyTypeObject*) state->PyJPValue_Type, args, kwargs);
	if (inner == NULL)
		return NULL;
	PyObject *self = type->tp_alloc(type, 0);
	PyObject_SetAttrString(self, __javavalue__, inner);
	return (PyObject*) self;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPValue_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValue_new")
	PyJPValue *self = (PyJPValue*) type->tp_alloc(type, 0);
	jvalue v;
	self->m_Value = JPValue(NULL, v);
	self->m_Context = NULL;
	return (PyObject*) self;
	JP_PY_CATCH(NULL);
}

int PyJPValueBase_init(PyObject *self, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValue_init", self)
	PyJPModuleState *state = PyJPModuleState_global;
	// Check if we are already initialized.
	if (PyObject_HasAttrString(self, __javavalue__))
		return 0;
	JPPyObject value(JPPyRef::_call,
			PyJPValue_new((PyTypeObject*) state->PyJPValue_Type, pyargs, kwargs));
	if (PyJPValue_init((PyJPValue*) value.get(), pyargs, kwargs) == -1)
		return -1;
	return PyObject_SetAttrString(self, __javavalue__, value.get());
	JP_PY_CATCH(-1);
}

int PyJPValue_init(PyJPValue *self, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValue_init", self)
	PyJPModuleState *state = PyJPModuleState_global;

	// Check if we are already initialized.
	if (self->m_Value.getClass() != 0)
		return 0;

	// Get the Java class from the type.
	JPPyObject obj = JPPyObject(JPPyRef::_claim,
			PyObject_GetAttrString((PyObject*) Py_TYPE(self), "__javaclass__"));
	JP_PY_CHECK();
	if (!PyObject_IsInstance(obj.get(), (PyObject*) state->PyJPClass_Type))
		JP_RAISE_TYPE_ERROR("__javaclass__ type is incorrect");

	// We must verify the context before we can access the class
	// as the class may already be dead.
	PyJPValue_GET_CONTEXT(obj.get());
	JPClass *cls = ((PyJPClass*) obj.get())->m_Class;
	PyJPContext *context = ((PyJPClass*) obj.get())->m_Value.m_Context;

	JPPyObjectVector args(pyargs);
	// DEBUG
	for (size_t i = 0; i < args.size(); ++i)
	{
		ASSERT_NOT_NULL(args[i]);
	}

	// Create an instance (this may fail)
	JPValue value = cls->newInstance(args);

	// If we succeed then we can hook up the context
	self->m_Context = context;
	if (context != NULL)
	{
		// Reference the object
		JP_TRACE("Reference object", cls->getCanonicalName());
		Py_INCREF(context);
		JPJavaFrame frame(context->m_Context);
		value.getValue().l = frame.NewGlobalRef(value.getValue().l);
	}
	self->m_Value = value;
	return 0;
	JP_PY_CATCH(-1);
}

void PyJPValue_dealloc(PyJPValue *self)
{
	JP_PY_TRY("PyJPValue_dealloc", self)
	// We have to handle partially constructed objects that result from
	// fails in __init__, thus lots of inits
	JPValue& value = self->m_Value;
	JPClass *cls = value.getClass();
	if (self->m_Context != NULL && cls != NULL)
	{
		JPContext *context = self->m_Context->m_Context;
		if (context->isRunning() && !cls->isPrimitive())
		{
			// If the JVM has shutdown then we don't need to free the resource
			// FIXME there is a problem with initializing the system twice.
			// Once we shut down the cls type goes away so this will fail.  If
			// we then reinitialize we will access this bad resource.  Not sure
			// of an easy solution.
			JP_TRACE("Dereference object", cls->getCanonicalName());
			context->ReleaseGlobalRef(value.getValue().l);
		}
	}

	PyTypeObject *type = Py_TYPE(self);
	PyObject_GC_UnTrack(self);
	PyJPValue_clear(self);
	type->tp_free((PyObject*) self);
	JP_PY_CATCH();
}

int PyJPValue_traverse(PyJPValue *self, visitproc visit, void *arg)
{
	JP_PY_TRY("PyJPValue_traverse", self)
	Py_VISIT(self->m_Context);
	return 0;
	JP_PY_CATCH(-1);
}

int PyJPValue_clear(PyJPValue *self)
{
	JP_PY_TRY("PyJPValue_clear", self)
	Py_CLEAR(self->m_Context);
	return 0;
	JP_PY_CATCH(-1);
}

//============================================================

// Test code

PyObject* PyJPValueBase_check(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValueBase_check", self)
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

// Test code

PyObject* PyJPValue_check(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValue_check", self)
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

	JP_PY_TRY("PyJPValue_toString", self)
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
	JP_PY_TRY("PyJPValue_toString", pyself)
	// Make this work for both PyJPValueBase and PyJPValue
	PyJPValue* self = PyJPValue_asValue(pyself);
	if (self == NULL)
		JP_RAISE_TYPE_ERROR("Must be Java value");

	JPClass *cls = self->m_Value.getClass();
	if (cls == NULL)
		JP_RAISE_VALUE_ERROR("toString called with null class");

	// Special handling for primitives.
	if (self->m_Context == NULL)
	{
		if (cls->isPrimitive())
		{
			JPPrimitiveType *pc = (JPPrimitiveType*) cls;
			return JPPyString::fromStringUTF8(pc->asString(self->m_Value.getValue())).keep();
		}
		JP_RAISE_RUNTIME_ERROR("Null context");
	}
	JPContext *context = self->m_Context->m_Context;
	ASSERT_JVM_RUNNING(context);
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
	JP_PY_TRY("PyJPValue_repr", pyself)
	// Make this work for both PyJPValueBase and PyJPValue
	PyJPValue *self = PyJPValue_asValue(pyself);
	if (self == NULL)
		JP_RAISE_TYPE_ERROR("Must be Java value");

	if (self->m_Context == NULL)
		JP_RAISE_RUNTIME_ERROR("Null context");

	JPContext * context = self->m_Context->m_Context;
	ASSERT_JVM_RUNNING(context);
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

PyObject *PyJPValue_getJVM(PyObject *pyself, void *closure)
{
	JP_PY_TRY("PyJPValue_getJVM", pyself)
	// Make this work for both PyJPValueBase and PyJPValue
	PyJPValue *self = PyJPValue_asValue(pyself);
	if (self->m_Context == NULL)
		Py_RETURN_NONE;
	Py_INCREF(self->m_Context);
	return (PyObject *) (self->m_Context);
	JP_PY_CATCH(NULL);
}

#ifdef __cplusplus
}
#endif
