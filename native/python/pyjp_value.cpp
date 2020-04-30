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
		return PyErr_NoMemory(); // GCOVR_EXCL_LINE
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
		Py_INCREF(type);  // GCOVR_EXCL_LINE

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
 * The Java class is guaranteed not to be null on success.
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
		PyObject_Free(obj);  // GCOVR_EXCL_LINE
	JP_PY_CATCH_NONE();
}

void PyJPValue_finalize(void* obj)
{
	JP_PY_TRY("PyJPValue_finalize", obj);
	JP_TRACE("type", Py_TYPE(obj)->tp_name);
	JPValue* value = PyJPValue_getJavaSlot((PyObject*) obj);
	if (value == NULL)
		return;
	JPContext *context = JPContext_global;
	if (context == NULL || !context->isRunning())
		return;
	JPJavaFrame frame(context);
	JPClass* cls = value->getClass();
	// This one can't check for initialized because we may need to delete a stale
	// resource after shutdown.
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
	if (cls->isPrimitive())
		JP_RAISE(PyExc_TypeError, "toString requires a Java object");

	if (value->getValue().l == NULL)
		return JPPyString::fromStringUTF8("null").keep();

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
			str = frame.toStringUTF8(jstr);
			cache = JPPyString::fromStringUTF8(str).keep();
			PyDict_SetItemString(dict.get(), "_jstr", cache);
			Py_INCREF(cache);
			return cache;
		}
	}

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

void PyJPValue_assignJavaSlot(JPJavaFrame &frame, PyObject* self, const JPValue& value)
{
	Py_ssize_t offset = PyJPValue_getJavaSlotOffset(self);
	// GCOVR_EXCL_START
	if (offset == 0)
	{
		std::stringstream ss;
		ss << "Missing Java slot on `" << Py_TYPE(self)->tp_name << "`";
		JP_RAISE(PyExc_SystemError, ss.str().c_str());
	}
	// GCOVR_EXCL_STOP

	JPValue* slot = (JPValue*) (((char*) self) + offset);
	// GCOVR_EXCL_START
	// This is a sanity check that should never trigger in normal operations.
	if (slot->getClass() != NULL)
	{
		JP_RAISE(PyExc_SystemError, "Slot assigned twice");
	}
	// GCOVR_EXCL_STOP
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
