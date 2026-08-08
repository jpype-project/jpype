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
#include <cstddef>
#include "jpype.h"
#include "pyjp.h"

#ifdef __cplusplus
extern "C"
{
#endif

static PyObject *PyJPObject_new(PyTypeObject *type, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPObject_new");
	// Get the Java class from the type.  This must use the canonical/visible
	// type -- the one the caller actually named -- since only it has
	// m_Class populated; a hidden concrete companion (see
	// PyJPClass_concrete) never does.
	JPClass *cls = PyJPClass_getJPClass((PyObject*) type);
	if (cls == nullptr)
	{
		PyErr_SetString(PyExc_TypeError, "Java class type is incorrect");
		return nullptr;
	}

	// Create an instance (this may fail)
	JPJavaFrame frame = JPJavaFrame::outer();
	JPPyObjectVector args(pyargs);
	JPValue jv = cls->newInstance(frame, args);

	// If this type is abstract (kept layout-trivial so it stays compatible
	// as a mixin base for any foreign family, e.g. boxed Number), redirect
	// the actual allocation to its hidden concrete companion. offset is no
	// longer usable to detect this: it is a hard invariant now, flattened to
	// the same real value on both halves of an abstract/concrete pair (see
	// the concreteCall branch of PyJPClass_init), so an abstract type's own
	// offset is just as real a number as its companion's. tp_concrete is
	// the actual "am I abstract" signal -- it is set only on the abstract
	// half of such a pair.
	PyTypeObject *allocType = PyJPClass_getConcrete(type);
	if (allocType == nullptr)
		allocType = type;

	// If it succeeded then allocate memory
	PyObject *self = allocType->tp_alloc(allocType, 0);
	JP_PY_CHECK();

	JP_FAULT_RETURN("PyJPObject_init.null", self);

	if (allocType != type)
	{
		// Polymorph back to the canonical/abstract type the caller actually
		// named, so type(instance) stays consistent with jpype.JClass(...)
		// and any other identity-sensitive code -- mirrors the legacy
		// PyJPValue_alloc's own Py_SET_TYPE trick (see pyjp_value.cpp).
		// The companion only ever exists as a transient allocation-sizing
		// vehicle; no live object should ever report it as its type.
		Py_INCREF(type);
		Py_SET_TYPE(self, type);
		Py_DECREF(allocType);
	}

	PyJPValue_assignJavaSlot(frame, self, jv);
	return self;
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPObject_compare(PyObject *self, PyObject *other, int op)
{
	JP_PY_TRY("PyJPObject_compare");
	if (op == Py_NE)
	{
		PyObject *ret = PyJPObject_compare(self, other, Py_EQ);
		if (ret == nullptr)
			return nullptr;
		int rc = (ret == Py_False);
		Py_DECREF(ret);
		return PyBool_FromLong(rc);
	}
	if (op != Py_EQ)
	{
		PyObject *out = Py_NotImplemented;
		Py_INCREF(out);
		return out;
	}

	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass *cls0 = PyJPValue_getJPClass(self);
	JPClass *cls1 = PyJPValue_getJPClass(other);
	jobject obj0 = (cls0 != nullptr) ? PyJPValue_getJValue(frame, self).l : nullptr;

	// First slot is Null
	if (cls0 == nullptr || obj0 == nullptr)
	{
		if (cls1 == nullptr)
			return PyBool_FromLong(other == Py_None);
		if (cls1->isPrimitive())
			Py_RETURN_FALSE;
		if (PyJPValue_getJValue(frame, other).l == nullptr)
			Py_RETURN_TRUE;
		Py_RETURN_FALSE;
	}

	// Check second slot is Null
	if (other == Py_None)
		Py_RETURN_FALSE;
	if (cls1 == nullptr)
	{
		// This block seems like a giant waste as there are very few cases in which
		// a converted object would ever satisfy equals.  But this was the original
		// logic in JPype so we will try to match it.
		JPMatch match(&frame, other);
		cls0->findJavaConversion(match);
		if (match.type < JPMatch::_implicit)
			Py_RETURN_FALSE;
		return PyBool_FromLong(frame.equals(obj0, match.convert().l));
	}
	if (cls1->isPrimitive())
		Py_RETURN_FALSE;
	jobject obj1 = PyJPValue_getJValue(frame, other).l;
	if (obj1 == nullptr)
		Py_RETURN_FALSE;

	return PyBool_FromLong(frame.equals(obj0, obj1));
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static PyObject *PyJPComparable_compare(PyObject *self, PyObject *other, int op)
{
	JP_PY_TRY("PyJPComparable_compare");
	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass *cls0 = PyJPValue_getJPClass(self);
	JPClass *cls1 = PyJPValue_getJPClass(other);

	bool null0 = false;
	bool null1 = false;

	// First slot is Null
	if (self == Py_None || cls0 == nullptr ||
			(!cls0->isPrimitive() && PyJPValue_getJValue(frame, self).l == nullptr))
		null0  = true;
	if (other == Py_None || (cls1 != nullptr &&
			!cls1->isPrimitive() && PyJPValue_getJValue(frame, other).l == nullptr))
		null1  = true;

	jobject obj0 = nullptr;
	jobject obj1 = nullptr;

	if (!null0)
		obj0 = PyJPValue_getJValue(frame, self).l;

	if (!null0 && !null1 && cls1 == nullptr)
	{
		// Okay here is the hard part.  We need to figure out what type
		// of object to create to make them comparable.  We can't assume
		// the most derived type because some classes inherit comparable
		// and that would require the derived type.  We have to find
		// the first super class that implements Comparable.  Further,
		// because of type erasure we can't actually get.
		JPClass *cls2 = cls0;
		JPMatch match(&frame, other);
		while (cls2 != nullptr && !cls2->findJavaConversion(match) && !JPModifier::isComparable(cls2->getModifiers()))
			cls2 = cls2->getSuperClass();

		// This should never happen.
		if (cls2 == nullptr)
		{
			PyObject *out = Py_NotImplemented;
			Py_INCREF(out);
			return out;
		}
		if (match.type < JPMatch::Type::_implicit)
		{
			if (op == Py_EQ || op == Py_NE)
				return  PyBool_FromLong(op == Py_NE);
			PyObject *out = Py_NotImplemented;
			Py_INCREF(out);
			return out;
		}
		obj1 = match.convert().l;
	} else if (!null1 && cls1 != nullptr && !cls1->isPrimitive())
		obj1 = PyJPValue_getJValue(frame, other).l;

	switch (op)
	{
		case Py_EQ:
			if (null0 && null1)
				Py_RETURN_TRUE;
			if (null0 || null1)
				Py_RETURN_FALSE;
			return PyBool_FromLong(frame.equals(obj0, obj1));
		case Py_NE:
			if (null0 && null1)
				Py_RETURN_FALSE;
			if (null0 || null1)
				Py_RETURN_TRUE;
			return PyBool_FromLong(!frame.equals(obj0, obj1));
		case Py_LT:
			if (null0 || null1)
				break;
			return PyBool_FromLong(frame.compareTo(obj0, obj1) < 0);
		case Py_LE:
			if (null0 || null1)
				break;
			return PyBool_FromLong(frame.compareTo(obj0, obj1) <= 0);
		case Py_GT:
			if (null0 || null1)
				break;
			return PyBool_FromLong(frame.compareTo(obj0, obj1) > 0);
		case Py_GE:
			if (null0 || null1)
				break;
			return PyBool_FromLong(frame.compareTo(obj0, obj1) >= 0);
	}
	PyErr_SetString(PyExc_ValueError, "can't compare null");
	JP_PY_CATCH(nullptr);  // GCOVR_EXCL_LINE
}

static Py_hash_t PyJPObject_hash(PyObject *obj)
{
	JP_PY_TRY("PyJPObject_hash");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (PyJPValue_getJPClass(obj) == nullptr)
		return Py_TYPE(Py_None)->tp_hash(Py_None);
	jobject o = PyJPValue_getJValue(frame, obj).l;
	if (o == nullptr)
		return Py_TYPE(Py_None)->tp_hash(Py_None);
	return frame.hashCode(o);
	JP_PY_CATCH(0);
}

static PyObject *PyJPObject_repr(PyObject *self)
{
	JP_PY_TRY("PyJPObject_repr");
	return PyUnicode_FromFormat("<java object '%s'>", Py_TYPE(self)->tp_name);
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static PyObject *PyJPObject_initSubclass(PyObject *cls, PyObject* args, PyObject *kwargs)
{
    Py_RETURN_NONE;
}

static PyMethodDef objectMethods[] = {
	{"__init_subclass__", (PyCFunction) PyJPObject_initSubclass, METH_CLASS | METH_VARARGS | METH_KEYWORDS, ""},
    {0}
};

static PyType_Slot objectSlots[] = {
	{Py_tp_finalize,    (void*) &PyJPValue_finalize},
	{Py_tp_new,      (void*) &PyJPObject_new},
	{Py_tp_free,     (void*) &PyJPValue_free},
	{Py_tp_getattro, (void*) &PyJPValue_getattro},
	{Py_tp_setattro, (void*) &PyJPValue_setattro},
	{Py_tp_str,      (void*) &PyJPValue_str},
	{Py_tp_repr,     (void*) &PyJPObject_repr},
	{Py_tp_richcompare, (void*) &PyJPObject_compare},
	{Py_tp_hash,     (void*) &PyJPObject_hash},
	{Py_tp_methods,  (void*) objectMethods},
	{0}
};

PyTypeObject *PyJPObject_Type = nullptr;
static PyType_Spec objectSpec = {
	"_jpype._JObject",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	objectSlots
};

static PyObject *PyJPException_new(PyTypeObject *type, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPException_new");
	// Get the Java class from the type.
	JPClass *cls = PyJPClass_getJPClass((PyObject*) type);
	if (cls == nullptr)
	{  // GCOVR_EXCL_START
		PyErr_SetString(PyExc_TypeError, "Java class type is incorrect");
		return nullptr;
	}  // GCOVR_EXCL_STOP

	// Special constructor path for Exceptions
	JPJavaFrame frame = JPJavaFrame::outer();
	JPPyObjectVector args(pyargs);
	if (args.size() == 2 && args[0] == _JObjectKey)
		return ((PyTypeObject*) PyExc_BaseException)->tp_new(type, args[1], kwargs);

	// Create an instance (this may fail)
	JPValue jv = cls->newInstance(frame, args);

	// Exception must be constructed with the BaseException_new
	PyObject *self = ((PyTypeObject*) PyExc_BaseException)->tp_new(type, pyargs, kwargs);
	JP_PY_CHECK();

	JP_FAULT_RETURN("PyJPException_init.null", self);
	PyJPValue_assignJavaSlot(frame, self, jv);
	return self;
	JP_PY_CATCH(nullptr);  // GCOVR_EXCL_LINE
}

static int PyJPException_init(PyObject *self, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPException_init");
	JPPyObjectVector args(pyargs);
	if (args.size() == 2 && args[0] == _JObjectKey)
		return ((PyTypeObject*) PyExc_BaseException)->tp_init(self, args[1], kwargs);

	// Exception must be constructed with the BaseException_new
	return ((PyTypeObject*) PyExc_BaseException)->tp_init(self, pyargs, kwargs);
	JP_PY_CATCH(-1);  // GCOVR_EXCL_LINE
}

static PyObject* PyJPException_expandStacktrace(PyObject* self)
{
	JP_PY_TRY("PyJPModule_expandStackTrace");
	JPJavaFrame frame = JPJavaFrame::outer();

	// These two are loop invariants and must match each time
	auto th = (jthrowable) PyJPValue_getJValue(frame, self).l;
	JPPyObject exc = JPPyObject::use(self);
	PyJPException_normalize(frame, exc, th, nullptr);

	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);  // GCOVR_EXCL_LINE
}

PyObject *PyJPException_args(PyBaseExceptionObject *self)
{
	if (self->args == nullptr)
		Py_RETURN_NONE;  // GCOVR_EXCL_LINE
	Py_INCREF(self->args);
	return self->args;
}

static PyMethodDef exceptionMethods[] = {
	{"_expandStacktrace", (PyCFunction) PyJPException_expandStacktrace, METH_NOARGS, ""},
	{nullptr},
};

static PyGetSetDef exceptionGetSets[] = {
	{"_args", (getter) PyJPException_args, nullptr, ""},
	{nullptr}
};

PyTypeObject *PyJPException_Type = nullptr;
static PyType_Slot excSlots[] = {
	{Py_tp_new,      (void*) &PyJPException_new},
	{Py_tp_init,     (void*) &PyJPException_init},
	{Py_tp_str,      (void*) &PyJPValue_str},
	{Py_tp_getattro, (void*) &PyJPValue_getattro},
	{Py_tp_setattro, (void*) &PyJPValue_setattro},
	{Py_tp_methods,  exceptionMethods},
	{Py_tp_getset,   exceptionGetSets},
	{0}
};

// Exception has a real, compile-time-known C layout (unlike plain Object,
// whose layout varies with whatever it's mixed with), so it can be given a
// concrete fixed offset directly -- no abstract/companion dance needed.
// Java exceptions are always single-inheritance below Throwable, so there's
// never a risk of this concrete layout conflicting with a foreign family
// (that risk is specific to Object-lineage interfaces getting mixed into
// boxed Number/Buffer/Array/Char, which stay legacy).
struct PyJPException
{
	PyBaseExceptionObject base;
	// Bare jvalue, not a full JPValue -- class is always derived from the
	// wrapper type (PyJPValue_getJPClass), so there's nothing to look up a
	// JPValue for here.
	jvalue extra;
};

static PyType_Spec excSpec = {
	"_jpype._JException",
	sizeof (struct PyJPException),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	excSlots
};

static PyType_Slot comparableSlots[] = {
	{Py_tp_richcompare, (void*) &PyJPComparable_compare},
	{Py_tp_hash,     (void*) &PyJPObject_hash},
	{0}
};

PyTypeObject *PyJPComparable_Type = nullptr;
static PyType_Spec comparableSpec = {
	"_jpype._JComparable",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	comparableSlots
};

#ifdef __cplusplus
}
#endif

void PyJPObject_initType(PyObject* module)
{
    // -1: abstract.  Object's layout must stay byte-identical to `object` so
    // it can be mixed, via ordinary Java interface implementation, into any
    // foreign family (boxed Number/Buffer/Array/Char) without CPython
    // raising "multiple bases have instance lay-out conflict".  A hidden
    // concrete companion (see PyJPClass_concrete) is created immediately and
    // used transparently at construction time (PyJPObject_new).
    PyJPObject_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&objectSpec, nullptr, -1);
    JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JObject", (PyObject*) PyJPObject_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
    JPPyObject bases = JPPyTuple_Pack(PyExc_Exception, PyJPObject_Type);
	PyJPException_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&excSpec, bases.get(),
			offsetof (struct PyJPException, extra));
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JException", (PyObject*) PyJPException_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE

	// Comparable is a pure interface mixin: it adds no fields of its own, so
	// it is abstract too (like Object) rather than concrete, and gets its
	// own (never actually used in practice, but harmless) hidden companion.
	bases = JPPyTuple_Pack(PyJPObject_Type);
	PyJPComparable_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&comparableSpec, bases.get(), -1);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JComparable", (PyObject*) PyJPComparable_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
}

/**
 * Attach stack frames and causes as required for a Python exception.
 */
void PyJPException_normalize(JPJavaFrame frame, JPPyObject exc, jthrowable th, jthrowable enclosing)
{
	JP_TRACE_IN("PyJPException_normalize");
	JPContext *context = PyJPModule_getContext();
	while (th != nullptr)
	{
		// Attach the frame to first
		JPPyObject trace = PyTrace_FromJavaException(frame, th, enclosing);
		if (trace.get() != nullptr)
			PyException_SetTraceback(exc.get(), trace.get());

		// Check for the next in the cause list
		enclosing = th;
		th = frame.getCause(th);
		if (th == nullptr)
			return;
		jvalue v;
		v.l = (jobject) th;
		JPPyObject next = context->_java_lang_Object->convertToPythonObject(frame, v, false);

		// This may already be a Python exception
		if (PyJPValue_getJPClass(next.get()) == nullptr)
		{
			PyException_SetCause(exc.get(), next.keep());
			return;
		}
		next.incref();  // Set cause will steal our reference
		PyException_SetCause(exc.get(), next.get());
		exc = next;
	}
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}
