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
#include "jpype.h"
#include "pyjp.h"
#include "jp_arrayclass.h"
#include "jp_boxedclasses.h"
#include "jp_stringclass.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Allocate a new Python object with a slot for Java.
 *
 * We need extra space to store our values, but because there
 * is no way to do so without disturbing the object layout.
 * Fortunately, Python already handles this for dict and weakref.
 * Python aligns the ends of the structure and increases the
 * base type size to add additional slots to a standard object.
 *
 * We will use the same trick to add an additional slot for Java
 * after the end of the object outside of where Python is looking.
 * As the memory is aligned this is safe to do.  We will use
 * the alloc and finalize slot to recognize which objects have this
 * extra slot appended.
 */
PyObject* PyJPValue_alloc(PyTypeObject* type, Py_ssize_t nitems )
{
	JP_PY_TRY("PyJPValue_alloc");
	// Modification from Python to add size elements
	const size_t size = _PyObject_VAR_SIZE(type, nitems + 1) + sizeof (JPValue);
	PyObject *obj = (PyType_IS_GC(type)) ? _PyObject_GC_Malloc(size)
			: (PyObject *) PyObject_MALLOC(size);
	if (obj == NULL)
		return PyErr_NoMemory();
	memset(obj, 0, size);

	Py_ssize_t refcnt = ((PyObject*) type)->ob_refcnt;
	if (type->tp_itemsize == 0)
		PyObject_Init(obj, type);
	else
		PyObject_InitVar((PyVarObject *) obj, type, nitems);

	// This line is required to deal with Python bug (GH-11661)
	// Some versions of Python fail to increment the reference counter of
	// heap types properly.
	if (refcnt == ((PyObject*) type)->ob_refcnt)
		Py_INCREF(type);

	if (PyType_IS_GC(type))
	{
		PyObject_GC_Track(obj);
	}
	JP_TRACE("alloc", type->tp_name, obj);
	return obj;
	JP_PY_CATCH(NULL);
}

bool PyJPValue_hasJavaSlot(PyTypeObject* type)
{
	if (type == NULL
			|| type->tp_alloc != (allocfunc) PyJPValue_alloc
			|| type->tp_finalize != (destructor) PyJPValue_finalize)
		return false;
	return true;
}

Py_ssize_t PyJPValue_getJavaSlotOffset(PyObject* self)
{
	PyTypeObject *type = Py_TYPE(self);
	if (type == NULL
			|| type->tp_alloc != (allocfunc) PyJPValue_alloc
			|| type->tp_finalize != (destructor) PyJPValue_finalize)
		return 0;
	Py_ssize_t offset;
	Py_ssize_t sz = Py_SIZE(self);
	// I have no clue what negative sizes mean
	if (sz < 0)
		sz = -sz;
	if (type->tp_itemsize == 0)
		offset = _PyObject_VAR_SIZE(type, 1);
	else
		offset = _PyObject_VAR_SIZE(type, sz + 1);
	return offset;
}

/**
 * Get the Java value if attached.
 *
 * @param obj
 * @return the Java value or 0 if not found.
 */
JPValue* PyJPValue_getJavaSlot(PyObject* self)
{
	Py_ssize_t offset = PyJPValue_getJavaSlotOffset(self);
	if (offset == 0)
		return NULL;
	JPValue* value = (JPValue*) (((char*) self) + offset);
	if (value->getClass() == NULL)
		return NULL;
	return value;
}

void PyJPValue_free(void* obj)
{
	JP_PY_TRY("PyJPValue_free", obj);
	// Normally finalize is not run on simple classes.
	PyTypeObject *type = Py_TYPE(obj);
	if (type->tp_finalize != NULL)
		type->tp_finalize((PyObject*) obj);
	if (type->tp_flags & Py_TPFLAGS_HAVE_GC)
		PyObject_GC_Del(obj);
	else
		PyObject_Free(obj);
	JP_PY_CATCH();
}

void PyJPValue_finalize(void* obj)
{
	JP_PY_TRY("PyJPValue_finalize", obj);
	JP_TRACE("type", Py_TYPE(obj)->tp_name);
	JPValue* value = PyJPValue_getJavaSlot((PyObject*) obj);
	if (value == NULL || value->getClass() == NULL)
		return;
	JPClass* cls = value->getClass();
	// This one can't check for initialized because we may need to delete a stale
	// resource after shutdown.
	if (cls != NULL && JPEnv::isInitialized() && !cls->isPrimitive())
	{
		JP_TRACE("Value", cls->getCanonicalName(), &(value->getValue()));
		JP_TRACE("Dereference object");
		JPJavaFrame::ReleaseGlobalRef(value->getValue().l);
		*value = JPValue();
	}
	JP_PY_CATCH();
}

