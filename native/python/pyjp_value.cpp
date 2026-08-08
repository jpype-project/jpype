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

bool PyJPValue_hasJavaSlot(PyTypeObject* type)
{
	if (type == nullptr || type->tp_finalize != (destructor) PyJPValue_finalize)
		return false;  // GCOVR_EXCL_LINE
	// offset is a hard invariant on every fully-created type, abstract or
	// concrete alike (see PyJPClass_getOffset) -- no companion chase needed.
	return PyJPClass_getOffset(type) > 0;
}

Py_ssize_t PyJPValue_getJavaSlotOffset(PyObject* self)
{
	PyTypeObject *type = Py_TYPE(self);
	if (type == nullptr)
		return 0;

	// Families ported to the fixed-offset object model have their slot's
	// location baked in at type-creation time on the metaclass, and it is
	// always the real, resolved value -- including on an abstract type (a
	// live instance's Py_TYPE may legitimately be abstract, since
	// construction polymorphs back to the canonical type -- see
	// PyJPObject_new), so no companion chase is needed here either.
	return PyJPClass_getOffset(type);
}

/**
 * Get the JPClass for a Java-backed object, if any.
 *
 * With one exception this never touches per-instance storage: a wrapper
 * instance's class is a property of its type (set once, when the wrapper
 * type itself was created -- see PyJPClass_GetClass), not something that can
 * vary instance to instance or go "unset" independently of the type.
 *
 * The exception is a _JClass type object itself (e.g. the type object for
 * java.lang.String): such an object is, self-referentially, ALSO a Java
 * value -- of type java.lang.Class -- and that one case still carries its
 * own full inline JPValue (see PyJPClass_getOffset's PyJPClass_Type special
 * case), untouched by the per-instance-storage migration.
 *
 * @return the class, or nullptr if self's type carries no Java slot at all
 * (an ordinary Python object).
 */
JPClass* PyJPValue_getJPClass(PyObject* self)
{
	PyTypeObject *type = Py_TYPE(self);
	Py_ssize_t offset = PyJPClass_getOffset(type);
	if (offset == 0)
		return nullptr;
	if (type == (PyTypeObject*) PyJPClass_Type)
	{
		auto* value = (JPValue*) (((char*) self) + offset);
		return value->getClass();
	}
	return PyJPClass_GetClass(type);
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
	auto* self = (PyObject*) obj;
	PyTypeObject *type = Py_TYPE(self);
	Py_ssize_t offset = PyJPClass_getOffset(type);
	if (offset == 0)
		return;

	// We can skip if the JVM is stopped.  No need for an exception here.
	JPContext *context = JPContext_global;
	if (context == nullptr || !context->isRunning())
		return;

	if (type == (PyTypeObject*) PyJPClass_Type)
	{
		// self is itself a _JClass type object -- still a full inline
		// JPValue at this offset, untouched by the per-instance-storage
		// migration (see PyJPValue_getJPClass).
		auto* value = (JPValue*) (((char*) self) + offset);
		JPClass* cls = value->getClass();
		// This one can't check for initialized because we may need to
		// delete a stale resource after shutdown.
		if (cls != nullptr && context->isRunning() && !cls->isPrimitive())
		{
			JP_TRACE("Value", cls->getCanonicalName(), &(value->getValue()));
			JP_TRACE("Dereference object");
			context->ReleaseGlobalRef(value->getValue().l);
			*value = JPValue();
		}
		return;
	}

	// Families reconstructing their jvalue on demand (see PyJPValueFn) don't
	// own a persistent global ref here at all -- nothing to release.
	if (PyJPClass_GetJValueFn(type) != nullptr)
		return;

	JPClass* cls = PyJPClass_GetClass(type);
	if (cls != nullptr && context->isRunning() && !cls->isPrimitive())
	{
		auto* slot = (jvalue*) (((char*) self) + offset);
		if (slot->l != nullptr)
		{
			JP_TRACE("Value", cls->getCanonicalName(), slot);
			JP_TRACE("Dereference object");
			context->ReleaseGlobalRef(slot->l);
			slot->l = nullptr;
		}
	}
	JP_PY_CATCH_NONE();
}

