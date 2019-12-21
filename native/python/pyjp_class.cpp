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

const char *classDoc =
		"Internal representation of a Java Class.  This class can represent\n"
		"either an object, an array, or a primitive.  This type is stored as\n"
		"__javaclass__ in any wrapper Python classes or instances.";

PyObject *PyJPClass_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClass_new", type);
	PyJPModuleState *state = PyJPModuleState_global;
	PyTypeObject *base = (PyTypeObject *) state->PyJPValue_Type;
	PyObject *self = base->tp_new(type, args, kwargs);
	((PyJPClass*) self)->m_Class = NULL;
	return self;
	JP_PY_CATCH(NULL);
}

int PyJPClass_init(PyJPClass *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClass_init", self);
	// Check if we are already initialized.
	if ((PyJPValue*) self->m_Class != 0)
		return 0;

	PyObject *arg0;

	if (!PyArg_ParseTuple(args, "O", &arg0))
	{
		return -1;
	}

	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);

	JPClass *cls;
	if (JPPyString::check(arg0))
	{
		string cname = JPPyString::asStringUTF8(arg0);
		cls = context->getTypeManager()->findClassByName(cname);
	} else
	{
		PyErr_SetString(PyExc_TypeError, "Classes require str object.");
		return (-1);
	}

	self->m_Value.m_Value = JPValue(context->_java_lang_Class,
			(jobject) cls->getJavaClass()).global(frame);
	self->m_Class = cls;
	return 0;
	JP_PY_CATCH(-1);
}

// FIXME remove this when finished.  It should be unnecessary

void PyJPClass_dealloc(PyObject *self)
{
	JP_PY_TRY("PyJPClass_dealloc", self);
	PyJPModuleState *state = PyJPModuleState_global;
	((PyTypeObject*) state->PyJPValue_Type)->tp_dealloc(self);
	JP_PY_CATCH();
}

/**
 * This is a special case as we can access the string from JPClass.
 */