static int PyJPValue_init(PyObject* self, PyObject* args, PyObject* kwargs)
{
	JP_PY_TRY("PyJPValue_init", self);
	ASSERT_JVM_RUNNING();
	JPJavaFrame frame;

	PyObject* claz;
	PyObject* pyvalue;

	if (!PyArg_ParseTuple(args, "OO", &claz, &pyvalue))
	{
		return -1;
	}

	JPClass* type = PyJPClass_getJPClass(claz);
	ASSERT_NOT_NULL(pyvalue);
	ASSERT_NOT_NULL(type);

	// If it is already a Java object, then let Java decide
	// if the cast is possible
	JPValue* value = PyJPValue_getJavaSlot(pyvalue);
	if (value != NULL && type->isInstance(*value))
	{
		PyJPValue_assignJavaSlot(self, *value);
		return 0;
	}

	// Otherwise, see if we can convert it
	if (type->canConvertToJava(pyvalue) == JPMatch::_none)
	{
		stringstream ss;
		ss << "Unable to convert " << Py_TYPE(pyvalue)->tp_name << " to java type " << type->toString();
		PyErr_SetString(PyExc_TypeError, ss.str().c_str());
		return -1;
	}

	jvalue v = type->convertToJava(pyvalue);
	PyJPValue_assignJavaSlot(self, JPValue(type, v));
	return 0;
	JP_PY_CATCH(-1);
}

/** This is the way to convert an object into a python string. */
PyObject* PyJPValue_str(PyObject* self)
{
	JP_PY_TRY("PyJPValue_toString", self);
	ASSERT_JVM_RUNNING();
	JPJavaFrame frame;
	JPValue* value = PyJPValue_getJavaSlot(self);
	if (value == NULL)
		JP_RAISE(PyExc_TypeError, "Not a Java value");

	JPClass* cls = value->getClass();

	if (cls == NULL)
		JP_RAISE(PyExc_TypeError, "Java class is null");
	if (cls->isPrimitive())
		JP_RAISE(PyExc_ValueError, "toString requires a java object");

	if (cls == JPTypeManager::_java_lang_String)
	{
		PyObject *cache;
		JPPyObject dict(JPPyRef::_accept, PyObject_GenericGetDict(self, NULL));
		if (!dict.isNull())
		{
			cache = PyDict_GetItemString(dict.get(), "_jstr");
			if (cache)
			{
				Py_INCREF(cache);
				return cache;
			}
			jstring jstr = (jstring) value->getValue().l;
			string str;
			if (jstr == NULL)
				str = "(null)";
			else
				str = JPJni::toStringUTF8(jstr);
			cache = JPPyString::fromStringUTF8(str).keep();
			PyDict_SetItemString(dict.get(), "_jstr", cache);
			Py_INCREF(cache);
			return cache;
		}
	}
	if (cls == NULL)
		JP_RAISE(PyExc_ValueError, "toString called with null class");

	// In general toString is not immutable, so we won't cache it.
	return JPPyString::fromStringUTF8(JPJni::toString(value->getValue().l)).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPValue_getattro(PyObject *obj, PyObject *name)
{
	JP_PY_TRY("PyJPObject_getattro");
	if (!PyUnicode_Check(name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				Py_TYPE(name)->tp_name);
		return NULL;
	}

	// Private members are accessed directly
	PyObject* pyattr = PyBaseObject_Type.tp_getattro(obj, name);
	if (pyattr == NULL)
		return NULL;
	JPPyObject attr(JPPyRef::_accept, pyattr);

	// Private members go regardless
	if (PyUnicode_GetLength(name) && PyUnicode_ReadChar(name, 0) == '_')
		return attr.keep();

	// Methods
	if (Py_TYPE(attr.get()) == (PyTypeObject*) PyJPMethod_Type)
		return attr.keep();

	// Don't allow properties to be rewritten
	if (!PyObject_IsInstance(attr.get(), (PyObject*) & PyProperty_Type))
		return attr.keep();

	const char *name_str = PyUnicode_AsUTF8(name);
	PyErr_Format(PyExc_AttributeError, "Field '%s' is static", name_str);
	return NULL;
	JP_PY_CATCH(NULL);
}

int PyJPValue_setattro(PyObject *self, PyObject *name, PyObject *value)
{
	JP_PY_TRY("PyJPObject_setattro");
	if (!PyUnicode_Check(name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				Py_TYPE(name)->tp_name);
		return -1;
	}

	// Private members are accessed directly
	if (PyUnicode_GetLength(name) && PyUnicode_ReadChar(name, 0) == '_')
		return PyObject_GenericSetAttr(self, name, value);
	JPPyObject f = JPPyObject(JPPyRef::_accept, Py_GetAttrDescriptor(Py_TYPE(self), name));
	if (f.isNull())
	{
		const char *name_str = PyUnicode_AsUTF8(name);
		PyErr_Format(PyExc_AttributeError, "Field '%s' is not found", name_str);
		return -1;
	}
	descrsetfunc desc = Py_TYPE(f.get())->tp_descr_set;
	if (desc != NULL)
		return desc(f.get(), self, value);

	// Not a descriptor
	const char *name_str = PyUnicode_AsUTF8(name);
	PyErr_Format(PyExc_AttributeError,
			"Field '%s' is not settable on Java '%s' object", name_str, Py_TYPE(self)->tp_name);
	return -1;
	JP_PY_CATCH(-1);
}

// This works for Primitives and Boxed types that derive from
// Python long or float,  char required special handling.

PyObject *PyJPChar_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPValueChar_new", type);
	ASSERT_JVM_RUNNING();
	JPJavaFrame frame;
	PyObject *self;
	if (PyTuple_Size(args) == 1 && JPPyString::checkCharUTF16(PyTuple_GetItem(args, 0)))
	{
		jchar i = JPPyString::asCharUTF16(PyTuple_GetItem(args, 0));
		PyObject *args2 = PyTuple_Pack(1, PyLong_FromLong(i));
		self = PyLong_Type.tp_new(type, args2, kwargs);
		Py_DECREF(args2);
	} else
	{
		self = PyLong_Type.tp_new(type, args, kwargs);
	}
	if (!self)
		JP_RAISE_PYTHON("type new failed");
	JPClass *cls = PyJPClass_getJPClass((PyObject*) type);
	if (cls == NULL)
		JP_RAISE(PyExc_TypeError, "Class type incorrect");
	jvalue val = cls->convertToJava(self);
	PyJPValue_assignJavaSlot(self, JPValue(cls, val));
	JP_TRACE("new", self);
	return self;
	JP_PY_CATCH(NULL);
}