/** This is the way to convert an object into a python string. */
PyObject* PyJPValue_str(PyObject* self)
{
	JP_PY_TRY("PyJPValue_str", self);
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass* cls = PyJPValue_getJPClass(self);
	if (cls == nullptr)
	{
		PyErr_SetString(PyExc_TypeError, "Not a Java value");
		return nullptr;
	}
	if (cls->isPrimitive())
	{
		PyErr_SetString(PyExc_TypeError, "toString requires a Java object");
		return nullptr;
	}

	jvalue value = PyJPValue_getJValue(frame, self);
	if (value.l == nullptr)
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
			auto jstr = (jstring) value.l;
			string str;
			str = frame.toStringUTF8(jstr);
			cache = JPPyString::fromStringUTF8(str).keep();
			PyDict_SetItemString(dict.get(), "_jstr", cache);
			return cache;
		}
	}

	// In general toString is not immutable, so we won't cache it.
	return JPPyString::fromStringUTF8(frame.toString(value.l)).keep();
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

	if (Py_TYPE(self) == (PyTypeObject*) PyJPClass_Type)
	{
		// self is itself a _JClass type object -- still a full inline
		// JPValue at this offset (see PyJPValue_getJPClass).
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
		return;
	}

	// Families reconstructing their jvalue on demand (see PyJPValueFn) own no
	// per-instance storage at all -- nothing to write. The value they'd be
	// asked to store here is always recoverable later from the instance's
	// own native representation (or, for a boxed null, from the nullBoxed
	// singleton), so this call is a no-op for them.
	if (PyJPClass_GetJValueFn(Py_TYPE(self)) != nullptr)
		return;

	// Every other family: class is implicit from the wrapper type (already
	// fixed when the type itself was created -- see PyJPClass_GetClass), so
	// only the jvalue itself is stored here. No "assigned twice" guard: a
	// bare jvalue's zero-initialized state (from ordinary allocation) is
	// indistinguishable from a legitimately-null Java reference (see
	// JPClass::convertToPythonObject's cast-to-null path), and this is
	// always called exactly once per instance from tp_new before the
	// instance is exposed to Python -- a call-lifetime invariant, not
	// something worth a runtime check here.
	auto* slot = (jvalue*) (((char*) self) + offset);
	JPClass* cls = value.getClass();
	if (cls != nullptr && !cls->isPrimitive())
		slot->l = frame.NewGlobalRef(value.getValue().l);
	else
		*slot = value.getValue();
}

bool PyJPValue_isSetJavaSlot(PyObject* self)
{
	Py_ssize_t offset = PyJPValue_getJavaSlotOffset(self);
	if (offset == 0)
		return false;  // GCOVR_EXCL_LINE
	// Only ever called on _JClass type-object instances (PyJPClass_setClass)
	// -- see PyJPValue_getJPClass for why that's the one family still
	// carrying a full inline JPValue.
	auto* slot = (JPValue*) (((char*) self) + offset);
	return slot->getClass() != nullptr;
}

jvalue PyJPValue_getJValue(JPJavaFrame &frame, PyObject* self)
{
	PyTypeObject *type = Py_TYPE(self);
	Py_ssize_t offset = PyJPClass_getOffset(type);
	if (offset == 0)
		return jvalue{};
	if (type == (PyTypeObject*) PyJPClass_Type)
	{
		auto* value = (JPValue*) (((char*) self) + offset);
		return value->getValue();
	}
	PyJPValueFn fn = PyJPClass_GetJValueFn(type);
	if (fn != nullptr)
		return fn(frame, self);
	return *(jvalue*) (((char*) self) + offset);
}

