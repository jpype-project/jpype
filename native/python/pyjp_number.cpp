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
#include "jp_boxedtype.h"
#include <cstddef>

// Float has a genuine compile-time-known C layout (like Exception), so it
// gets a real struct and a direct offsetof-based concrete offset -- no
// digit-budget reasoning needed since PyFloatObject is always fixed-size.
struct PyJPFloat
{
	PyFloatObject base;
	// Bare jvalue, not a full JPValue -- class is always derived from the
	// wrapper type (PyJPValue_getJPClass), so there's nothing to look up a
	// JPValue for here.
	jvalue extra;
};

// Long/Boolean no longer keep any trailing per-instance storage at all: the
// PyLongObject itself, and tp_jvalue (see longJValue below) reconstructs a
// jvalue from it on demand -- boxing via a real JNI call when the context is
// a boxed wrapper, reading the digits directly when it's a primitive. So
// there's no fixed offset to protect any more, and ordinary CPython subtype
// construction (long_subtype_new, dispatched via PyLong_Type.tp_new with the
// real subtype) is exactly the right tool -- the earlier hand-written-digit
// version existed solely to keep a trailing JPValue at a constant offset.
PyObject* PyJPNumber_longFromLongLong(PyTypeObject* type, long long value)
{
	JPPyObject tmp = JPPyObject::call(PyLong_FromLongLong(value));
	JPPyObject args = JPPyTuple_Pack(tmp.get());
	return PyLong_Type.tp_new(type, args.get(), nullptr);
}

static PyObject* newFloatFixed(PyTypeObject* type, double value)
{
	auto* self = (PyFloatObject*) type->tp_alloc(type, 0);
	if (self == nullptr)
		return nullptr;
	self->ob_fval = value;
	return (PyObject*) self;
}

static bool isNull(PyObject *self)
{
	JPClass *cls = PyJPValue_getJPClass(self);
	if (cls == nullptr || cls->isPrimitive())
		return false;
	// Reconstructing families (Long/Boolean, see longJValue) never carry a
	// jvalue that could be null -- tp_jvalue always boxes a live value.
	// A real Java null is instead represented by a per-class singleton
	// instance (see PyJPClass_GetNullBoxed / jp_boxedtype.cpp), so null-ness
	// is an identity check, not a value read.
	if (PyJPClass_GetJValueFn(Py_TYPE(self)) != nullptr)
		return self == PyJPClass_GetNullBoxed(Py_TYPE(self));
	JPJavaFrame frame = JPJavaFrame::outer();
	return PyJPValue_getJValue(frame, self).l == nullptr;
}

// tp_jvalue for the Long/Boolean family: reconstructs a jvalue from the
// PyLong's own digits, boxing via a real JNI call only when this instance's
// class is a boxed wrapper. The null-boxed singleton is special-cased first
// so it costs a pointer compare, not a JNI round trip.
static jvalue longJValue(JPJavaFrame& frame, PyObject* self)
{
	JPClass *cls = PyJPValue_getJPClass(self);
	jvalue prim{};
	prim.j = (jlong) PyLong_AsLongLong(self);
	if (cls == nullptr || cls->isPrimitive())
		return prim;
	if (self == PyJPClass_GetNullBoxed(Py_TYPE(self)))
	{
		jvalue null_{};
		return null_;
	}
	jvalue out{};
	out.l = (dynamic_cast<JPBoxedType*>(cls))->box(frame, prim);
	return out;
}

