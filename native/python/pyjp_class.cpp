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

#ifdef __cplusplus
extern "C"
{
#endif

static PyObject *PyJPClass_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClass_new", type);
	PyObject *self = type->tp_alloc(type, 0);
	((PyJPClass*) self)->m_Class = NULL;
	return self;
	JP_PY_CATCH(NULL);
}

static int PyJPClass_init(PyJPClass *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClass_init", self);
	ASSERT_JVM_RUNNING("PyJPClass::__init__");
	JPJavaFrame frame;
	JPPyTuple tuple(JPPyRef::_use, args);
	if (tuple.size() != 1)
		JP_RAISE_TYPE_ERROR("Classes must have one argument.");

	JPClass* claz = NULL;
	PyObject* arg0 = tuple.getItem(0);
	JPValue* jpvalue = JPPythonEnv::getJavaValue(arg0);
	if (jpvalue != NULL && jpvalue->getClass() == JPTypeManager::_java_lang_Class)
	{
		claz = JPTypeManager::findClass((jclass) jpvalue->getJavaObject());
	} else if (JPPyString::check(arg0))
	{
		string cname = JPPyString::asStringUTF8(arg0);
		claz = JPTypeManager::findClass(cname);
	} else
	{
		JP_RAISE_TYPE_ERROR("Classes require str or java.lang.Class object.");
	}

	if (claz == NULL)
	{
		return (-1);
	}
	self->m_Class = claz;
	return 0;
	JP_PY_CATCH(-1);
}

static void PyJPClass_dealloc(PyObject *self)
{
	JP_PY_TRY("PyJPClass_dealloc", self);
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH();
}

