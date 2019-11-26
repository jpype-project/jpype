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

/**
 * Search for a method in the tree.
 *
 * The regular method in Python always de-references the descriptor before
 * we can examine it.  We need to lookup it out without de-referencing it.
 * This will only searches the type dictionaries.
 *
 * @param type
 * @param attr_name
 * @return
 */
PyObject* PyType_Lookup(PyTypeObject *type, PyObject *attr_name)
{
	if (type->tp_mro == NULL)
		return NULL;

	PyObject *mro = type->tp_mro;
	Py_ssize_t n = PyTuple_Size(mro);
	for (Py_ssize_t i = 0; i < n; ++i)
	{
		type = (PyTypeObject*) PyTuple_GetItem(mro, i);
		PyObject *res = PyDict_GetItem(type->tp_dict, attr_name);
		if (res)
		{
			Py_INCREF(res);
			return res;
		}
	}
	return NULL;
}

//============================================================
PyObject *PyJPClassMeta_Type = NULL;
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
	{Py_tp_new, &PyJPClassMeta_new},
	{Py_tp_init, &PyJPClassMeta_init},
	{Py_tp_getattro, &PyJPClassMeta_getattro},
	{Py_tp_setattro, &PyJPClassMeta_setattro},
	{Py_tp_methods, &metaMethods},
	{0}
};

static PyType_Spec metaSpec = {
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
		PyObject *factory = PyObject_GetAttrString(PyJPModule::module, "ClassFactory");
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
	if (Py_TYPE(attr) == (PyTypeObject*) PyJPMethod_Type)
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
	int ret = 0;

	PyObject *other;
	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;

	PyObject *mro = Py_TYPE(other)->tp_mro;
	Py_ssize_t n = PyTuple_Size(mro);
	if (n >= 3)
	{
		ret = (PyTuple_GetItem(mro, n - 3) == PyJPClassMeta_Type);
	}

	return PyBool_FromLong(ret);
}

const char* JAVA_VALUE = "__javavalue__";

PyObject* PyJPValueBase_Type = NULL;
PyObject* PyJPValue_Type = NULL;
PyObject* PyJPValueExc_Type = NULL;
PyObject* PyJPValueLong_Type = NULL;
PyObject* PyJPValueFloat_Type = NULL;

JPPyObject PyJPValue_alloc(PyTypeObject *wrapper, JPContext *context, const JPValue& value);
JPPyObject PyJPValue_alloc_base(PyTypeObject *wrapper, JPContext *context, const JPValue& value);
JPPyObject PyJPValue_alloc_boxed(PyTypeObject *wrapper, JPContext *context, const JPValue& value);
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

static struct PyMethodDef baseGetSet[] = {
	{"__jvm__", (PyCFunction) PyJPValue_getJVM, NULL, ""},
	{0}
};

static PyType_Slot baseSlots[] = {
	{Py_tp_new, PyJPValueBase_new},
	{Py_tp_setattro, PyJPValue_setattro},
	{Py_tp_str, PyJPValue_str},
	{Py_tp_repr, PyJPValue_repr},
	{Py_tp_methods, &baseMethods},
	{Py_tp_getset, &baseGetSet},
	{0}
};

static PyType_Spec baseSpec = {
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
	{ Py_tp_new, PyJPValue_new},
	{ Py_tp_dealloc, PyJPValue_dealloc},
	{ Py_tp_traverse, PyJPValue_traverse},
	{ Py_tp_clear, PyJPValue_clear},
	{ Py_tp_methods, &valueMethods},
	{ Py_tp_members, &valueMembers},
	{0}
};

static PyType_Spec valueSpec = {
	"_jpype.PyJPValue",
	sizeof (PyJPValue),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
	valueSlots
};

static PyType_Slot valueExcSlots[] = {
	{0}
};

static PyType_Spec valueExcSpec = {
	"_jpype.PyJPValueException",
	sizeof (PyObject),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	valueExcSlots
};

