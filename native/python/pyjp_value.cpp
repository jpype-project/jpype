/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#include "jpype.h"
#include "pyjp.h"
#include "jp_stringtype.h"
#include <Python.h>
#include <mutex>

#ifdef __cplusplus
extern "C"
{
#endif

std::mutex mtx;

// Create a dummy type which we will use only for allocation
PyTypeObject* PyJPAlloc_Type = nullptr;

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
PyObject* PyJPValue_alloc(PyTypeObject* type, Py_ssize_t nitems)
{
	JP_PY_TRY("PyJPValue_alloc");

#if PY_VERSION_HEX >= 0x030d0000
	// This flag will try to place the dictionary are part of the object which 
	// adds an unknown number of bytes to the end of the object making it impossible
	// to attach our needed data.  If we kill the flag then we get usable behavior.
	if (PyType_HasFeature(type, Py_TPFLAGS_INLINE_VALUES)) {
		PyErr_Format(PyExc_RuntimeError, "Unhandled object layout");
		return 0;
	}
#endif

	PyObject* obj = nullptr;
	{  
		std::lock_guard<std::mutex> lock(mtx);
		// Mutate the allocator type 
		PyJPAlloc_Type->tp_flags = type->tp_flags;
		PyJPAlloc_Type->tp_basicsize = type->tp_basicsize + sizeof (JPValue);
		PyJPAlloc_Type->tp_itemsize = type->tp_itemsize;
	
		// Create a new allocation for the dummy type
		obj = PyType_GenericAlloc(PyJPAlloc_Type, nitems);
	}

	// Watch for memory errors
	if (obj == nullptr)
		return nullptr;

	// Polymorph the type to match our desired type
	obj->ob_type = type;
	Py_INCREF(type);

	JP_TRACE("alloc", type->tp_name, obj);
	return obj;
	JP_PY_CATCH(nullptr);
}

bool PyJPValue_hasJavaSlot(PyTypeObject* type)
{
	if (type == nullptr
			|| type->tp_alloc != (allocfunc) PyJPValue_alloc
			|| type->tp_finalize != (destructor) PyJPValue_finalize)
		return false;  // GCOVR_EXCL_LINE
	return true;
}

Py_ssize_t PyJPValue_getJavaSlotOffset(PyObject* self)
{
	PyTypeObject *type = Py_TYPE(self);
	if (type == nullptr
			|| type->tp_alloc != (allocfunc) PyJPValue_alloc
			|| type->tp_finalize != (destructor) PyJPValue_finalize)
	{
		return 0;
	}

	Py_ssize_t offset = 0;
	Py_ssize_t sz = 0;
	
#if PY_VERSION_HEX>=0x030c0000
	// starting in 3.12 there is no longer ob_size in PyLong
	if (PyType_HasFeature(self->ob_type, Py_TPFLAGS_LONG_SUBCLASS))
		sz = (((PyLongObject*)self)->long_value.lv_tag) >> 3;  // Private NON_SIZE_BITS
	else 
#endif
	if (type->tp_itemsize != 0)
		sz = Py_SIZE(self);
	// PyLong abuses ob_size with negative values prior to 3.12
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
 * The Java class is guaranteed not to be nullptr on success.
 *
 * @param obj
 * @return the Java value or 0 if not found.
 */
JPValue* PyJPValue_getJavaSlot(PyObject* self)
{
	Py_ssize_t offset = PyJPValue_getJavaSlotOffset(self);
	if (offset == 0)
		return nullptr;
	auto value = (JPValue*) (((char*) self) + offset);
	if (value->getClass() == nullptr)
		return nullptr;
	return value;
}

void PyJPValue_free(void* obj)
{
	JP_PY_TRY("PyJPValue_free", obj);
	// Normally finalize is not run on simple classes.
	PyTypeObject *type = Py_TYPE(obj);
	if (type->tp_finalize != nullptr)
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
	if (value == nullptr)
		return;

	// We can skip if the JVM is stopped.  No need for an exception here.
	JPContext *context = JPContext_global;
	if (context == nullptr || !context->isRunning())
		return;

	// JVM is running so set up the frame.
	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass* cls = value->getClass();
	// This one can't check for initialized because we may need to delete a stale
	// resource after shutdown.
	if (cls != nullptr && context->isRunning() && !cls->isPrimitive())
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
	JPJavaFrame frame = JPJavaFrame::outer();
	JPValue* value = PyJPValue_getJavaSlot(self);
	if (value == nullptr)
	{
		PyErr_SetString(PyExc_TypeError, "Not a Java value");
		return nullptr;
	}

	JPClass* cls = value->getClass();
	if (cls->isPrimitive())
	{
		PyErr_SetString(PyExc_TypeError, "toString requires a Java object");
		return nullptr;
	}

	if (value->getValue().l == nullptr)
		return JPPyString::fromStringUTF8("null").keep();

	if (cls == context->_java_lang_String)
	{
		PyObject *cache;
		JPPyObject dict = JPPyObject::accept(PyObject_GenericGetDict(self, nullptr));
		if (!dict.isNull())
		{
			cache = PyDict_GetItemString(dict.get(), "_jstr");
			if (cache)
			{
				Py_INCREF(cache);
				return cache;
			}
			auto jstr = (jstring) value->getValue().l;
			string str;
			str = frame.toStringUTF8(jstr);
			cache = JPPyString::fromStringUTF8(str).keep();
			PyDict_SetItemString(dict.get(), "_jstr", cache);
			return cache;
		}
	}

	// In general toString is not immutable, so we won't cache it.
	return JPPyString::fromStringUTF8(frame.toString(value->getValue().l)).keep();
	JP_PY_CATCH(nullptr);
}

PyObject *PyJPValue_getattro(PyObject *obj, PyObject *name)
{
	JP_PY_TRY("PyJPObject_getattro");
	if (!PyUnicode_Check(name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				Py_TYPE(name)->tp_name);
		return nullptr;
	}

	// Private members are accessed directly
	PyObject* pyattr = PyBaseObject_Type.tp_getattro(obj, name);
	if (pyattr == nullptr)
		return nullptr;
	JPPyObject attr = JPPyObject::accept(pyattr);

	// Private members go regardless
	if (PyUnicode_GetLength(name) && PyUnicode_ReadChar(name, 0) == '_')
		return attr.keep();

	// Methods
	if (Py_TYPE(attr.get()) == (PyTypeObject*) PyJPMethod_Type)
		return attr.keep();

	// Don't allow properties to be rewritten
	if (!PyObject_IsInstance(attr.get(), (PyObject*) & PyProperty_Type))
		return attr.keep();

	PyErr_Format(PyExc_AttributeError, "Field '%U' is static", name);
	return nullptr;
	JP_PY_CATCH(nullptr);
}

int PyJPValue_setattro(PyObject *self, PyObject *name, PyObject *value)
{
	JP_PY_TRY("PyJPObject_setattro");

	// Private members are accessed directly
	if (PyUnicode_GetLength(name) && PyUnicode_ReadChar(name, 0) == '_')
		return PyObject_GenericSetAttr(self, name, value);
	JPPyObject f = JPPyObject::accept(PyJP_GetAttrDescriptor(Py_TYPE(self), name));
	if (f.isNull())
	{
		PyErr_Format(PyExc_AttributeError, "Field '%U' is not found", name);
		return -1;
	}
	descrsetfunc desc = Py_TYPE(f.get())->tp_descr_set;
	if (desc != nullptr)
		return desc(f.get(), self, value);

	// Not a descriptor
	PyErr_Format(PyExc_AttributeError,
			"Field '%U' is not settable on Java '%s' object", name, Py_TYPE(self)->tp_name);
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
		JP_RAISE(PyExc_SystemError, ss.str());
	}
	// GCOVR_EXCL_STOP

	auto* slot = (JPValue*) (((char*) self) + offset);
	// GCOVR_EXCL_START
	// This is a sanity check that should never trigger in normal operations.
	if (slot->getClass() != nullptr)
	{
		JP_RAISE(PyExc_SystemError, "Slot assigned twice");
	}
	// GCOVR_EXCL_STOP
	JPClass* cls = value.getClass();
	if (cls != nullptr && !cls->isPrimitive())
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
		return false;  // GCOVR_EXCL_LINE
	auto* slot = (JPValue*) (((char*) self) + offset);
	return slot->getClass() != nullptr;
}

/***************** Create a dummy type for use when allocating. ************************/
static int PyJPAlloc_traverse(PyObject *self, visitproc visit, void *arg)
{
	return 0;
}

static int PyJPAlloc_clear(PyObject *self)
{
	return 0;
}


static PyType_Slot allocSlots[] = {
	{ Py_tp_traverse, (void*) PyJPAlloc_traverse},
	{ Py_tp_clear, (void*) PyJPAlloc_clear},
	{0, NULL}  // Sentinel
};

static PyType_Spec allocSpec = {
	"_jpype._JAlloc",
	sizeof(PyObject),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	allocSlots
};

void PyJPValue_initType(PyObject* module)
{
	PyObject *bases = PyTuple_Pack(1, &PyBaseObject_Type);
	PyJPAlloc_Type = (PyTypeObject*) PyType_FromSpecWithBases(&allocSpec, bases);
	Py_DECREF(bases);
	Py_INCREF(PyJPAlloc_Type);
	JP_PY_CHECK();
}
