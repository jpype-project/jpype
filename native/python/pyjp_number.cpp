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

// Long/Boolean keep no trailing per-instance storage at all: the
// PyLongObject itself, and tp_jvalue (see longJValue below) reconstructs a
// jvalue from it on demand. With no appended slot to protect, this builds
// the digits directly into a single instance of the real subtype, rather
// than routing through PyLong_Type.tp_new -- which for an int subtype
// (long_subtype_new) allocates and fills a throwaway base-PyLong first via
// PyNumber_Long, then allocates a second, real instance of the subtype and
// copies the digits across. Every array element pulled through this path
// paid for two allocations plus a tuple pack/unpack instead of one.
//
// Digit layout is duplicated per CPython version boundary (confirmed
// against the CPython source tree directly, tags v3.10.0 through v3.14.0):
//   - <=3.11: struct _longobject { PyObject_VAR_HEAD; digit ob_digit[1]; };
//     sign is the sign of ob_size.
//   - >=3.12: struct _longobject { PyObject_HEAD; _PyLongValue long_value; }
//     where long_value = { uintptr_t lv_tag; digit ob_digit[1]; }. lv_tag's
//     low 2 bits are sign (0=positive,1=zero,2=negative), bit 2 is the
//     immortal-object flag (0 for our freshly allocated objects), lv_tag>>3
//     is the digit count.
PyObject* PyJPNumber_longFromLongLong(PyTypeObject* type, long long value)
{
	// Magnitude via unsigned negation so INT64_MIN doesn't overflow.
	unsigned long long mag = (value < 0)
			? (0ULL - (unsigned long long) value)
			: (unsigned long long) value;

#if PYLONG_BITS_IN_DIGIT == 30
#define JLONG_MAX_DIGITS 3 /* 3*30 = 90 >= 64 bits */
#elif PYLONG_BITS_IN_DIGIT == 15
#define JLONG_MAX_DIGITS 5 /* 5*15 = 75 >= 64 bits */
#else
#error "Unexpected PYLONG_BITS_IN_DIGIT"
#endif

	digit digits[JLONG_MAX_DIGITS];
	unsigned long long m = mag;
	for (int i = 0; i < JLONG_MAX_DIGITS; i++)
	{
		digits[i] = (digit) (m & PyLong_MASK);
		m >>= PyLong_SHIFT;
	}
	int ndigits = JLONG_MAX_DIGITS;
	while (ndigits > 0 && digits[ndigits - 1] == 0)
		ndigits--;
#undef JLONG_MAX_DIGITS

	// Allocate exactly the digits this value needs -- no appended slot
	// means no fixed budget to protect, unlike the earlier version of this
	// function that reserved a worst-case-width digit array.
	auto* self = (PyLongObject*) type->tp_alloc(type, ndigits);
	if (self == nullptr)
		return nullptr;

#if PY_VERSION_HEX >= 0x030c0000
	int sign_code = (mag == 0) ? 1 : (value < 0 ? 2 : 0);
	self->long_value.lv_tag = ((uintptr_t) ndigits << 3) | (uintptr_t) sign_code;
	for (int i = 0; i < ndigits; i++)
		self->long_value.ob_digit[i] = digits[i];
#else
	Py_SET_SIZE(self, (value < 0) ? -ndigits : ndigits);
	for (int i = 0; i < ndigits; i++)
		self->ob_digit[i] = digits[i];
#endif
	return (PyObject*) self;
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

// Concrete leaf classes (JByte/JShort/JInt/JLong/JFloat/JDouble/JBoolean)
// used to be plain Python `class JXxx(_jpype._JYyy, internal=True): pass`
// statements in jpype/types.py. CPython's type_new unconditionally sets
// Py_TPFLAGS_HAVE_GC on any heap type it creates -- even one instantiated
// through this metaclass -- so every one of those classes silently picked
// up GC tracking that their non-GC family root (built via
// PyJPClass_FromSpecWithBases, bypassing type_new) deliberately avoids.
// None of them can ever hold an arbitrary Python reference (tp_dictoffset
// is 0, inherited from the root), so they can provably never participate
// in a reference cycle -- the GC bookkeeping was pure per-instance
// allocation/deallocation overhead. Building them the same way as their
// root, with no additional slots, gets them the same non-GC treatment.
static PyType_Slot leafSlots[] = {
	{0}
};

// Dotted names, like the family roots, even though these are leaves: if
// spec->name has no dot, CPython's own type-from-spec machinery (seeing no
// pre-existing "__module__" in the freshly built tp_dict) raises a
// DeprecationWarning ("builtin type JInt has no __module__ attribute")
// instead of silently defaulting one -- so a dot has to be there at
// creation time to keep that quiet. tp_name (and hence repr(), which
// prints tp_name verbatim -- see PyJPClass_repr) is fixed back up to the
// plain name below, after creation, alongside the __module__ override.
static PyType_Spec byteSpec = {"_jpype.JByte", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, leafSlots};
static PyType_Spec shortSpec = {"_jpype.JShort", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, leafSlots};
static PyType_Spec intSpec = {"_jpype.JInt", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, leafSlots};
static PyType_Spec longSpec = {"_jpype.JLong", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, leafSlots};
static PyType_Spec floatSpec = {"_jpype.JFloat", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, leafSlots};
static PyType_Spec doubleSpec = {"_jpype.JDouble", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, leafSlots};
static PyType_Spec booleanLeafSpec = {"_jpype.JBoolean", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, leafSlots};

// Builds one of the leaf types above as a single-inheritance child of
// `base`, registers it into the module under `attrName`, and restores both
// tp_name and __module__ to what these classes had as ordinary
// jpype/types.py class statements ("JInt" / "jpype.types") -- see the spec
// table comment above for tp_name, and PyJPClass_FromSpecWithBases always
// baking in __module__ "_jpype" (right for the family roots, genuinely
// defined in this file, but not for these leaves).
static PyTypeObject* PyJPNumber_createLeaf(PyType_Spec *spec, PyTypeObject *base,
		Py_ssize_t offset, PyObject *module, const char *attrName)
{
	JPPyObject bases = JPPyTuple_Pack(base);
	auto *type = (PyTypeObject*) PyJPClass_FromSpecWithBases(spec, bases.get(), offset);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	// Types built through PyJPClass_FromSpecWithBases are permanent for the
	// JVM's session (never deallocated), so replacing tp_name with a static
	// string here -- distinct from the heap-allocated buffer type_dealloc
	// would otherwise free via _ht_tpname -- is safe: that buffer is simply
	// never freed, exactly like every other permanent resource this family
	// of types already holds onto for the process lifetime.
	type->tp_name = attrName;
	PyDict_SetItemString(type->tp_dict, "__module__", PyUnicode_FromString("jpype.types"));
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, attrName, (PyObject*) type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	return type;
}

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

	// The eight concrete leaves. Each shares its root's sentinel offset
	// (identical layout, no new fields), and each inherits longJValue/
	// tp_jvalue from its root via the tp_base walk in
	// PyJPClass_GetJValueFn -- nothing extra to wire up here.
	PyJPNumber_createLeaf(&byteSpec, PyJPNumberLong_Type, longOffset, module, "JByte");
	PyJPNumber_createLeaf(&shortSpec, PyJPNumberLong_Type, longOffset, module, "JShort");
	PyJPNumber_createLeaf(&intSpec, PyJPNumberLong_Type, longOffset, module, "JInt");
	PyJPNumber_createLeaf(&longSpec, PyJPNumberLong_Type, longOffset, module, "JLong");
	PyJPNumber_createLeaf(&floatSpec, PyJPNumberFloat_Type, offsetof (struct PyJPFloat, extra), module, "JFloat");
	PyJPNumber_createLeaf(&doubleSpec, PyJPNumberFloat_Type, offsetof (struct PyJPFloat, extra), module, "JDouble");
	PyJPNumber_createLeaf(&booleanLeafSpec, PyJPNumberBool_Type, longOffset, module, "JBoolean");
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