//============================================================
// Factory for Base classes

JPPyObject PyJPArray_alloc(PyTypeObject *wrapper, JPContext *context, const JPValue& value);
JPPyObject PyJPClass_alloc(PyTypeObject *type, JPContext *context, JPClass *cls);

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
	// dispatch by type so we will create the right wrapper type
	JPPyObject out;
	JPClass* cls = value.getClass();

	if (type == (PyTypeObject *) PyJPValue_Type)
	{
		out = PyJPValue_alloc(type, context, value);
	} else if (cls->isThrowable())
	{
		out = PyJPValue_alloc_base(type, context, value);
	} else if (cls == context->_java_lang_Class)
	{
		JPClass *cls2 = context->getTypeManager()->findClass((jclass) value.getValue().l);
		out = PyJPClass_alloc(type, context, cls2);
	} else if (dynamic_cast<JPBoxedType*> (cls) != 0)
	{
		out = PyJPValue_alloc_boxed(type, context, value);
	} else if (dynamic_cast<JPArrayClass*> (cls) != 0)
	{
		out = PyJPArray_alloc(type, context, value);
	} else
	{
		out = PyJPValue_alloc(type, context, value);
	}

	return out;
}

JPPyObject PyJPValue_alloc(PyTypeObject *wrapper, JPContext *context, const JPValue& value)
{
	JPJavaFrame frame(context);
	JP_TRACE_IN_C("PyJPValue_alloc");

	PyJPValue *self = (PyJPValue*) ((PyTypeObject*) wrapper)->tp_alloc(wrapper, 0);
	JP_PY_CHECK();

	// If it is not a primitive, we need to reference it
	if (!value.getClass()->isPrimitive())
	{
		jvalue v;
		v.l = frame.NewGlobalRef(value.getValue().l);
		self->m_Value = JPValue(value.getClass(), v);
	} else
	{
		// New value instance
		self->m_Value = value;
	}
	self->m_Context = (PyJPContext*) (context->getHost());
	Py_INCREF(self->m_Context);
	return JPPyObject(JPPyRef::_claim, (PyObject*) self);
	JP_TRACE_OUT_C;
}

JPPyObject PyJPValue_alloc_base(PyTypeObject *wrapper, JPContext *context, const JPValue& value)
{
	JPJavaFrame frame(context);
	JP_TRACE_IN_C("PyJPValue_alloc_base");
	JPPyObject self(JPPyRef::_claim, ((PyTypeObject*) wrapper)->tp_alloc(wrapper, 0));
	self.setAttrString(JAVA_VALUE, PyJPValue_alloc((PyTypeObject*) PyJPValue_Type, context, value).get());
	return self;
	JP_TRACE_OUT_C;
}

JPPyObject PyJPValue_alloc_boxed(PyTypeObject *wrapper, JPContext *context, const JPValue& value)
{
	// Find the primitive type
	JPPrimitiveType *primitive = ((JPBoxedType*) value.getClass())->getPrimitive();

	// Convert object to primitive type
	JPValue jcontents = primitive->getValueFromObject(value);
	JPPyObject pycontents;

	// Convert to python based on the type
	if (primitive == context->_boolean)
	{
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong(jcontents.getValue().z));
	} else if (primitive == context->_byte)
	{
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong(jcontents.getValue().b));
	} else if (primitive == context->_short)
	{
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong(jcontents.getValue().s));
	} else if (primitive == context->_int)
	{
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong(jcontents.getValue().i));
	} else if (primitive == context->_long)
	{
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong((long) jcontents.getValue().j));
	} else if (primitive == context->_float)
	{
		pycontents = JPPyObject(JPPyRef::_claim, PyFloat_FromDouble(jcontents.getValue().f));
	} else if (primitive == context->_double)
	{
		pycontents = JPPyObject(JPPyRef::_claim, PyFloat_FromDouble(jcontents.getValue().d));
	} else if (primitive == context->_char)
	{
		return PyJPValue_alloc(wrapper, context, value);
	}

	JPPyTuple tuple = JPPyTuple::newTuple(1);
	tuple.setItem(0, pycontents.get());
	JPPyObject self(JPPyRef::_call, PyObject_Call((PyObject*) wrapper, tuple.get(), NULL));
	self.setAttrString(JAVA_VALUE, PyJPValue_alloc((PyTypeObject*) PyJPValue_Type, context, value).get());
	return self;
}