PyObject* PyJPValueChar_str(PyObject* self)
{
	JP_PY_TRY("PyJPValueChar_str", self);
	JPValue *value = PyJPValue_getJavaSlot(self);
	if (value == NULL)
		JP_RAISE(PyExc_RuntimeError, "Java slot missing");
	return JPPyString::fromCharUTF16(value->getValue().c).keep();
	JP_PY_CATCH(NULL);
}

static PyType_Slot valueSlots[] = {
	{ Py_tp_init,     (void*) PyJPValue_init},
	{ Py_tp_free,     (void*) PyJPValue_free},
	{ Py_tp_getattro, (void*) &PyJPValue_getattro},
	{ Py_tp_setattro, (void*) &PyJPValue_setattro},
	{0}
};

PyTypeObject *PyJPValue_Type = NULL;
static PyType_Spec valueSpec = {
	"_jpype._JValue",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	valueSlots
};

static PyType_Slot valueLongSlots[] = {
	{ Py_tp_new,  (void*) PyJPNumber_new},
	{0}
};

PyTypeObject *PyJPValueLong_Type = NULL;
static PyType_Spec valueLongSpec = {
	"_jpype._JValueLong",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	valueLongSlots
};


static PyType_Slot valueFloatSlots[] = {
	{ Py_tp_new,   (void*) PyJPNumber_new},
	{0}
};

PyTypeObject *PyJPValueFloat_Type = NULL;
static PyType_Spec valueFloatSpec = {
	"_jpype._JValueFloat",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	valueFloatSlots
};


static PyType_Slot valueCharSlots[] = {
	{ Py_tp_new,  (void*) PyJPChar_new},
	{ Py_tp_str,  (void*) PyJPValueChar_str},
	{0}
};

PyTypeObject *PyJPValueChar_Type = NULL;
static PyType_Spec valueCharSpec = {
	"_jpype._JValueChar",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	valueCharSlots
};

#ifdef __cplusplus
}
#endif

void PyJPValue_initType(PyObject* module)
{
	PyObject *bases;
	PyJPValue_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&valueSpec, NULL);
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JValue", (PyObject*) PyJPValue_Type);
	JP_PY_CHECK();

	bases = PyTuple_Pack(1, &PyLong_Type);
	PyJPValueLong_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&valueLongSpec, bases);
	Py_DECREF(bases);
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JValueLong", (PyObject*) PyJPValueLong_Type);
	JP_PY_CHECK();

	bases = PyTuple_Pack(1, &PyFloat_Type);
	PyJPValueFloat_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&valueFloatSpec, bases);
	Py_DECREF(bases);
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JValueFloat", (PyObject*) PyJPValueFloat_Type);
	JP_PY_CHECK();

	bases = PyTuple_Pack(1, &PyLong_Type);
	PyJPValueChar_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&valueCharSpec, bases);
	Py_DECREF(bases);
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JValueChar", (PyObject*) PyJPValueChar_Type);
	JP_PY_CHECK();

}

