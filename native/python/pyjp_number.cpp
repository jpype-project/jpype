// --- file: python/pyjp_number.cpp ---
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
#include <atomic>

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
#if PYLONG_BITS_IN_DIGIT == 30
#define JLONG_MAX_DIGITS 3 /* 3*30 = 90 >= 64 bits */
#elif PYLONG_BITS_IN_DIGIT == 15
#define JLONG_MAX_DIGITS 5 /* 5*15 = 75 >= 64 bits */
#else
#error "Unexpected PYLONG_BITS_IN_DIGIT"
#endif

// Recycling pool for the byte/short/int/long tagged-number leaves
// (JByte/JShort/JInt/JLong -- not Float/Double, which have a genuine
// compile-time-fixed layout and don't go through tp_alloc at all here, and
// not Boolean, which gets its own two-singleton treatment below instead of
// a pool). Measured: array-pull (`list(intArray)`) construction is
// dominated by generic `tp_alloc` (PyType_GenericAlloc) -- these are
// non-builtin heap types, so they get none of CPython's own small-int
// cache or PyLong-specific allocator fast path. A pool closes most of
// that gap.
//
// One fixed-size bucket, not one per digit count: every value these four
// types can ever hold fits in JLONG_MAX_DIGITS digits, and there's nothing
// to gain from chopping the pool's memory any finer than "one block big
// enough for the largest possible jlong" -- every recycled block is
// allocated at that one size regardless of the value it happens to hold
// next, and the true digit count is written into the header
// (lv_tag/ob_size) on every reuse regardless of the block's origin.
//
// Only the four primitive leaves opt in (gated by exact type-pointer
// identity in isEligible, populated once from PyJPNumber_initType).
// Callers cannot register a Python subclass of these to bypass the check
// (`class MyInt(JInt): pass` raises "Java classes cannot be extended in
// Python" -- these leaves are final in practice despite carrying
// Py_TPFLAGS_BASETYPE), but PyJPNumber_longFromLongLong has a second,
// unrelated caller family: PyJPNumber_create boxes java.lang.Integer/
// Long/Short/Byte/Boolean return values through the very same function
// with the *boxed* class's own host type, which is a distinct
// PyTypeObject from the primitive leaf even though it shares the same
// memory layout. Pooling those too might well be safe (nothing here is
// JVM-object-identity-bearing -- see the class comment above) but is out
// of scope for this pass, so they deliberately fall through to the
// ordinary tp_alloc/tp_free path via the identity check below.
//
// Treiber stack (atomic head, CAS push/pop, next-pointer stored inline in
// the dead object's own memory) rather than a mutex -- construction here
// runs under the GIL today so a lock would cost nothing extra either way,
// but the lock-free form costs nothing extra either and stays correct if
// this is ever reached under free-threaded CPython.
//
// The pool itself lives on PyJPModuleState (st->intfreelist), not as a
// process-wide static, because it recycles raw allocated blocks -- under a
// per-interpreter GIL/allocator build (PEP 684), a block freed under one
// interpreter's obmalloc arena and popped back out under another's would
// corrupt that interpreter's heap. Every entry point below takes the
// PyJPModuleState* of the type it's working with, resolved by the caller
// the same way this file already resolves it elsewhere (via
// ((PyJPClass*) type)->m_State).
namespace intfreelist
{

struct Node
{
	std::atomic<Node*> next;
};

constexpr int CAP = 4096;

inline bool isEligible(PyJPModuleState* st, PyTypeObject* type)
{
	for (auto* t : st->intfreelist.eligibleTypes)
		if (t == type)
			return true;
	return false;
}

inline void* pop(PyJPModuleState* st)
{
	std::atomic<void*>& head = st->intfreelist.head;
	auto* n = (Node*) head.load(std::memory_order_acquire);
	while (n != nullptr)
	{
		Node* next = n->next.load(std::memory_order_relaxed);
		if (head.compare_exchange_weak((void*&) n, (void*) next, std::memory_order_acq_rel, std::memory_order_acquire))
		{
			st->intfreelist.count.fetch_sub(1, std::memory_order_relaxed);
			return (void*) n;
		}
	}
	return nullptr;
}

inline bool push(PyJPModuleState* st, void* p)
{
	std::atomic<void*>& head = st->intfreelist.head;
	if (st->intfreelist.count.load(std::memory_order_relaxed) >= CAP)
		return false;
	auto* n = (Node*) p;
	auto* old = (Node*) head.load(std::memory_order_relaxed);
	do
	{
		n->next.store(old, std::memory_order_relaxed);
	} while (!head.compare_exchange_weak((void*&) old, (void*) n, std::memory_order_release, std::memory_order_relaxed));
	st->intfreelist.count.fetch_add(1, std::memory_order_relaxed);
	return true;
}

} // namespace intfreelist