JPPyObject PyJPArray_alloc(PyTypeObject *wrapper, JPContext *context, const JPValue& value)
{
	JP_TRACE_IN_C("PyJPArray_alloc");
	JPPyObject self = PyJPValue_alloc(wrapper, context, value);
	((PyJPArray*) self.get())->m_Array = new JPArray(value.getClass(), (jarray) value.getValue().l);
	return self;
	JP_TRACE_OUT_C;
}

JPPyObject PyJPClass_alloc(PyTypeObject *wrapper, JPContext *context, JPClass *cls, jvalue value)
{
	JP_TRACE_IN_C("PyJPClass_alloc");
	JPJavaFrame frame(context);
	jvalue value;
	value.l = (jobject) cls->getJavaClass();
	JPPyObject self = PyJPValue_alloc(wrapper, context, JPValue(context->_java_lang_Class, value));
	((PyJPClass*) self.get())->m_Class = cls;
	return self;
	JP_TRACE_OUT_C;
}

//============================================================

PyObject* PyJPValueBase_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	PyObject *self = type->tp_alloc(type, 0);
	PyObject_SetAttrString(self, JAVA_VALUE,
			PyJPValue_new((PyTypeObject*) PyJPValue_Type, args, kwargs));
	return (PyObject*) self;
}

PyObject *PyJPValue_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	PyJPValue *self = (PyJPValue*) type->tp_alloc(type, 0);
	jvalue v;
	self->m_Value = JPValue(NULL, v);
	self->m_Context = NULL;
	return (PyObject*) self;
}

int PyJPValueBase_init(PyObject *self, PyObject *pyargs, PyObject *kwargs)
{
	JP_TRACE_IN_C("PyJPValue_init", self);
	JP_TRACE("init", self);
	try
	{
		// Check if we are already initialized.
		if (PyObject_HasAttrString(self, JAVA_VALUE))
			return 0;
		JPPyObject value(JPPyRef::_call,
				PyJPValue_new((PyTypeObject*) PyJPValue_Type, pyargs, kwargs));
		if (PyJPValue_init((PyJPValue*) value.get(), pyargs, kwargs) == -1)
			return -1;
		return PyObject_SetAttrString(self, JAVA_VALUE, value.get());
	}
	PY_STANDARD_CATCH(-1);
	JP_TRACE_OUT_C;
}

int PyJPValue_init(PyJPValue *self, PyObject *pyargs, PyObject *kwargs)
{
	JP_TRACE_IN_C("PyJPValue_init", self);
	JP_TRACE("init", self);
	try
	{
		// Check if we are already initialized.
		if (self->m_Value.getClass() != 0)
			return 0;

		// Get the Java class from the type.
		PyObject *obj = PyObject_GetAttrString((PyObject*) Py_TYPE(self), "__javaclass__");
		JP_PY_CHECK();
		if (!PyObject_IsInstance(obj, (PyObject*) & PyJPClass::Type))
		{
			Py_DECREF(obj);
			JP_RAISE_TYPE_ERROR("__javaclass__ type is incorrect");
		}
		JPClass *cls = ((PyJPClass*) obj)->m_Class;
		Py_DECREF(obj);

		if (dynamic_cast<JPArrayClass*> (cls) != NULL)
		{
			int sz;
			if (!PyArg_ParseTuple(pyargs, "i", &sz))
			{
				return NULL;
			}
			self->m_Value = ((JPArrayClass*) cls)->newInstance(sz);
			return 0;
		}

		JPPyObjectVector args(pyargs);
		// DEBUG
		for (size_t i = 0; i < args.size(); ++i)
		{
			ASSERT_NOT_NULL(args[i]);
		}
		self->m_Value =  cls->newInstance(args);
		return 0;
	}
	PY_STANDARD_CATCH(-1);
	JP_TRACE_OUT_C;
}