PyObject *PyJPClass_str(PyJPClass *pyself)
{
	JP_PY_TRY("PyJPClass_toString", pyself);
	JPJavaFrame frame(PyJPModule_getContext());
	return JPPyString::fromStringUTF8(
			frame.toString((jobject) pyself->m_Class->getJavaClass())).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPClass_getCanonicalName(PyJPClass *self, void *closure)
{
	JP_PY_TRY("PyJPClass_getCanonicalName", self);
	PyJPModule_getContext();
	string name = self->m_Class->getCanonicalName();
	JP_TRACE(name);
	PyObject *res = JPPyString::fromStringUTF8(name).keep();
	return res;
	JP_PY_CATCH(NULL);
}

/**
 * Get the bases during Python wrapper class creation.
 *
 * This does the work of selecting which class methods are required for a
 * given base type.  Some classes like boxed and exceptions cannot inherit
 * directly from Java objects due to base class conflicts.
 *
 * @param self
 * @param closure
 * @return
 */
PyObject *PyJPClass_getBases(PyJPClass *self, void *closure)
{
	JP_PY_TRY("PyJPClass_getBases", self);
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);

	self->m_Class->ensureMembers(frame);

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
		super = NULL;
		baseType = JPPyObject(JPPyRef::_use, state->PyJPValueExc_Type);
	} else if (dynamic_cast<JPArrayClass*> (self->m_Class) == self->m_Class)
	{
		baseType = JPPyObject(JPPyRef::_claim, PyObject_GetAttrString(PyJPModule_global, "_JArrayBase"));
	} else if (self->m_Class->isPrimitive())
	{
		JP_RAISE(PyExc_TypeError, "primitives are not objects");
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

/**
 * Get the class fields.
 *
 * Used only by JClassFactory.
 *
 * @param self
 * @param closure
 * @return
 */
PyObject *PyJPClass_getClassFields(PyJPClass *self, void *closure)
{
	JP_PY_TRY("PyJPClass_getClassFields", self);
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);

	self->m_Class->ensureMembers(frame);

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

/**
 * Get class methods.
 *
 * Used only by JClassFactory.  This does the special magic to
 * include the Object methods for boxed and exception types.
 *
 * @param self
 * @param closure
 * @return
 */
PyObject *PyJPClass_getClassMethods(PyJPClass *self, void *closure)
{
	JP_PY_TRY("PyJPClass_getClassMethods", self);
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);

	self->m_Class->ensureMembers(frame);

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

/**
 * Create a instance with a different JClass than specified by the object.
 *
 * This is used to force a type into the method resolution.  Called by
 * JObject.
 *
 * @param self
 * @param args
 * @return
 */
PyObject *PyJPClass_cast(PyJPClass *self, PyObject *value)
{
	JP_PY_TRY("PyJPClass_cast", self);
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	JPClass *type = self->m_Class;
	ASSERT_NOT_NULL(value);
	ASSERT_NOT_NULL(type);

	// If it is already a Java object, then let Java decide
	// if the cast is possible
	JPValue *jval = JPPythonEnv::getJavaValue(value);
	if (jval != NULL && type->isInstance(frame, *jval))
	{
		JPPyObject wrapper(JPPyRef::_claim, PyJPModule_getClass(NULL, (PyObject*) self));
		return PyJPValue_create((PyTypeObject*) wrapper.get(), context,
				JPValue(type, jval->getValue())).keep();
	}

	// Otherwise, see if we can convert it
	{
		JPMatch match;
		type->getJavaConversion(&frame, match, value);
		if (match.type == JPMatch::_none || match.conversion == NULL)
		{
			stringstream ss;
			ss << "Unable to convert " << Py_TYPE(value)->tp_name << " to java type " << type->toString();
			PyErr_SetString(PyExc_TypeError, ss.str().c_str());
			return 0;
		}

		JPPyObject wrapper(JPPyRef::_claim, PyJPModule_getClass(NULL, (PyObject*) self));
		jvalue v = match.conversion->convert(&frame, type, value);
		return PyJPValue_create((PyTypeObject*) wrapper.get(), context, JPValue(type, v)).keep();
	}
	JP_PY_CATCH(NULL);
}

PyObject *PyJPClass_canConvertToJava(PyJPClass *self, PyObject *args)
{
	JP_PY_TRY("PyJPClass_canConvertToJava", self);
	JPContext *context = PyJPModule_getContext();
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
	JP_PY_TRY("PyJPClass_convertToJava", self);
	PyJPModuleState *state = PyJPModuleState_global;
	JPContext *context = PyJPModule_getContext();
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

PyObject *PyJPClass_dumpCtor(PyJPClass *self)
{
	JP_PY_TRY("PyJPClass_dumpCtor", self);
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	string report = self->m_Class->getCtor()->dump();
	return JPPyString::fromStringUTF8(report).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPClass_newArrayType(PyJPClass* self, PyObject* dims)
{
	JP_PY_TRY("PyJPClass_getArrayType", self);
	JPContext* context = PyJPModule_getContext();
	stringstream ss;
	if (!PyIndex_Check(dims))
		JP_RAISE(PyExc_TypeError, "dims must be an integer");
	Py_ssize_t d = PyNumber_AsSsize_t(dims, PyExc_IndexError);
	if (d > 255)
		JP_RAISE(PyExc_ValueError, "dims too large");

	for (int i = 0; i < d; ++i)
		ss << "[";
	if (self->m_Class->isPrimitive())
		ss << ((JPPrimitiveType*) self->m_Class)->getTypeCode();
	else if (dynamic_cast<JPArrayClass*> (self->m_Class) == self->m_Class)
		ss << self->m_Class->getName();
	else
		ss << "L" << self->m_Class->getName() << ";";
	JPClass* cls = context->getTypeManager()->findClassByName(ss.str());
	JPPyObject pycls(PyJPClass_create(
			(PyTypeObject*) PyJPModuleState_global->PyJPClass_Type, context, cls));
	return PyJPModule_getClass(NULL, pycls.get());
	JP_PY_CATCH(NULL);
}

bool PyJPClass_Check(PyObject* obj)
{
	PyTypeObject* type = Py_TYPE(obj);
	// PyJPClass -> PyJPValue -> PyJPValueBase -> object
	PyObject *mro = type->tp_mro;
	Py_ssize_t n = PyTuple_Size(mro);
	if (n < 4)
		return false;
	return PyTuple_GetItem(mro, n - 4) == PyJPModuleState_global->PyJPClass_Type;
}

static PyMethodDef classMethods[] = {
	{"_cast", (PyCFunction) (&PyJPClass_cast), METH_O, ""},
	{"_canConvertToJava", (PyCFunction) (&PyJPClass_canConvertToJava), METH_VARARGS, ""},
	{"_convertToJava", (PyCFunction) (&PyJPClass_convertToJava), METH_VARARGS, ""},
	{"_dumpCtor", (PyCFunction) (&PyJPClass_dumpCtor), METH_NOARGS, ""},
	{"_newArrayType", (PyCFunction) (&PyJPClass_newArrayType), METH_O, ""},
	{NULL},
};

static PyGetSetDef classGetSets[] = {
	{"__javaname__", (getter) (&PyJPClass_getCanonicalName), NULL, ""},
	{"_fields",      (getter) (&PyJPClass_getClassFields), NULL, ""},
	{"_methods",     (getter) (&PyJPClass_getClassMethods), NULL, ""},
	{"_bases",       (getter) (&PyJPClass_getBases), NULL, ""},
	{0}
};

static PyType_Slot classSlots[] = {
	{ Py_tp_new,     (void*) PyJPClass_new},
	{ Py_tp_init,    (void*) PyJPClass_init},
	{ Py_tp_dealloc, (void*) PyJPClass_dealloc},
	{ Py_tp_str,     (void*) PyJPClass_str},
	{ Py_tp_methods, (void*) classMethods},
	{ Py_tp_doc,     (void*) classDoc},
	{ Py_tp_getset,  (void*) &classGetSets},
	{0}
};

PyType_Spec PyJPClassSpec = {
	"_jpype.PyJPClass",
	sizeof (PyJPClass),
	0,
	Py_TPFLAGS_DEFAULT  | Py_TPFLAGS_BASETYPE,
	classSlots
};

#ifdef __cplusplus
}
#endif

/**
 * Internal method for wrapping a returned Java class instance.
 *
 * @param wrapper
 * @param context
 * @param cls
 * @return
 */
JPPyObject PyJPClass_create(PyTypeObject *wrapper, JPContext *context, JPClass *cls)
{
	JP_TRACE_IN("PyJPClass_create");
	// Special case for primitives
	if (context == NULL)
	{
		JPPyObject self = PyJPValue_createInstance(wrapper, context, JPValue());
		((PyJPClass*) self.get())->m_Class = cls;
		return self;
	}

	JPJavaFrame frame(context);
	JPPyObject self = PyJPValue_createInstance(wrapper, context,
			JPValue(context->_java_lang_Class, (jobject) cls->getJavaClass()));
	((PyJPClass*) self.get())->m_Class = cls;
	return self;
	JP_TRACE_OUT;
}
