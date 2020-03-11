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
#include "jp_boxedtype.h"
#include "jp_stringtype.h"

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
	JPContext *context = PyJPModule_getContext();
	if (cls != NULL && context->isRunning() && !cls->isPrimitive())
	{
		JP_TRACE("Value", cls->getCanonicalName(), &(value->getValue()));
		JP_TRACE("Dereference object");
		context->ReleaseGlobalRef(value->getValue().l);
		*value = JPValue();
	}
	JP_PY_CATCH_NONE();
}

/** This is the way to convert an object into a python string. */
PyObject* PyJPValue_str(PyObject* self)
{
	JP_PY_TRY("PyJPValue_str", self);
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	JPValue* value = PyJPValue_getJavaSlot(self);
	if (value == NULL)
		JP_RAISE(PyExc_TypeError, "Not a Java value");

	JPClass* cls = value->getClass();

	if (cls == NULL)
		JP_RAISE(PyExc_TypeError, "Java class is null");
	if (cls->isPrimitive())
		JP_RAISE(PyExc_ValueError, "toString requires a java object");

	if (cls == context->_java_lang_String)
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
				str = frame.toStringUTF8(jstr);
			cache = JPPyString::fromStringUTF8(str).keep();
			PyDict_SetItemString(dict.get(), "_jstr", cache);
			Py_INCREF(cache);
			return cache;
		}
	}
	if (cls == NULL)
		JP_RAISE(PyExc_ValueError, "toString called with null class");

	// In general toString is not immutable, so we won't cache it.
	return JPPyString::fromStringUTF8(frame.toString(value->getValue().l)).keep();
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

#ifdef __cplusplus
}
#endif

// These are from the internal methods when we already have the jvalue

JPPyObject PyJPValue_create(JPJavaFrame &frame, const JPValue& value2)
{
	JP_TRACE_IN("PyJPValue_create");
	JPValue value = value2;
	JPClass* cls = value.getClass();
	JPContext *context = frame.getContext();
	if (cls == NULL)
	{
		cls = context->_java_lang_Object;
		value.getValue().l = NULL;
	}

	if (cls->isPrimitive())
	{
		// We could actually get these to cast to the correct
		// primitive type which would preserve type conversions here.
		return cls->convertToPythonObject(frame, value.getValue());
	}

	JPPyObject obj;
	JPPyObject wrapper = PyJPClass_create(frame, cls);
	if (dynamic_cast<JPArrayClass*> (cls) == cls)
	{
		obj = PyJPArray_create(frame, (PyTypeObject*) wrapper.get(), value);
	} else if (cls->isThrowable())
	{
		// Exceptions need new and init
		JPPyTuple tuple = JPPyTuple::newTuple(1);
		tuple.setItem(0, _JObjectKey);
		obj = JPPyObject(JPPyRef::_call, PyObject_Call(wrapper.get(), tuple.get(), NULL));
	} else if (dynamic_cast<JPBoxedType*> (cls) != NULL)
	{
		obj = PyJPNumber_create(frame, wrapper, value);
	} else
	{
		// Simple objects don't have a new or init function
		PyTypeObject *type = (PyTypeObject*) wrapper.get();
		PyObject *obj2 = type->tp_alloc(type, 0);
		JP_PY_CHECK();
		obj = JPPyObject(JPPyRef::_claim, obj2);
	}

	// Fill in the Java slot
	PyJPValue_assignJavaSlot(frame, obj.get(), value);
	return obj;
	JP_TRACE_OUT;
}

void PyJPValue_assignJavaSlot(JPJavaFrame &frame, PyObject* self, const JPValue& value)
{
	Py_ssize_t offset = PyJPValue_getJavaSlotOffset(self);
	if (offset == 0)
	{
		std::stringstream ss;
		ss << "Missing Java slot on `" << Py_TYPE(self)->tp_name << "`";
		JP_RAISE(PyExc_SystemError, ss.str().c_str());
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