void PyJPValue_dealloc(PyJPValue *self)
{
	// We have to handle partially constructed objects that result from
	// fails in __init__, thus lots of inits
	JP_TRACE_IN_C("PyJPValue::__dealloc__", self);
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
	type->tp_clear((PyObject*) self);
	type->tp_free((PyObject*) self);
	JP_TRACE_OUT_C;
}

int PyJPValue_traverse(PyJPValue *self, visitproc visit, void *arg)
{
	JP_TRACE_IN_C("PyJPValue_traverse", self);
	Py_VISIT(self->m_Context);
	return 0;
	JP_TRACE_OUT_C;
}

int PyJPValue_clear(PyJPValue *self)
{
	JP_TRACE_IN_C("PyJPValue::clear", self);
	Py_CLEAR(self->m_Context);
	return 0;
	JP_TRACE_OUT_C;
}

//============================================================

// Test code

PyObject* PyJPValueBase_check(PyObject *self, PyObject *args, PyObject *kwargs)
{
	int ret = 0;

	PyObject *other;
	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;

	PyObject *mro = Py_TYPE(other)->tp_mro;
	Py_ssize_t n = PyTuple_Size(mro);
	if (n >= 2)
		ret = PyTuple_GetItem(mro, n - 2) == PyJPValueBase_Type;

	return PyBool_FromLong(ret);
}

// Test code

PyObject* PyJPValue_check(PyObject *self, PyObject *args, PyObject *kwargs)
{
	int ret = 0;

	PyObject *other;
	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;

	PyObject *mro = Py_TYPE(other)->tp_mro;
	Py_ssize_t n = PyTuple_Size(mro);
	if (n >= 3)
	{
		ret = PyTuple_GetItem(mro, n - 3) == PyJPValue_Type;
	}

	return PyBool_FromLong(ret);
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
	PyObject* mro = Py_TYPE(self)->tp_mro;
	if (mro == NULL)
		return NULL;
	Py_ssize_t n = PyTuple_Size(mro);

	if (n >= 3 && PyTuple_GetItem(mro, n - 3) == PyJPValue_Type)
		return (PyJPValue*) self;
	if (n >= 2 && PyTuple_GetItem(mro, n - 2) == PyJPValueBase_Type)
	{
		PyObject* dict = PyObject_GenericGetDict(self, NULL);
		PyObject* proxy = PyDict_GetItemString(dict, JAVA_VALUE); // borrowed
		Py_DECREF(dict);
		return (PyJPValue*) proxy;
	}
	return NULL;
}