// tp_dealloc for the four pooled leaves (wired in via intFamilySlots
// below): push the block back onto the pool instead of freeing it, unless
// the pool is already at capacity. These leaves are non-GC, have no
// __dict__/weakref slot (tp_dictoffset/tp_weaklistoffset are both 0,
// inherited from the family root), so there is nothing else for a normal
// tp_dealloc to tear down first -- a bare recycle-or-free is the whole job.
static void PyJPNumberInt_freelistDealloc(PyObject* self)
{
	PyJPModuleState* st = ((PyJPClass*) Py_TYPE(self))->m_State;
	if (intfreelist::push(st, self))
		return;
	PyObject_Free(self);
}

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

	PyJPModuleState* st = ((PyJPClass*) type)->m_State;
	PyLongObject* self;
	if (intfreelist::isEligible(st, type))
	{
		self = (PyLongObject*) intfreelist::pop(st);
		if (self != nullptr)
		{
			// Reused block: still carries its previous occupant's
			// refcount/type from before it was pushed back, so both must
			// be reset before this is handed out as a live object again.
			Py_SET_REFCNT((PyObject*) self, 1);
			Py_SET_TYPE((PyObject*) self, type);
		} else
		{
			// Fresh block, but always sized for the pool's one fixed
			// capacity (JLONG_MAX_DIGITS) regardless of this value's
			// actual digit count, so it's poolable on the way back in too.
			self = (PyLongObject*) type->tp_alloc(type, JLONG_MAX_DIGITS);
			if (self == nullptr)
				return nullptr;
		}
	} else
	{
		// Allocate exactly the digits this value needs -- no appended slot
		// means no fixed budget to protect, unlike the earlier version of
		// this function that reserved a worst-case-width digit array.
		self = (PyLongObject*) type->tp_alloc(type, ndigits);
		if (self == nullptr)
			return nullptr;
	}

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
#undef JLONG_MAX_DIGITS

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
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
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
	PyJPModuleState* st = ((PyJPClass*) type)->m_State;
	JPJavaFrame frame = JPJavaFrame::outer(st->context);

	jvalue val;
	// One argument tries Java conversion first
	if (PyTuple_Size(args) == 1)
	{
		PyObject *arg = PyTuple_GetItem(args, 0);
		JPMatch match(frame, arg);
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

	if (PyObject_IsSubclass((PyObject*) type, (PyObject*) &PyLong_Type))
	{
		JPPyObject self = JPPyObject::call(PyLong_Type.tp_new(&PyLong_Type, args, kwargs));
		JPMatch match(frame, self.get());
		cls->findJavaConversion(match);
		match.type = JPMatch::_exact;
		val = match.convert();
		return cls->convertToPythonObject(frame, val, true).keep();
	} else if (PyObject_IsSubclass((PyObject*) type, (PyObject*) &PyFloat_Type))
	{
		JPPyObject self = JPPyObject::call(PyFloat_Type.tp_new(&PyFloat_Type, args, kwargs));
		JPMatch match(frame, self.get());
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
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
	if (!isNull(self))
		return PyLong_Type.tp_as_number->nb_int(self);
	PyErr_SetString(PyExc_TypeError, "cast of null pointer would return non-int");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberLong_float(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_float");
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
	if (!isNull(self))
		return PyLong_Type.tp_as_number->nb_float(self);
	PyErr_SetString(PyExc_TypeError, "cast of null pointer would return non-float");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_int(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_int");
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
	if (!isNull(self))
		return PyFloat_Type.tp_as_number->nb_int(self);
	PyErr_SetString(PyExc_TypeError, "cast of null pointer would return non-int");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_float(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_float");
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
	if (!isNull(self))
		return PyFloat_Type.tp_as_number->nb_float(self);
	PyErr_SetString(PyExc_TypeError, "cast of null pointer would return non-float");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberLong_str(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_str");
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	return PyLong_Type.tp_str(self);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_str(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_str");
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	return PyFloat_Type.tp_str(self);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberLong_repr(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_repr");
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	return PyLong_Type.tp_repr(self);
	JP_PY_CATCH(nullptr);
}


static PyObject *PyJPNumberFloat_repr(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_repr");
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
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
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
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
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
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
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
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
	JPJavaFrame frame = JPJavaFrame::outer(PyJPObject_getContext(self));
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

// A Java boolean has exactly two possible values, so -- like Python's own
// True/False -- there's no reason to allocate one past the first
// construction of each; every later JBoolean(x) just hands out an incref
// of whichever singleton matches. Safe to skip findJavaConversion/
// assignJavaSlot entirely once cached: this family's tp_jvalue
// (longJValue) reconstructs the jvalue straight from the PyLong bits on
// demand (see the comment on PyJPNumber_longFromLongLong above), so
// there's no per-instance Java slot state a reused instance could ever
// hold stale.
//
// Only the bare JBoolean leaf is cached (gated by exact type-pointer
// identity, same reasoning as intfreelist::isEligible above) -- the boxed
// java.lang.Boolean path doesn't come through here at all (PyJPNumber_create
// special-cases Boolean and calls PyJPNumber_longFromLongLong directly).
// Cached on PyJPModuleState (st->boolSingleton/st->boolLeafType), not as a
// process-wide static -- each (sub)interpreter builds its own JBoolean leaf
// type, so a cached instance from one interpreter must never be handed back
// to another.
static PyObject *PyJPBoolean_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPBoolean_new", type);
	if (PyTuple_Size(args) != 1)
	{
		PyErr_SetString(PyExc_TypeError, "Requires one argument");
		return nullptr;
	}
	PyJPModuleState* st = ((PyJPClass*) type)->m_State;
	int i = PyObject_IsTrue(PyTuple_GetItem(args, 0));
	if (type == st->boolLeafType && st->boolSingleton[i] != nullptr)
	{
		Py_INCREF(st->boolSingleton[i]);
		return st->boolSingleton[i];
	}
	JPClass *cls = PyJPClass_getJPClass((PyObject*) type);
	if (cls == nullptr)
	{
		PyErr_SetString(PyExc_TypeError, "Class type incorrect");
		return nullptr;
	}
	JPPyObject self = JPPyObject::call(PyJPNumber_longFromLongLong(type, i));
	JP_PY_CHECK();
	JPJavaFrame frame = JPJavaFrame::outer(PyJPType_getContext(type));
	JPMatch match(frame, self.get());
	cls->findJavaConversion(match);
	jvalue val = match.convert();
	PyJPValue_assignJavaSlot(frame, self.get(), JPValue(cls, val));
	JP_TRACE("new", self.get());
	if (type == st->boolLeafType && st->boolSingleton[i] == nullptr)
	{
		Py_INCREF(self.get());
		st->boolSingleton[i] = self.get();
	}
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
	{Py_tp_new,	  (void*) &PyJPNumber_new},
	{Py_tp_getattro, (void*) &PyJPValue_getattro},
	{Py_tp_setattro, (void*) &PyJPValue_setattro},
	{Py_nb_int,	  (void*) &PyJPNumberLong_int},
	{Py_nb_float,	(void*) &PyJPNumberLong_float},
	{Py_tp_str,	  (void*) &PyJPNumberLong_str},
	{Py_tp_repr,	 (void*) &PyJPNumberLong_repr},
	{Py_tp_hash,	 (void*) &PyJPNumberLong_hash},
	{Py_tp_richcompare, (void*) &PyJPNumberLong_compare},
	{Py_tp_methods,  (void*) numberMethods},
	{0}
};

PyType_Spec numberLongSpec = {
	"_jpype._JNumberLong",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	numberLongSlots
};

static PyType_Slot numberFloatSlots[] = {
	{Py_tp_new,	  (void*) &PyJPNumber_new},
	{Py_tp_getattro, (void*) &PyJPValue_getattro},
	{Py_tp_setattro, (void*) &PyJPValue_setattro},
	{Py_nb_int,	  (void*) &PyJPNumberFloat_int},
	{Py_nb_float,	(void*) &PyJPNumberFloat_float},
	{Py_tp_str,	  (void*) &PyJPNumberFloat_str},
	{Py_tp_repr,	 (void*) &PyJPNumberFloat_repr},
	{Py_tp_hash,	 (void*) &PyJPNumberFloat_hash},
	{Py_tp_richcompare, (void*) &PyJPNumberFloat_compare},
	{Py_tp_methods,  (void*) numberMethods},
	{0}
};

PyType_Spec numberFloatSpec = {
	"_jpype._JNumberFloat",
	sizeof (struct PyJPFloat),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	numberFloatSlots
};

static PyType_Slot numberBooleanSlots[] = {
	{Py_tp_new,	  (void*) PyJPBoolean_new},
	{Py_tp_getattro, (void*) PyJPValue_getattro},
	{Py_tp_setattro, (void*) PyJPValue_setattro},
	{Py_tp_str,	  (void*) PyJPBoolean_str},
	{Py_tp_repr,	 (void*) PyJPBoolean_str},
	{Py_nb_int,	  (void*) PyJPNumberLong_int},
	{Py_nb_float,	(void*) PyJPNumberLong_float},
	{Py_tp_hash,	 (void*) PyJPNumberLong_hash},
	{Py_tp_richcompare, (void*) PyJPNumberLong_compare},
	{Py_tp_methods,  (void*) numberMethods},
	{0}
};

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

// Byte/Short/Int/Long additionally override tp_dealloc to recycle through
// intfreelist instead of freeing -- see the pool comment above
// PyJPNumber_longFromLongLong. Doesn't disturb the non-GC treatment
// discussed above: dealloc is unrelated to the GC-flag inheritance being
// protected there.
static PyType_Slot intFamilyLeafSlots[] = {
	{Py_tp_dealloc, (void*) &PyJPNumberInt_freelistDealloc},
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
static PyType_Spec byteSpec = {"_jpype.JByte", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, intFamilyLeafSlots};
static PyType_Spec shortSpec = {"_jpype.JShort", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, intFamilyLeafSlots};
static PyType_Spec intSpec = {"_jpype.JInt", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, intFamilyLeafSlots};
static PyType_Spec longSpec = {"_jpype.JLong", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, intFamilyLeafSlots};
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
	auto *type = (PyTypeObject*) PyJPClass_FromSpecWithBases(module, spec, bases.get(), offset);
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

void PyJPNumber_initType(PyObject* module, PyJPModuleState* st)
{
	// Long/Boolean keep no per-instance storage at all any more (see
	// longJValue above), so the offset passed to PyJPClass_FromSpecWithBases
	// is a pure sentinel -- it only has to be nonzero to mark the family as
	// Java-backed. It is never dereferenced: PyJPValue_getJValue/
	// assignJavaSlot/finalize all check PyJPClass_GetJValueFn first and
	// return before touching instance memory at this offset.
	Py_ssize_t longOffset = (Py_ssize_t) PyLong_Type.tp_basicsize;

	JPPyObject bases = JPPyTuple_Pack(&PyLong_Type, st->PyJPObject_Type);
	st->PyJPNumberLong_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(module, &numberLongSpec, bases.get(), longOffset);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyJPClass_SetJValueFn(st->PyJPNumberLong_Type, &longJValue);
	PyModule_AddObject(module, "_JNumberLong", (PyObject*) st->PyJPNumberLong_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE

	bases = JPPyTuple_Pack(&PyFloat_Type, st->PyJPObject_Type);
	st->PyJPNumberFloat_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(module, &numberFloatSpec, bases.get(),
			offsetof (struct PyJPFloat, extra));
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JNumberFloat", (PyObject*) st->PyJPNumberFloat_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE

	// Boolean is its own family root (not a subclass of PyJPNumberLong_Type)
	// but shares the identical PyLong_Type-based layout, so it reuses the
	// same sentinel offset and jvalue-reconstruction function.
	bases = JPPyTuple_Pack(&PyLong_Type, st->PyJPObject_Type);
	st->PyJPNumberBool_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(module, &numberBooleanSpec, bases.get(), longOffset);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyJPClass_SetJValueFn(st->PyJPNumberBool_Type, &longJValue);
	PyModule_AddObject(module, "_JBoolean", (PyObject*) st->PyJPNumberBool_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE

	// The eight concrete leaves. Each shares its root's sentinel offset
	// (identical layout, no new fields), and each inherits longJValue/
	// tp_jvalue from its root via the tp_base walk in
	// PyJPClass_GetJValueFn -- nothing extra to wire up here.
	st->intfreelist.eligibleTypes[0] = PyJPNumber_createLeaf(&byteSpec, st->PyJPNumberLong_Type, longOffset, module, "JByte");
	st->intfreelist.eligibleTypes[1] = PyJPNumber_createLeaf(&shortSpec, st->PyJPNumberLong_Type, longOffset, module, "JShort");
	st->intfreelist.eligibleTypes[2] = PyJPNumber_createLeaf(&intSpec, st->PyJPNumberLong_Type, longOffset, module, "JInt");
	st->intfreelist.eligibleTypes[3] = PyJPNumber_createLeaf(&longSpec, st->PyJPNumberLong_Type, longOffset, module, "JLong");
	PyJPNumber_createLeaf(&floatSpec, st->PyJPNumberFloat_Type, offsetof (struct PyJPFloat, extra), module, "JFloat");
	PyJPNumber_createLeaf(&doubleSpec, st->PyJPNumberFloat_Type, offsetof (struct PyJPFloat, extra), module, "JDouble");
	st->boolLeafType = PyJPNumber_createLeaf(&booleanLeafSpec, st->PyJPNumberBool_Type, longOffset, module, "JBoolean");
}

JPPyObject PyJPNumber_create(JPJavaFrame &frame, JPPyObject& wrapper, const JPValue& value)
{
	JPContext *context = frame.getContext();
	// Bools are not numbers in Java
	if (value.getClass() == context->_java_lang_Boolean)
	{
		jlong l = 0;
		if (!value.isJavaNull())
			l = frame.CallBooleanMethodA(value.getJavaObject(frame), context->_java_lang_Boolean->m_BooleanValueID, nullptr);
		return JPPyObject::call(PyJPNumber_longFromLongLong((PyTypeObject*) wrapper.get(), l));
	}
	if (PyObject_IsSubclass(wrapper.get(), (PyObject*) & PyLong_Type))
	{
		jlong l = 0;
		if (!value.isJavaNull())
		{
			auto* jb = dynamic_cast<JPBoxedType*>( value.getClass());
			l = frame.CallLongMethodA(value.getJavaObject(frame), jb->m_LongValueID, nullptr);
		}
		return JPPyObject::call(PyJPNumber_longFromLongLong((PyTypeObject*) wrapper.get(), l));
	}
	if (PyObject_IsSubclass(wrapper.get(), (PyObject*) & PyFloat_Type))
	{
		jdouble l = 0;
		if (!value.isJavaNull())
		{
			auto* jb = dynamic_cast<JPBoxedType*>( value.getClass());
			l = frame.CallDoubleMethodA(value.getJavaObject(frame), jb->m_DoubleValueID, nullptr);
		}
		return JPPyObject::call(newFloatFixed((PyTypeObject*) wrapper.get(), l));
	}
	JP_RAISE(PyExc_TypeError, "unable to convert");  //GCOVR_EXCL_LINE
}