#ifdef __cplusplus
extern "C"
{
#endif

static PyObject *PyJPNumber_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPNumber_new", type);
	auto *cls = (JPClass*) PyJPClass_getJPClass((PyObject*) type);
	if (cls == nullptr)
		JP_RAISE(PyExc_TypeError, "Class type incorrect");

	JPJavaFrame frame = JPJavaFrame::outer();
	jvalue val;
	// One argument tries Java conversion first
	if (PyTuple_Size(args) == 1)
	{
		PyObject *arg = PyTuple_GetItem(args, 0);
		JPMatch match(&frame, arg);
		cls->findJavaConversion(match);
		if (match.type >= JPMatch::_implicit)
		{
			// Disable OverrangeError
			match.type = JPMatch::_exact;
			val = match.convert();
			PyObject *obj = cls->convertToPythonObject(frame, val, true).keep();
			return obj;
		}
	}

	if (PyObject_IsSubclass((PyObject*) type, (PyObject*) & PyLong_Type))
	{
		JPPyObject self = JPPyObject::call(PyLong_Type.tp_new(&PyLong_Type, args, kwargs));
		JPMatch match(&frame, self.get());
		cls->findJavaConversion(match);
		match.type = JPMatch::_exact;
		val = match.convert();
		return cls->convertToPythonObject(frame, val, true).keep();
	} else if (PyObject_IsSubclass((PyObject*) type, (PyObject*) & PyFloat_Type))
	{
		JPPyObject self = JPPyObject::call(PyFloat_Type.tp_new(&PyFloat_Type, args, kwargs));
		JPMatch match(&frame, self.get());
		cls->findJavaConversion(match);
		match.type = JPMatch::_exact;
		val = match.convert();
		return cls->convertToPythonObject(frame, val, true).keep();
	} else
	{
		PyErr_Format(PyExc_TypeError, "Type '%s' is not a number class", type->tp_name);
		return nullptr;
	}
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberLong_int(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_int");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (!isNull(self))
		return PyLong_Type.tp_as_number->nb_int(self);
	PyErr_SetString(PyExc_TypeError, "cast of null pointer would return non-int");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberLong_float(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_float");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (!isNull(self))
		return PyLong_Type.tp_as_number->nb_float(self);
	PyErr_SetString(PyExc_TypeError, "cast of null pointer would return non-float");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_int(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_int");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (!isNull(self))
		return PyFloat_Type.tp_as_number->nb_int(self);
	PyErr_SetString(PyExc_TypeError, "cast of null pointer would return non-int");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_float(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_float");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (!isNull(self))
		return PyFloat_Type.tp_as_number->nb_float(self);
	PyErr_SetString(PyExc_TypeError, "cast of null pointer would return non-float");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberLong_str(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_str");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	return PyLong_Type.tp_str(self);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_str(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_str");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	return PyFloat_Type.tp_str(self);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberLong_repr(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_repr");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	return PyLong_Type.tp_repr(self);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_repr(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_repr");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	return PyFloat_Type.tp_repr(self);
	JP_PY_CATCH(nullptr);
}

static const char* op_names[] = {
	"<", "<=", "==", "!=", ">", ">="
};

static PyObject *PyJPNumberLong_compare(PyObject *self, PyObject *other, int op)
{
	JP_PY_TRY("PyJPNumberLong_compare");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (isNull(self))
	{
		if (op == Py_EQ)
			return PyBool_FromLong(other == Py_None);
		if (op == Py_NE)
			return PyBool_FromLong(other != Py_None);
		PyErr_Format(PyExc_TypeError, "'%s' not supported with null pointer", op_names[op]);
		JP_RAISE_PYTHON();
	}
	if (!PyNumber_Check(other))
	{
		PyObject *out = Py_NotImplemented;
		Py_INCREF(out);
		return out;
	}
	return PyLong_Type.tp_richcompare(self, other, op);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_compare(PyObject *self, PyObject *other, int op)
{
	JP_PY_TRY("PyJPNumberFloat_compare");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (isNull(self))
	{
		if (op == Py_EQ)
			return PyBool_FromLong(other == Py_None);
		if (op == Py_NE)
			return PyBool_FromLong(other != Py_None);
		PyErr_Format(PyExc_TypeError, "'%s' not supported with null pointer", op_names[op]);
		JP_RAISE_PYTHON();
	}
	if (!PyNumber_Check(other)) // || Py_TYPE(other) == (PyTypeObject*) _JChar)
	{
		PyObject *out = Py_NotImplemented;
		Py_INCREF(out);
		return out;
	}
	return PyFloat_Type.tp_richcompare(self, other, op);
	JP_PY_CATCH(nullptr);
}

static Py_hash_t PyJPNumberLong_hash(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_hash");
	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass *cls = PyJPValue_getJPClass(self);
	if (cls == nullptr)
		return Py_TYPE(Py_None)->tp_hash(Py_None);
	if (!cls->isPrimitive())
	{
		jobject o = PyJPValue_getJValue(frame, self).l;
		if (o == nullptr)
			return Py_TYPE(Py_None)->tp_hash(Py_None);
	}
	return PyLong_Type.tp_hash(self);
	JP_PY_CATCH(0);
}

static Py_hash_t PyJPNumberFloat_hash(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_hash");
	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass *cls = PyJPValue_getJPClass(self);
	if (cls == nullptr)
		return Py_TYPE(Py_None)->tp_hash(Py_None);
	if (!cls->isPrimitive())
	{
		jobject o = PyJPValue_getJValue(frame, self).l;
		if (o == nullptr)
			return Py_TYPE(Py_None)->tp_hash(Py_None);
	}
	return PyFloat_Type.tp_hash(self);
	JP_PY_CATCH(0);
}

static PyObject *PyJPBoolean_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPBoolean_new", type);
	if (PyTuple_Size(args) != 1)
	{
		PyErr_SetString(PyExc_TypeError, "Requires one argument");
		return nullptr;
	}
	int i = PyObject_IsTrue(PyTuple_GetItem(args, 0));
	JPClass *cls = PyJPClass_getJPClass((PyObject*) type);
	if (cls == nullptr)
	{
		PyErr_SetString(PyExc_TypeError, "Class type incorrect");
		return nullptr;
	}
	JPPyObject self = JPPyObject::call(PyJPNumber_longFromLongLong(type, i));
	JP_PY_CHECK();
	JPJavaFrame frame = JPJavaFrame::outer();
	JPMatch match(&frame, self.get());
	cls->findJavaConversion(match);
	jvalue val = match.convert();
	PyJPValue_assignJavaSlot(frame, self.get(), JPValue(cls, val));
	JP_TRACE("new", self.get());
	return self.keep();
	JP_PY_CATCH(nullptr);
}

static PyObject* PyJPBoolean_str(PyObject* self)
{
	JP_PY_TRY("PyJPBoolean_str", self);
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	if (PyLong_AsLong(self) == 0)
		return Py_TYPE(Py_False)->tp_str(Py_False);
	return Py_TYPE(Py_True)->tp_str(Py_True);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumber_initSubclass(PyObject *cls, PyObject* args, PyObject *kwargs)
{
        Py_RETURN_NONE;
}

static PyMethodDef numberMethods[] = {
    {"__init_subclass__", (PyCFunction) PyJPNumber_initSubclass, METH_CLASS | METH_VARARGS | METH_KEYWORDS, ""},
    {0}
};


static PyType_Slot numberLongSlots[] = {
	{Py_tp_new,      (void*) &PyJPNumber_new},
	{Py_tp_getattro, (void*) &PyJPValue_getattro},
	{Py_tp_setattro, (void*) &PyJPValue_setattro},
	{Py_nb_int,      (void*) &PyJPNumberLong_int},
	{Py_nb_float,    (void*) &PyJPNumberLong_float},
	{Py_tp_str,      (void*) &PyJPNumberLong_str},
	{Py_tp_repr,     (void*) &PyJPNumberLong_repr},
	{Py_tp_hash,     (void*) &PyJPNumberLong_hash},
	{Py_tp_richcompare, (void*) &PyJPNumberLong_compare},
	{Py_tp_methods,  (void*) numberMethods},
	{0}
};

PyTypeObject *PyJPNumberLong_Type = nullptr;
PyType_Spec numberLongSpec = {
	"_jpype._JNumberLong",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	numberLongSlots
};

static PyType_Slot numberFloatSlots[] = {
	{Py_tp_new,      (void*) &PyJPNumber_new},
	{Py_tp_getattro, (void*) &PyJPValue_getattro},
	{Py_tp_setattro, (void*) &PyJPValue_setattro},
	{Py_nb_int,      (void*) &PyJPNumberFloat_int},
	{Py_nb_float,    (void*) &PyJPNumberFloat_float},
	{Py_tp_str,      (void*) &PyJPNumberFloat_str},
	{Py_tp_repr,     (void*) &PyJPNumberFloat_repr},
	{Py_tp_hash,     (void*) &PyJPNumberFloat_hash},
	{Py_tp_richcompare, (void*) &PyJPNumberFloat_compare},
	{Py_tp_methods,  (void*) numberMethods},
	{0}
};

PyTypeObject *PyJPNumberFloat_Type = nullptr;
PyType_Spec numberFloatSpec = {
	"_jpype._JNumberFloat",
	sizeof (struct PyJPFloat),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	numberFloatSlots
};

static PyType_Slot numberBooleanSlots[] = {
	{Py_tp_new,      (void*) PyJPBoolean_new},
	{Py_tp_getattro, (void*) PyJPValue_getattro},
	{Py_tp_setattro, (void*) PyJPValue_setattro},
	{Py_tp_str,      (void*) PyJPBoolean_str},
	{Py_tp_repr,     (void*) PyJPBoolean_str},
	{Py_nb_int,      (void*) PyJPNumberLong_int},
	{Py_nb_float,    (void*) PyJPNumberLong_float},
	{Py_tp_hash,     (void*) PyJPNumberLong_hash},
	{Py_tp_richcompare, (void*) PyJPNumberLong_compare},
	{Py_tp_methods,  (void*) numberMethods},
	{0}
};

PyTypeObject *PyJPNumberBool_Type = nullptr;
PyType_Spec numberBooleanSpec = {
	"_jpype._JBoolean",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	numberBooleanSlots
};

#ifdef __cplusplus
}
#endif

void PyJPNumber_initType(PyObject* module)
{
	// Long/Boolean keep no per-instance storage at all any more (see
	// longJValue above), so the offset passed to PyJPClass_FromSpecWithBases
	// is a pure sentinel -- it only has to be nonzero to mark the family as
	// Java-backed. It is never dereferenced: PyJPValue_getJValue/
	// assignJavaSlot/finalize all check PyJPClass_GetJValueFn first and
	// return before touching instance memory at this offset.
	Py_ssize_t longOffset = (Py_ssize_t) PyLong_Type.tp_basicsize;

	JPPyObject bases = JPPyTuple_Pack(&PyLong_Type, PyJPObject_Type);
	PyJPNumberLong_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&numberLongSpec, bases.get(), longOffset);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyJPClass_SetJValueFn(PyJPNumberLong_Type, &longJValue);
	PyModule_AddObject(module, "_JNumberLong", (PyObject*) PyJPNumberLong_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE

	bases = JPPyTuple_Pack(&PyFloat_Type, PyJPObject_Type);
	PyJPNumberFloat_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&numberFloatSpec, bases.get(),
			offsetof (struct PyJPFloat, extra));
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JNumberFloat", (PyObject*) PyJPNumberFloat_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE

	// Boolean is its own family root (not a subclass of PyJPNumberLong_Type)
	// but shares the identical PyLong_Type-based layout, so it reuses the
	// same sentinel offset and jvalue-reconstruction function.
	bases = JPPyTuple_Pack(&PyLong_Type, PyJPObject_Type);
	PyJPNumberBool_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&numberBooleanSpec, bases.get(), longOffset);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyJPClass_SetJValueFn(PyJPNumberBool_Type, &longJValue);
	PyModule_AddObject(module, "_JBoolean", (PyObject*) PyJPNumberBool_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
}

JPPyObject PyJPNumber_create(JPJavaFrame &frame, JPPyObject& wrapper, const JPValue& value)
{
	JPContext *context = PyJPModule_getContext();
	// Bools are not numbers in Java
	if (value.getClass() == context->_java_lang_Boolean)
	{
		jlong l = 0;
		if (value.getValue().l != nullptr)
			l = frame.CallBooleanMethodA(value.getJavaObject(), context->_java_lang_Boolean->m_BooleanValueID, nullptr);
		return JPPyObject::call(PyJPNumber_longFromLongLong((PyTypeObject*) wrapper.get(), l));
	}
	if (PyObject_IsSubclass(wrapper.get(), (PyObject*) & PyLong_Type))
	{
		jlong l = 0;
		if (value.getValue().l != nullptr)
		{
			auto* jb = dynamic_cast<JPBoxedType*>( value.getClass());
			l = frame.CallLongMethodA(value.getJavaObject(), jb->m_LongValueID, nullptr);
		}
		return JPPyObject::call(PyJPNumber_longFromLongLong((PyTypeObject*) wrapper.get(), l));
	}
	if (PyObject_IsSubclass(wrapper.get(), (PyObject*) & PyFloat_Type))
	{
		jdouble l = 0;
		if (value.getValue().l != nullptr)
		{
			auto* jb = dynamic_cast<JPBoxedType*>( value.getClass());
			l = frame.CallDoubleMethodA(value.getJavaObject(), jb->m_DoubleValueID, nullptr);
		}
		return JPPyObject::call(newFloatFixed((PyTypeObject*) wrapper.get(), l));
	}
	JP_RAISE(PyExc_TypeError, "unable to convert");  //GCOVR_EXCL_LINE
}