static JPPyObject PyJPNumber_create(JPPyObject& wrapper, const JPValue& value)
{
	// Bools are not numbers in Java
	if (value.getClass() == JPTypeManager::_java_lang_Boolean)
	{
		jlong l = JPJni::booleanValue(value.getJavaObject());
		PyObject *args = PyTuple_Pack(1, PyLong_FromLongLong(l));
		return JPPyObject(JPPyRef::_call,
				PyLong_Type.tp_new((PyTypeObject*) wrapper.get(), args, NULL));
	}
	if (PyObject_IsSubclass(wrapper.get(), (PyObject*) & PyLong_Type))
	{
		jlong l = JPJni::longValue(value.getJavaObject());
		PyObject *args = PyTuple_Pack(1, PyLong_FromLongLong(l));
		return JPPyObject(JPPyRef::_call,
				PyLong_Type.tp_new((PyTypeObject*) wrapper.get(), args, NULL));
	}
	if (PyObject_IsSubclass(wrapper.get(), (PyObject*) & PyFloat_Type))
	{
		jdouble l = JPJni::doubleValue(value.getJavaObject());
		PyObject *args = PyTuple_Pack(1, PyFloat_FromDouble(l));
		return JPPyObject(JPPyRef::_call,
				PyFloat_Type.tp_new((PyTypeObject*) wrapper.get(), args, NULL));
	}
	JP_RAISE(PyExc_TypeError, "unable to convert");
}

// These are from the internal methods when we already have the jvalue

JPPyObject PyJPValue_create(const JPValue& value2)
{
	JP_TRACE_IN("PyJPValue_create");
	JPValue value = value2;
	JPClass* cls = value.getClass();
	if (cls == NULL)
	{
		cls = JPTypeManager::_java_lang_Object;
		value.getValue().l = NULL;
	}

	if (cls->isPrimitive())
	{
		// FIXME we could actually get these to cast to the correct
		// primitive type which would preserve type conversions here.
		return cls->convertToPythonObject(value.getValue());
	}

	JPPyObject obj;
	JPPyObject wrapper = PyJPClass_create(cls);
	if (dynamic_cast<JPArrayClass*> (cls) == cls)
	{
		obj = PyJPArray_create((PyTypeObject*) wrapper.get(), value);
	} else if (cls->isThrowable())
	{
		// Exceptions need new and init
		JPPyTuple tuple = JPPyTuple::newTuple(0);
		obj = JPPyObject(JPPyRef::_call, PyObject_Call(wrapper.get(), tuple.get(), NULL));
	} else if (dynamic_cast<JPBoxedClass*> (cls) != NULL)
	{
		obj = PyJPNumber_create(wrapper, value);
	} else
	{
		// Simple objects don't have a new or init function
		PyTypeObject *type = (PyTypeObject*) wrapper.get();
		PyObject *obj2 = type->tp_alloc(type, 0);
		JP_PY_CHECK();
		obj = JPPyObject(JPPyRef::_claim, obj2);
	}

	// Fill in the Java slot
	PyJPValue_assignJavaSlot(obj.get(), value);
	return obj;
	JP_TRACE_OUT;
}

void PyJPValue_assignJavaSlot(PyObject* self, const JPValue& value)
{
	JPJavaFrame frame;
	Py_ssize_t offset = PyJPValue_getJavaSlotOffset(self);
	if (offset == 0)
	{
		std::stringstream ss;
		ss << "Missing Java slot on `" << Py_TYPE(self)->tp_name << "`";
		JP_RAISE(PyExc_TypeError, ss.str().c_str());
	}

	JPValue* slot = (JPValue*) (((char*) self) + offset);
	JPClass* cls = value.getClass();
	if (cls != NULL && !cls->isPrimitive())
	{
		jvalue q;
		q.l = frame.NewGlobalRef(value.getValue().l);
		*slot = JPValue(cls, q);
	} else
		*slot = value;
}

bool PyJPValue_isSetJavaSlot(PyObject* self)
{
	Py_ssize_t offset = PyJPValue_getJavaSlotOffset(self);
	if (offset == 0)
		return false;
	JPValue* slot = (JPValue*) (((char*) self) + offset);
	return slot->getClass() != NULL;
}