static PyObject *PyJPClass_getCanonicalName(PyJPClass *self, PyObject *closure)
{
	JP_PY_TRY("PyJPClass_getCanonicalName", self);
	ASSERT_JVM_RUNNING("PyJPClass_getCanonicalName");
	JPJavaFrame frame;
	string name = self->m_Class->getCanonicalName();
	JP_TRACE(name);
	PyObject *res = JPPyString::fromStringUTF8(name).keep();
	return res;
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPClass_getSuperClass(PyJPClass *self, PyObject *arg)
{
	JP_PY_TRY("PyJPClass_getSuperClass", self);
	ASSERT_JVM_RUNNING("PyJPClass_getSuperClass");
	JPJavaFrame frame;

	JPClass* base = self->m_Class->getSuperClass();
	if (base == NULL)
		Py_RETURN_NONE;

	return PyJPClass_alloc(base).keep();
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPClass_getInterfaces(PyJPClass *self, PyObject *arg)
{
	JP_PY_TRY("PyJPClass_getInterfaces", self);
	ASSERT_JVM_RUNNING("PyJPClass_getInterfaces");
	JPJavaFrame frame;

	const JPClass::ClassList& baseItf = self->m_Class->getInterfaces();

	// Pack into a tuple
	JPPyTuple result(JPPyTuple::newTuple(baseItf.size()));
	for (unsigned int i = 0; i < baseItf.size(); i++)
	{
		result.setItem(i, PyJPClass_alloc(baseItf[i]).get());
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
static PyObject *PyJPClass_getClassFields(PyJPClass *self, PyObject *closure)
{
	JP_PY_TRY("PyJPClass_getClassFields", self);
	ASSERT_JVM_RUNNING("PyJPClass_getClassFields");
	JPJavaFrame frame;

	int i = 0;
	const JPClass::FieldList& instFields = self->m_Class->getFields();
	JPPyTuple result(JPPyTuple::newTuple(instFields.size()));
	for (JPClass::FieldList::const_iterator iter = instFields.begin(); iter != instFields.end(); iter++)
	{
		result.setItem(i++, PyJPField::alloc(*iter).get());
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
static PyObject *PyJPClass_getClassMethods(PyJPClass *self, PyObject *closure)
{
	JP_PY_TRY("PyJPClass_getClassMethods", self);
	ASSERT_JVM_RUNNING("PyJPClass_getClassMethods");
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
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPClass_newInstance(PyJPClass *self, PyObject *pyargs)
{
	JP_PY_TRY("PyJPClass_newInstance", self);
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
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPClass_isInterface(PyJPClass* self, PyObject* arg)
{
	JP_PY_TRY("PyJPClass_isInterface", self);
	ASSERT_JVM_RUNNING("PyJPClass_isInterface");
	JPJavaFrame frame;
	return PyBool_FromLong(self->m_Class->isInterface());
	JP_PY_CATCH(NULL);
}

static PyObject* PyJPClass_isPrimitive(PyJPClass* self, PyObject* args)
{
	JP_PY_TRY("PyJPClass_isPrimitive", self);
	ASSERT_JVM_RUNNING("PyJPClass_isPrimitive");
	JPJavaFrame frame;
	return PyBool_FromLong((self->m_Class)->isPrimitive());
	JP_PY_CATCH(NULL);
}

static PyObject* PyJPClass_isArray(PyJPClass* self, PyObject* args)
{
	JP_PY_TRY("PyJPClass_isPrimitive", self);
	ASSERT_JVM_RUNNING("PyJPClass::isArray");
	JPJavaFrame frame;
	return PyBool_FromLong(dynamic_cast<JPArrayClass*> (self->m_Class) == self->m_Class);
	JP_PY_CATCH(NULL);
}

// Added for auditing

static PyObject *PyJPClass_canConvertToJava(PyJPClass *self, PyObject *other)
{
	JP_PY_TRY("PyJPClass_convertToJava", self);

	ASSERT_JVM_RUNNING("PyJPClass::asJavaValue");
	JPJavaFrame frame;

	JPClass *cls = self->m_Class;

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
	JP_PY_CATCH(NULL);
}

// Added for auditing

static PyObject *PyJPClass_convertToJava(PyJPClass *self, PyObject *other)
{
	JP_PY_TRY("PyJPClass_convertToJava", self);
	ASSERT_JVM_RUNNING("PyJPClass::asJavaValue");
	JPJavaFrame frame;

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
	JP_PY_CATCH(NULL);
}

static PyMethodDef classMethods[] = {
	{"getCanonicalName", (PyCFunction) (&PyJPClass_getCanonicalName), METH_NOARGS, ""},
	{"getSuperClass", (PyCFunction) (&PyJPClass_getSuperClass), METH_NOARGS, ""},
	{"getClassFields", (PyCFunction) (&PyJPClass_getClassFields), METH_NOARGS, ""},
	{"getClassMethods", (PyCFunction) (&PyJPClass_getClassMethods), METH_NOARGS, ""},
	{"newInstance", (PyCFunction) (&PyJPClass_newInstance), METH_VARARGS, ""},
	{"getSuperclass", (PyCFunction) (&PyJPClass_getSuperClass), METH_NOARGS, ""},
	{"getInterfaces", (PyCFunction) (&PyJPClass_getInterfaces), METH_NOARGS, ""},
	{"isInterface", (PyCFunction) (&PyJPClass_isInterface), METH_NOARGS, ""},
	{"isPrimitive", (PyCFunction) (&PyJPClass_isPrimitive), METH_NOARGS, ""},
	{"isArray", (PyCFunction) (&PyJPClass_isArray), METH_NOARGS, ""},
	{"canConvertToJava", (PyCFunction) (&PyJPClass_canConvertToJava), METH_O, ""},
	{"convertToJava", (PyCFunction) (&PyJPClass_convertToJava), METH_O, ""},
	{NULL},
};

PyTypeObject* PyJPClass_Type = 0;
PyTypeObject _PyJPClass_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	/* tp_name           */ "_jpype.PyJPClass",
	/* tp_basicsize      */ sizeof (PyJPClass),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPClass_dealloc,
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
	/* tp_init           */ (initproc) PyJPClass_init,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPClass_new

};

#ifdef __cplusplus
}
#endif

/**
 * Internal method for wrapping a returned Java class instance.
 *
 * @param cls
 * @return
 */
JPPyObject PyJPClass_alloc(JPClass* cls)
{
	JP_TRACE_IN("PyJPClass_create");
	PyJPClass* res = (PyJPClass*) PyJPClass_Type->tp_alloc(PyJPClass_Type, 0);
	JP_PY_CHECK();
	res->m_Class = cls;
	return JPPyObject(JPPyRef::_claim, (PyObject*) res);
	JP_TRACE_OUT;
}

void PyJPClass_initType(PyObject* module)
{
	PyJPClass_Type = &_PyJPClass_Type;
	PyType_Ready(PyJPClass_Type);
	Py_INCREF(PyJPClass_Type);
	PyModule_AddObject(module, "PyJPClass", (PyObject*) (PyJPClass_Type));
}