int PyJPValue_setattro(PyObject *o, PyObject *attr_name, PyObject *v)
{
	JP_TRACE_IN_C("PyJPValue_toString", pyself);
	try
	{
		if (!PyUnicode_Check(attr_name))
		{
			PyErr_Format(PyExc_TypeError,
					"attribute name must be string, not '%.200s'",
					Py_TYPE(attr_name)->tp_name);
			return -1;
		}

		// Private members are accessed directly
		if (PyUnicode_GetLength(attr_name) && PyUnicode_ReadChar(attr_name, 0) == '_')
			return PyObject_GenericSetAttr(o, attr_name, v);
		PyObject *f = PyType_Lookup(Py_TYPE(o), attr_name);
		if (f == NULL)
		{
			const char *name_str = PyUnicode_AsUTF8(attr_name);
			PyErr_Format(PyExc_AttributeError, "Field '%s' is not found", name_str);
			return -1;
		}
		descrsetfunc desc = Py_TYPE(f)->tp_descr_set;
		if (desc != NULL)
		{
			int res = desc(f, attr_name, v);
			Py_DECREF(f);
			return res;
		}
		Py_DECREF(f);

		// Not a descriptor
		const char *name_str = PyUnicode_AsUTF8(attr_name);
		PyErr_Format(PyExc_AttributeError,
				"Field '%s' is not settable on Java '%s' object", name_str, Py_TYPE(o)->tp_name);
		return -1;
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPValue_str(PyObject *pyself)
{
	JP_TRACE_IN_C("PyJPValue_toString", pyself);
	try
	{
		// Make this work for both PyJPValueBase and PyJPValue
		PyJPValue* self = PyJPValue_asValue(pyself);
		if (self == NULL)
			JP_RAISE_TYPE_ERROR("Must be Java value");

		if (self->m_Context == NULL)
			JP_RAISE_RUNTIME_ERROR("Null context");

		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPClass *cls = self->m_Value.getClass();
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
			string cstring = context->toStringUTF8(str);
			PyDict_SetItemString(dict.get(), "str", out = JPPyString::fromStringUTF8(cstring).keep());
			Py_INCREF(out);
			return out;
		}

		if (cls->isPrimitive())
			JP_RAISE_VALUE_ERROR("toString requires a java object");
		if (cls == NULL)
			JP_RAISE_VALUE_ERROR("toString called with null class");

		// In general toString is not immutable, so we won't cache it.
		return JPPyString::fromStringUTF8(context->toString(self->m_Value.getValue().l)).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPValue_repr(PyObject *pyself)
{
	JP_TRACE_IN_C("PyJPValue:_repr", pyself);
	try
	{
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
			sout << "  value = " << jo << " " << context->toString(jo);
		}

		sout << ">";
		return JPPyString::fromStringUTF8(sout.str()).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPValue_getJVM(PyObject *pyself, void *closure)
{
	// Make this work for both PyJPValueBase and PyJPValue
	PyJPValue *self = PyJPValue_asValue(pyself);
	if (self->m_Context == NULL)
		Py_RETURN_NONE;
	Py_INCREF(self->m_Context);
	return (PyObject *) (self->m_Context);
}

//============================================================

void PyJPValue::initType(PyObject* module)
{
	PyObject *bases = PyTuple_Pack(1, &PyType_Type);
	PyModule_AddObject(module, "PyJPClassMeta",
			PyJPClassMeta_Type = PyType_FromSpecWithBases(&metaSpec, bases));
	Py_DECREF(bases);

	PyModule_AddObject(module, "PyJPValueBase",
			PyJPValueBase_Type = PyType_FromSpec(&baseSpec));

	bases = PyTuple_Pack(1, PyJPValueBase_Type);
	PyModule_AddObject(module, "PyJPValue",
			PyJPValue_Type = PyType_FromSpecWithBases(&valueSpec, bases));
	Py_DECREF(bases);

	bases = PyTuple_Pack(2, PyExc_Exception, PyJPValueBase_Type);
	PyModule_AddObject(module, "PyJPValueException",
			PyJPValueExc_Type = PyType_FromSpecWithBases(&valueExcSpec, bases));
	Py_DECREF(bases);

	// Int is hard so we need to use the regular type process
	PyObject *args;
	args = Py_BuildValue("sNN",
			"_jpype.PyJPValueLong",
			PyTuple_Pack(2, &PyLong_Type, PyJPValueBase_Type),
			PyDict_New());
	PyModule_AddObject(module, "PyJPValueLong",
			PyJPValueInt_Type = PyObject_Call((PyObject*) & PyType_Type, args, NULL));
	Py_DECREF(args);

	args = Py_BuildValue("sNN",
			"_jpype.PyJPValueFloat",
			PyTuple_Pack(2, &PyLong_Type, PyJPValueBase_Type),
			PyDict_New());
	PyModule_AddObject(module, "PyJPValueFloat",
			PyJPValueInt_Type = PyObject_Call((PyObject*) & PyType_Type, args, NULL));
	Py_DECREF(args);
}


