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
#include <algorithm>
#include <cstddef>
#include <Python.h>
#include <frameobject.h>
#include <structmember.h>
#include "jpype.h"
#include "pyjp.h"
#include "jp_array.h"
#include "jp_arrayclass.h"
#include "jp_boxedtype.h"
#include "jp_field.h"
#include "jp_method.h"
#include "jp_methoddispatch.h"
#include "jp_primitive_accessor.h"

struct PyJPClass
{
	PyHeapTypeObject ht_type;
	JPClass *m_Class;
	PyObject *m_Doc;
	// Java-value slot bookkeeping for the fixed-offset object model.  See
	// the offset parameter documentation in include/pyjp.h. Once a type is
	// fully created this is ALWAYS the real, resolved byte offset -- for an
	// abstract/concrete pair, the same value is written onto BOTH halves
	// (see the concreteCall branch of PyJPClass_init), so no caller ever
	// needs to branch or chase a companion to use it. Never -1 in steady
	// state; 0 is a hard-error sentinel for legacy families that no longer
	// exist (see PyJPClass_FromSpecWithBases).
	Py_ssize_t offset;
	// Abstract/concrete pairing, split into two one-directional, explicitly
	// named edges rather than one field whose meaning depends on which side
	// you're looking from:
	//   tp_concrete: set only on an abstract type, points to its hidden
	//     concrete companion. This is the OWNED edge -- the companion is
	//     kept alive permanently (for the JVM session) by the reference
	//     PyJPClass_concrete's tp_call never releases; this field is that
	//     same pointer, not a separate incref.
	//   tp_abstract: set only on a concrete companion, points back to the
	//     abstract type it belongs to. This is a raw, NON-owned back-edge:
	//     the pair is created and torn down as a unit, and the abstract
	//     type's own lifetime never depends on its companion, so no
	//     refcounting is needed on this direction. Consequently tp_traverse
	//     visits tp_concrete but never tp_abstract (Py_VISIT should only
	//     report edges this object actually owns a reference on).
	// Both are null for any type that isn't part of such a pair.
	PyTypeObject *tp_concrete;
	PyTypeObject *tp_abstract;
	// Every _JClass instance (i.e. every generated Java class/interface
	// wrapper type object, such as the type object for java.lang.String)
	// itself carries a JPValue for the java.lang.Class object it represents.
	// struct PyJPClass is a single, closed, compile-time-fixed layout --
	// PyJPClass_Type is never used as a base for any other spec, and every
	// wrapper is an *instance* of it, not a Python-level subclass with its
	// own extra slots -- so this can be a plain trailing field, exactly like
	// Exception/Array/Buffer/Char, with no per-family scan needed.  See the
	// PyJPClass_Type special case in PyJPClass_getOffset below.
	JPValue extra;
	// Per-family-root hook (set once on the family root type, e.g.
	// PyJPNumberLong_Type/PyJPChar_Type, and inherited by every leaf
	// wrapper class via PyJPClass_GetJValueFn's tp_base walk) that
	// reconstructs a jvalue on demand for families with no live per-instance
	// JPValue (Long/Boolean/Character). Null for families that still store
	// jvalue directly (general objects/arrays/exceptions, Float/Double).
	PyJPValueFn tp_jvalue;
	// Per-leaf-boxed-class singleton for JObject(None, cls), lazily built and
	// cached on first request (see JPBoxedType::convertToPythonObject). Null
	// for non-boxed types and for boxed classes that haven't had a null cast
	// yet. Deliberately NOT visited/cleared by tp_traverse/tp_clear: this
	// type strongly owns nullBoxed, and nullBoxed's own Py_TYPE strongly owns
	// this type right back (ordinary instance-of-type reference) -- the same
	// owned/back-edge shape as tp_concrete/tp_abstract above, and for the
	// same reason (see those field comments): it's a permanent, JVM-lifetime
	// edge that must stay invisible to the cyclic GC rather than become an
	// uncollectable 2-node cycle tp_clear can never actually break.
	PyObject *nullBoxed;
} ;

PyObject* PyJPClassMagic = nullptr;
PyObject* PyJPClassMagicConcrete = nullptr;

static inline size_t PyJPClass_alignUp(size_t value, size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

#ifdef __cplusplus
extern "C"
{
#endif

int PyJPClass_Check(PyObject* obj)
{
	return PyJP_IsInstanceSingle(obj, PyJPClass_Type);
}

Py_ssize_t PyJPClass_getOffset(PyTypeObject* type)
{
	// Special case: PyJPClass_Type itself.  Its own instances are the
	// generated Java class/interface wrapper type objects (e.g. the type
	// object for java.lang.String) -- each one directly *is* a
	// `struct PyJPClass`, whose `extra` field is a fixed, compile-time-known
	// offset, not something computed per-family.  This is what lets
	// PyJPValue_getJPClass/getJValue(obj) work uniformly whether obj is an ordinary
	// Java object instance (Py_TYPE(obj) is itself a PyJPClass instance,
	// handled below) or a class/interface wrapper object (Py_TYPE(obj) is
	// PyJPClass_Type exactly).
	if (type == (PyTypeObject*) PyJPClass_Type)
		return offsetof (struct PyJPClass, extra);
	if (type == nullptr || Py_TYPE(type) != (PyTypeObject*) PyJPClass_Type)
		return 0;
	return ((PyJPClass*) type)->offset;
}

PyTypeObject* PyJPClass_getConcrete(PyTypeObject* type)
{
	if (type == nullptr || Py_TYPE(type) != (PyTypeObject*) PyJPClass_Type)
		return nullptr;
	return ((PyJPClass*) type)->tp_concrete;
}

PyTypeObject* PyJPClass_getAbstract(PyTypeObject* type)
{
	if (type == nullptr || Py_TYPE(type) != (PyTypeObject*) PyJPClass_Type)
		return nullptr;
	return ((PyJPClass*) type)->tp_abstract;
}

JPClass* PyJPClass_GetClass(PyTypeObject* type)
{
	if (type == nullptr || Py_TYPE(type) != (PyTypeObject*) PyJPClass_Type)
		return nullptr;
	return ((PyJPClass*) type)->m_Class;
}

PyObject* PyJPClass_GetNullBoxed(PyTypeObject* type)
{
	if (type == nullptr || Py_TYPE(type) != (PyTypeObject*) PyJPClass_Type)
		return nullptr;
	return ((PyJPClass*) type)->nullBoxed;
}

void PyJPClass_SetNullBoxed(PyTypeObject* type, PyObject* obj)
{
	if (type == nullptr || Py_TYPE(type) != (PyTypeObject*) PyJPClass_Type)
		return;
	Py_XINCREF(obj);
	Py_XDECREF(((PyJPClass*) type)->nullBoxed);
	((PyJPClass*) type)->nullBoxed = obj;
}

/**
 * Create a hidden, single-purpose concrete companion type for an abstract
 * (offset == -1) type.  The companion shares the abstract type's tp_name,
 * is single-inheritance (bases = (abstract_type,)) so it can never conflict
 * with a foreign layout at creation time, and is not itself subclassable
 * (Py_TPFLAGS_BASETYPE cleared once it is built -- see the concrete branch
 * of PyJPClass_init).
 */
static int PyJPClass_concrete(PyTypeObject *abstractType)
{
	PyObject *bases = PyTuple_Pack(1, (PyObject*) abstractType);
	if (bases == nullptr)
		return -1;
	PyObject *args = Py_BuildValue("sNN", abstractType->tp_name, bases, PyDict_New());
	if (args == nullptr)
		return -1;
	PyObject *self = ((PyTypeObject*) PyJPClass_Type)->tp_call((PyObject*) PyJPClass_Type, args, PyJPClassMagicConcrete);
	Py_DECREF(args);
	if (self == nullptr)
		return -1;
	// Intentionally not decref'd: this is a permanent, JVM-lifetime
	// companion kept alive by the reference tp_call returned. That same
	// pointer is what gets stored into ((PyJPClass*)abstractType)->tp_concrete
	// (see the concreteCall branch of PyJPClass_init) -- it IS the owning
	// reference, not a separate incref.
	return 0;
}

static int PyJPClass_traverse(PyJPClass *self, visitproc visit, void *arg)
{
	// This type overrides type's tp_traverse, so it must chain to it to
	// keep visiting tp_dict/tp_bases/tp_mro/etc, or the GC undercounts
	// outgoing edges from this object while other objects (e.g. the
	// wrapper_descriptors in tp_dict) still count their incoming edge to
	// us - causing a gc refcount underflow assertion at shutdown.
	if (PyType_Type.tp_traverse((PyObject*) self, visit, arg) != 0)
		return -1; // GCOVR_EXCL_LINE
	Py_VISIT(self->m_Doc);
	// self->tp_concrete is the one genuinely OWNED edge in the abstract/
	// concrete pair (see the struct PyJPClass field comments) -- but is
	// deliberately not visited here anyway: it is a permanent, JVM-lifetime
	// reference that is intentionally never cleared (the companion's own
	// tp_bases points straight back at this type, so visiting both halves
	// of that pair would turn it into a real, GC-visible 2-node cycle that
	// tp_clear can never actually break -- a traverse/clear contract
	// violation that corrupts cycle-collector refcount accounting for the
	// whole connected component it touches, not just this pair). Treat it
	// the same as the C++-side JPClass::m_Host reference: a permanent edge
	// that is simply invisible to the cyclic GC, exactly like this design
	// already intends it to behave.
	//
	// self->tp_abstract is never visited for a different reason: it is not
	// an owned reference at all (no incref backs it -- see the struct
	// field comment), so there is no edge here for the GC to account for
	// in the first place.
	//
	// self->nullBoxed is likewise deliberately never visited/cleared, for
	// the same reason as tp_concrete -- see the struct field comment.
	return 0;
}

static int PyJPClass_clear(PyJPClass *self)
{
	PyType_Type.tp_clear((PyObject*) self);
	Py_CLEAR(self->m_Doc);
	return 0;
}

PyObject* examine(PyObject *module, PyObject *other);

PyObject* PyJPClass_FromSpecWithBases(PyType_Spec *spec, PyObject *bases, Py_ssize_t offset)
{
	JP_PY_TRY("PyJPClass_FromSpecWithBases");
#if PY_VERSION_HEX>=0x030c0000
	// Starting in Python 3.12 there is a function for creating from a meta class
	// that replaces this madeness.
	PyTypeObject *type = (PyTypeObject*) PyType_FromMetaclass((PyTypeObject*) PyJPClass_Type, NULL, spec, bases);
	if (type == nullptr)
		return (PyObject*) type;
#else
	// Python lacks a FromSpecWithMeta so we are going to have to fake it here.
	auto* type = (PyTypeObject*) PyJPClass_Type->tp_alloc(PyJPClass_Type, 0);
	auto* heap = (PyHeapTypeObject*) type;
	type->tp_flags = spec->flags | Py_TPFLAGS_HEAPTYPE;
	type->tp_name = spec->name;
	const char *s = strrchr(spec->name, '.');
	if (s == nullptr)
		s = spec->name;
	else
		s++;
	heap->ht_qualname = PyUnicode_FromString(s);
	heap->ht_name = heap->ht_qualname;
	Py_INCREF(heap->ht_name);
	if (bases == nullptr)
	{
		// do NOT use JPPyTuple_Pack here
		type->tp_bases = PyTuple_Pack(1, (PyObject*) & PyBaseObject_Type);
	}
	else
	{
		type->tp_bases = bases;
		Py_INCREF(bases);
	}
	type->tp_base = (PyTypeObject*) PyTuple_GetItem(type->tp_bases, 0);
	Py_INCREF(type->tp_base);
	type->tp_as_async = &heap->as_async;
	type->tp_as_buffer = &heap->as_buffer;
	type->tp_as_mapping = &heap->as_mapping;
	type->tp_as_number = &heap->as_number;
	type->tp_as_sequence = &heap->as_sequence;
	type->tp_basicsize = spec->basicsize;
	if (spec->basicsize == 0)
		type->tp_basicsize = type->tp_base->tp_basicsize;
	type->tp_itemsize = spec->itemsize;
	if (spec->itemsize == 0)
		type->tp_itemsize = type->tp_base->tp_itemsize;
	// Default allocator: inherit from the base, exactly like ordinary
	// CPython single-inheritance type creation would.  Every built-in
	// _jpype._J* family now bakes its Java-value slot into tp_basicsize
	// itself (concrete: a real trailing struct field; abstract: no slot of
	// its own yet), so no special allocator is needed here any more.
	type->tp_alloc = type->tp_base->tp_alloc;
	type->tp_free = PyJPValue_free;
	type->tp_finalize = (destructor) PyJPValue_finalize;
	for (PyType_Slot* slot = spec->slots; slot->slot; slot++)
	{
		switch (slot->slot)
		{
			case Py_tp_finalize:
				type->tp_finalize = (destructor) slot->pfunc;
				break;
			case Py_tp_alloc:
				type->tp_alloc = (allocfunc) slot->pfunc;
				break;
			case Py_tp_free:
				type->tp_free = (freefunc) slot->pfunc;
				break;
			case Py_tp_new:
				type->tp_new = (newfunc) slot->pfunc;
				break;
			case Py_tp_init:
				type->tp_init = (initproc) slot->pfunc;
				break;
			case Py_tp_getattro:
				type->tp_getattro = (getattrofunc) slot->pfunc;
				break;
			case Py_tp_setattro:
				type->tp_setattro = (setattrofunc) slot->pfunc;
				break;
			case Py_tp_dealloc:
				type->tp_dealloc = (destructor) slot->pfunc;
				break;
			case Py_tp_str:
				type->tp_str = (reprfunc) slot->pfunc;
				break;
			case Py_tp_repr:
				type->tp_repr = (reprfunc) slot->pfunc;
				break;
			case Py_tp_methods:
				type->tp_methods = (PyMethodDef*) slot->pfunc;
				break;
			case Py_sq_item:
				heap->as_sequence.sq_item = (ssizeargfunc) slot->pfunc;
				break;
			case Py_sq_length:
				heap->as_sequence.sq_length = (lenfunc) slot->pfunc;
				break;
			case Py_mp_ass_subscript:
				heap->as_mapping.mp_ass_subscript = (objobjargproc) slot->pfunc;
				break;
			case Py_tp_hash:
				type->tp_hash = (hashfunc) slot->pfunc;
				break;
			case Py_nb_int:
				heap->as_number.nb_int = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_float:
				heap->as_number.nb_float = (unaryfunc) slot->pfunc;
				break;
			case Py_tp_richcompare:
				type->tp_richcompare = (richcmpfunc) slot->pfunc;
				break;
			case Py_mp_subscript:
				heap->as_mapping.mp_subscript = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_index:
				heap->as_number.nb_index = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_absolute:
				heap->as_number.nb_absolute = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_and:
				heap->as_number.nb_and = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_or:
				heap->as_number.nb_or = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_xor:
				heap->as_number.nb_xor = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_add:
				heap->as_number.nb_add = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_subtract:
				heap->as_number.nb_subtract = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_multiply:
				heap->as_number.nb_multiply = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_rshift:
				heap->as_number.nb_rshift = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_lshift:
				heap->as_number.nb_lshift = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_negative:
				heap->as_number.nb_negative = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_bool:
				heap->as_number.nb_bool = (inquiry) slot->pfunc;
				break;
			case Py_nb_invert:
				heap->as_number.nb_invert = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_positive:
				heap->as_number.nb_positive = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_floor_divide:
				heap->as_number.nb_floor_divide = (binaryfunc) slot->pfunc;
				break;
			case Py_nb_divmod:
				heap->as_number.nb_divmod = (binaryfunc) slot->pfunc;
				break;
			case Py_tp_getset:
				type->tp_getset = (PyGetSetDef*) slot->pfunc;
				break;
#if PY_VERSION_HEX >= 0x03090000
			case Py_bf_getbuffer:
				type->tp_as_buffer->bf_getbuffer = (getbufferproc) slot->pfunc;
				break;
			case Py_bf_releasebuffer:
				type->tp_as_buffer->bf_releasebuffer = (releasebufferproc) slot->pfunc;
				break;
#endif
				// GCOVR_EXCL_START
			default:
				PyErr_Format(PyExc_TypeError, "slot %d not implemented", slot->slot);
				JP_RAISE_PYTHON();
				// GCOVR_EXCL_STOP
		}
	}

	// GC objects are required to implement clear and traverse, this is a
	// safety check to make sure we implemented all properly.	This error should
	// never happen in production code.
	if (PyType_IS_GC(type) && (
			type->tp_traverse==nullptr ||
			type->tp_clear==nullptr))
	{
		PyErr_Format(PyExc_TypeError, "GC requirements failed for %s", spec->name);
		JP_RAISE_PYTHON();
	}

#endif

	// Every built-in family now has its Java-value slot's location either
	// baked into tp_basicsize (concrete) or deliberately absent (abstract,
	// awaiting a concrete companion) -- so the inherited allocator set
	// above is always correct and there is no legacy (offset == 0) case
	// left to force onto the thread-local dummy-heap-type allocator.
	if (offset == 0)
	{
		PyErr_Format(PyExc_TypeError,
				"internal error: '%s' registered with offset 0 (legacy "
				"allocator no longer exists)", spec->name);
		Py_DECREF(type);
		JP_RAISE_PYTHON();
	}
	type->tp_finalize = (destructor) PyJPValue_finalize;
	((PyJPClass*) type)->offset = offset;
	((PyJPClass*) type)->tp_concrete = nullptr;
	((PyJPClass*) type)->tp_abstract = nullptr;
	((PyJPClass*) type)->tp_jvalue = nullptr;
	((PyJPClass*) type)->nullBoxed = nullptr;

	PyType_Ready(type);
	PyDict_SetItemString(type->tp_dict, "__module__", PyUnicode_FromString("_jpype"));

	if (offset == -1 && PyJPClass_concrete(type) == -1)
	{
		Py_DECREF(type);
		return nullptr;
	}

	return (PyObject*) type;
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

int PyJPClass_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClass_init");

	if (!PyObject_IsInstance(self, (PyObject*) PyJPClass_Type))
	{
		PyErr_SetString(PyExc_TypeError, "Type incorrect");
		return -1;
	}

	PyTypeObject *type = (PyTypeObject*) self;

#if PY_VERSION_HEX >= 0x030d0000
	// Python 3.13 - This flag will try to place the dictionary are part of the object which 
	// adds an unknown number of bytes to the end of the object making it impossible
	// to attach our needed data.  If we kill the flag then we get usable behavior.
	type->tp_flags &= ~Py_TPFLAGS_INLINE_VALUES;
#endif

	// Verify that we were called internally
	int magic = 0;
	bool concreteCall = false;
	if (kwargs == PyJPClassMagicConcrete)
	{
		magic = 1;
		concreteCall = true;
		kwargs = nullptr;
	} else if (kwargs == PyJPClassMagic || (kwargs != nullptr && PyDict_GetItemString(kwargs, "internal") != nullptr))
	{
		magic = 1;
		kwargs = nullptr;
	}
	if (magic == 0)
	{
		PyErr_Format(PyExc_TypeError, "Java classes cannot be extended in Python");
		return -1;
	}

	// Set the host object
	PyObject *name = nullptr;
	PyObject *bases = nullptr;
	PyObject *members = nullptr;
	if (!PyArg_ParseTuple(args, "OOO", &name, &bases, &members))
		return -1;

	//	 Check that all types are Java types
	if (!PyTuple_Check(bases))
	{
		PyErr_SetString(PyExc_TypeError, "Bases must be a tuple");
		return -1;
	}

	JP_BLOCK("PyJPClass_new::verify")
	{
		// Watch for final classes
		Py_ssize_t len = PyTuple_Size(bases);
		for (Py_ssize_t i = 0; i < len; ++i)
		{
			PyObject *item = PyTuple_GetItem(bases, i);
			JPClass *cls = PyJPClass_getJPClass(item);
			if (cls != nullptr && cls->isFinal())
			{
				PyErr_Format(PyExc_TypeError, "Cannot extend final class '%s'",
						((PyTypeObject*) item)->tp_name);
			}
		}
	}

	// Resolve this type's Java-value slot family by scanning all bases.
	// Priority is legacy > concrete > abstract:
	//  - any legacy base forces the whole type back to the fully-legacy
	//    path, no matter what else is mixed in.  This is what keeps boxed
	//    Number types (which implement Comparable/Serializable -- abstract,
	//    migrated bases) safe: they stay on the untouched legacy allocator
	//    rather than risk a CPython "instance lay-out conflict" from mixing
	//    a migrated family's layout with PyLong/PyFloat ancestry.
	//  - failing that, a concrete base's offset is inherited directly (no
	//    further work needed -- CPython's own type_new already grew
	//    tp_basicsize correctly via ordinary single/solid-base inheritance).
	//  - failing that, if any base is abstract, this type is abstract too
	//    and needs its own concrete companion before it can be instantiated.
	bool anyLegacy = false;
	Py_ssize_t concreteOffset = 0;
	bool anyAbstract = false;
	{
		Py_ssize_t n = PyTuple_Size(bases);
		for (Py_ssize_t i = 0; i < n; ++i)
		{
			PyObject *b = PyTuple_GetItem(bases, i);
			if (Py_TYPE(b) != (PyTypeObject*) PyJPClass_Type)
				continue;
			// An already-linked abstract-kind base has tp_concrete set (see
			// the struct PyJPClass field comments) -- check that FIRST and
			// unconditionally treat it as abstract for inheritance-safety
			// classification, regardless of what its own offset field says.
			// offset is now always the real, resolved byte offset even on
			// abstract types (flattened onto both halves of the pair when
			// the companion is built -- see the concreteCall branch below),
			// so once linked it can no longer be used as the abstract/
			// concrete signal here: that field answers "where is my data",
			// not "am I safe to physically extend tp_basicsize with" --
			// interfaces must stay classified as abstract forever so they
			// remain safe to mix into arbitrary multiple-inheritance
			// chains, even though their offset is a real, usable number.
			if (((PyJPClass*) b)->tp_concrete != nullptr)
			{
				anyAbstract = true;
				continue;
			}
			Py_ssize_t bo = ((PyJPClass*) b)->offset;
			if (bo == 0)
				anyLegacy = true;
			else if (bo > 0)
			{
				if (concreteOffset == 0)
					concreteOffset = bo;
			}
			else
			{
				// bo == -1: an abstract base that is mid-construction of
				// its own companion RIGHT NOW (this scan is running as
				// part of that very concreteCall -- see PyJPClass_concrete)
				// and hasn't been flattened/linked yet. Still abstract.
				anyAbstract = true;
			}
		}
	}

	// anyLegacy can no longer trip (no family registers offset == 0 any
	// more -- see PyJPClass_FromSpecWithBases), and the "no Java-value-
	// bearing base at all" case is unreachable in practice: PyJPClass_
	// getBases always supplies either an interface/superclass base (which
	// itself resolves recursively) or, when super == nullptr, the abstract
	// PyJPObject_Type explicitly -- so every wrapper's bases tuple contains
	// at least one non-zero-offset PyJPClass_Type instance, bottoming out
	// at Object. Both are asserted here rather than silently forced onto a
	// (now-deleted) legacy allocator, so that if this reasoning is ever
	// wrong for some case we haven't enumerated, it fails loudly instead of
	// corrupting memory.
	if (anyLegacy || (!(concreteOffset > 0) && !anyAbstract))
	{
		PyErr_Format(PyExc_TypeError,
				"internal error: '%s' has no Java-value-bearing base "
				"(legacy allocator no longer exists)",
				((PyHeapTypeObject*) type)->ht_name ?
				PyUnicode_AsUTF8(((PyHeapTypeObject*) type)->ht_name) : "?");
		return -1;
	}
	Py_ssize_t resolvedOffset = (concreteOffset > 0) ? concreteOffset : -1;

	type->tp_finalize = (destructor) PyJPValue_finalize;
	((PyJPClass*) self)->m_Doc = nullptr;
	((PyJPClass*) self)->offset = resolvedOffset;
	((PyJPClass*) self)->tp_concrete = nullptr;
	((PyJPClass*) self)->tp_abstract = nullptr;
	((PyJPClass*) self)->tp_jvalue = nullptr;
	((PyJPClass*) self)->nullBoxed = nullptr;

	// Call the type init
	int rc = PyType_Type.tp_init(self, args, nullptr);
	if (rc == -1)
		return rc; // GCOVR_EXCL_LINE no clue how to trigger this one

	// GCOVR_EXCL_START
	// Sanity checks.  Not testable
	if (type == nullptr)
		return -1;
	if (type->tp_finalize != nullptr && type->tp_finalize != (destructor) PyJPValue_finalize)
	{
		PyErr_SetString(PyExc_TypeError, "finalizer conflict");
		return -1;
	}

	// This sanity check is trigger if the user attempts to build their own
	// type wrapper with a __del__ method defined.	It is hard to trigger.
	// No family forces its own allocator any more (see PyJPClass_init above),
	// so the only correct value is whatever ordinary CPython inheritance
	// already gave this type from its base.
	if (type->tp_alloc != type->tp_base->tp_alloc)
	{
		PyErr_SetString(PyExc_TypeError, "alloc conflict");
		return -1;
	}
	// GCOVR_EXCL_STOP

#if PY_VERSION_HEX < 0x03090000
	// This was required at one point but I don't know what version it applied to.
	// PyJPException_Type is null while it is itself under construction (see
	// PyJPObject_initType), which this type-init path runs through too -- guard
	// against that self-referential bootstrap case rather than dereferencing null.
	if (PyJPException_Type != nullptr &&
			PyObject_IsSubclass((PyObject*) type, (PyObject*) PyJPException_Type))
	{
		type->tp_new = PyJPException_Type->tp_new;
	}
#endif

	if (concreteCall)
	{
		// This call came from PyJPClass_concrete: bases is exactly
		// (abstractType,).  Bake in the real fixed offset for it now and
		// pair the two types together; no further Python-level subclassing
		// of this hidden companion is allowed.
		PyObject *abstractBase = PyTuple_GetItem(bases, 0);
		// Bare jvalue, not a full JPValue -- class is always derived from the
		// wrapper type (PyJPClass_GetClass/PyJPValue_getJPClass), never
		// stored per-instance.
		size_t off = PyJPClass_alignUp((size_t) type->tp_basicsize, alignof (jvalue));
		type->tp_basicsize = (Py_ssize_t) (off + sizeof (jvalue));
		((PyJPClass*) self)->offset = (Py_ssize_t) off;
		// Flatten the SAME real offset onto the abstract half too, so
		// resolving a live instance's jvalue never needs to branch or
		// chase a companion again -- offset becomes a hard invariant on
		// both halves of the pair, not just the concrete one. See the
		// struct PyJPClass field comments for why tp_concrete is the owned
		// edge and tp_abstract is a raw, non-owned back-edge.
		((PyJPClass*) abstractBase)->offset = (Py_ssize_t) off;
		((PyJPClass*) abstractBase)->tp_concrete = type;
		((PyJPClass*) self)->tp_abstract = (PyTypeObject*) abstractBase;
		type->tp_flags &= ~Py_TPFLAGS_BASETYPE;
	} else if (resolvedOffset == -1)
	{
		// An ordinary type that resolved to abstract: give it its own
		// hidden concrete companion so it can actually be instantiated.
		if (PyJPClass_concrete(type) == -1)
			return -1;
	}

	return rc;
	JP_PY_CATCH(-1);
}

static void PyJPClass_dealloc(PyJPClass *self)
{
	JP_PY_TRY("PyJPClass_dealloc");
	PyObject_GC_UnTrack(self);
	PyJPClass_clear(self);
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH_NONE(); // GCOVR_EXCL_LINE
}

PyObject* PyJPClass_mro(PyTypeObject *self)
{
	Py_ssize_t sz = PySequence_Size(self->tp_bases);
	std::list<PyObject*> bases;
	bases.push_back((PyObject*) self);

	// Merge together all bases
	std::list<PyObject*> out;
	for (auto iter = bases.begin();
			iter != bases.end(); ++iter)
	{
		PyObject *l = ((PyTypeObject*) * iter)->tp_bases;
		sz = PySequence_Size(l);
		for (Py_ssize_t i = 0; i < sz; ++i)
		{
			PyObject *obj = PyTuple_GetItem(l, i);
			bool found = (std::find(bases.begin(), bases.end(), obj) != bases.end());
			if (!found)
			{
				bases.push_back(obj);
			}
		}
	}

	while (!bases.empty())
	{
		PyObject* front = bases.front();
		bases.pop_front();
		for (auto iter = bases.begin();
				iter != bases.end(); ++iter)
		{
			if (PySequence_Contains(((PyTypeObject*) * iter)->tp_bases, front))
			{
				bases.push_back(front);
				front = nullptr;
				break;
			}
		}
		if (front != nullptr)
		{
			out.push_back(front);
			auto* next = (PyObject*) ((PyTypeObject*) front)->tp_base;
			if (next)
			{
				bases.remove(next);
				bases.push_front(next);
			}
		}
	}

	PyObject *obj = PyTuple_New(out.size());
	int j = 0;
	for (auto iter = out.begin();
			iter != out.end(); ++iter)
	{
		Py_INCREF(*iter);
		PyTuple_SetItem(obj, j++, *iter);
	}
	return obj;
}

PyObject *PyJPClass_getattro(PyObject *obj, PyObject *name)
{
	JP_PY_TRY("PyJPClass_getattro");
	if (!PyUnicode_Check(name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				Py_TYPE(name)->tp_name);
		return nullptr;
	}

	// Private members are accessed directly
	PyObject* pyattr = PyType_Type.tp_getattro(obj, name);
	if (pyattr == nullptr)
		return nullptr;
	JPPyObject attr = JPPyObject::claim(pyattr);

	// Private members go regardless
	if (PyUnicode_GetLength(name) && PyUnicode_ReadChar(name, 0) == '_')
		return attr.keep();

	// Methods
	if (Py_TYPE(attr.get()) == PyJPMethod_Type)
		return attr.keep();

	// Don't allow properties to be rewritten
	if (!PyObject_IsInstance(attr.get(), (PyObject*) & PyProperty_Type))
		return attr.keep();

	const char *name_str = PyUnicode_AsUTF8(name);
	PyErr_Format(PyExc_AttributeError, "Field '%s' is static", name_str);
	return nullptr;
	JP_PY_CATCH(nullptr);
}

int PyJPClass_setattro(PyObject *self, PyObject *attr_name, PyObject *v)
{
	JP_PY_TRY("PyJPClass_setattro");
	PyJPModule_getContext();
	if (!PyUnicode_Check(attr_name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				attr_name->ob_type->tp_name);
		return -1;
	}

	// Private members are accessed directly
	if (PyUnicode_GetLength(attr_name) && PyUnicode_ReadChar(attr_name, 0) == '_')
		return PyType_Type.tp_setattro(self, attr_name, v);

	JPPyObject f = JPPyObject::accept(PyJP_GetAttrDescriptor((PyTypeObject*) self, attr_name));
	if (f.isNull())
	{
		const char *name_str = PyUnicode_AsUTF8(attr_name);
		PyErr_Format(PyExc_AttributeError, "Field '%s' is not found", name_str);
		return -1;
	}

	descrsetfunc desc = Py_TYPE(f.get())->tp_descr_set;
	if (desc != nullptr)
		return desc(f.get(), self, v);

	// Not a descriptor
	const char *name_str = PyUnicode_AsUTF8(attr_name);
	PyErr_Format(PyExc_AttributeError,
			"Static field '%s' is not settable on Java '%s' object",
			name_str, ((PyTypeObject*) self)->tp_name);
	return -1;
	JP_PY_CATCH(-1);
}

PyObject* PyJPClass_subclasscheck(PyTypeObject *type, PyTypeObject *test)
{
	JP_PY_TRY("PyJPClass_subclasscheck");
	if (test == type)
		Py_RETURN_TRUE;

	// GCOVR_EXCL_START
	// This is triggered only if the user asks for isInstance when the
	// JVM is shutdown. It should not happen in normal operations.
	if (!JPContext_global->isRunning())
	{
		if ((PyObject*) type == _JObject)
			return PyBool_FromLong(PyJP_IsSubClassSingle(PyJPObject_Type, test));
		return PyBool_FromLong(PyJP_IsSubClassSingle(type, test));
	}
	// GCOVR_EXCL_STOP

	JPJavaFrame frame = JPJavaFrame::outer();

	// Check for class inheritance first
	JPClass *testClass = PyJPClass_getJPClass((PyObject*) test);
	JPClass *typeClass = PyJPClass_getJPClass((PyObject*) type);
	if (testClass == nullptr)
		Py_RETURN_FALSE;
	if (testClass == typeClass)
		Py_RETURN_TRUE;
	if (typeClass != nullptr)
	{
		if (typeClass->isPrimitive())
			Py_RETURN_FALSE;
		bool b = frame.IsAssignableFrom(testClass->getJavaClass(), typeClass->getJavaClass()) != 0;
		return PyBool_FromLong(b);
	}

	// Otherwise check for special cases
	if ((PyObject*) type == _JInterface)
		return PyBool_FromLong(testClass->isInterface());
	if ((PyObject*) type == _JObject)
		return PyBool_FromLong(!testClass->isPrimitive());
	if ((PyObject*) type == _JArray)
		return PyBool_FromLong(testClass->isArray());
	if ((PyObject*) type == _JException)
		return PyBool_FromLong(testClass->isThrowable());

	PyObject* mro1 = test->tp_mro;
	Py_ssize_t n1 = PyTuple_Size(mro1);
	for (int i = 0; i < n1; ++i)
	{
		if (PyTuple_GetItem(mro1, i) == (PyObject*) type)
			Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPClass_class(PyObject *self, PyObject *closure)
{
	JP_PY_TRY("PyJPClass_class");
	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass* cls = PyJPValue_getJPClass(self);
	if (cls == nullptr)
	{
		PyErr_SetString(PyExc_AttributeError, "Java slot is null");
		return nullptr;
	}
	return cls->convertToPythonObject(frame, PyJPValue_getJValue(frame, self), false).keep();
	JP_PY_CATCH(nullptr);
}

static int PyJPClass_setClass(PyObject *self, PyObject *type, PyObject *closure)
{
	JP_PY_TRY("PyJPClass_setClass", self);
	JPJavaFrame frame = JPJavaFrame::outer();
	JPContext *context = frame.getContext();
	JPClass* typeCls = PyJPValue_getJPClass(type);
	if (typeCls == nullptr || typeCls != context->_java_lang_Class)
	{
		PyErr_SetString(PyExc_TypeError, "Java class instance is required");
		return -1;
	}
	if (PyJPValue_isSetJavaSlot(self))
	{
		PyErr_SetString(PyExc_AttributeError, "Java class can't be set");
		return -1;
	}
	jvalue typeVal = PyJPValue_getJValue(frame, type);
	PyJPValue_assignJavaSlot(frame, self, JPValue(typeCls, typeVal));

	JPClass* cls = frame.findClass((jclass) typeVal.l);
	JP_TRACE("Set host", cls, typeCls->getCanonicalName().c_str());
	if (cls->getHost() == nullptr)
		cls->setHost(self);
	((PyJPClass*) self)->m_Class = cls;
	return 0;
	JP_PY_CATCH(-1);
}

static PyObject *PyJPClass_hints(PyJPClass *self, PyObject *closure)
{
	JP_PY_TRY("PyJPClass_hints");
	PyJPModule_getContext();
	JPPyObject hints = JPPyObject::use(self->m_Class->getHints());
	if (hints.get() == nullptr)
		Py_RETURN_NONE; // GCOVR_EXCL_LINE only triggered if JClassPost failed

	if (PyObject_HasAttrString((PyObject*) self, "returns") == 1)
		return hints.keep();

	// Copy in info.
	JPConversionInfo info;
	JPPyObject ret = JPPyObject::call(PyList_New(0));
	JPPyObject implicit = JPPyObject::call(PyList_New(0));
	JPPyObject attribs = JPPyObject::call(PyList_New(0));
	JPPyObject exact = JPPyObject::call(PyList_New(0));
	JPPyObject expl = JPPyObject::call(PyList_New(0));
	JPPyObject none = JPPyObject::call(PyList_New(0));
	info.ret = ret.get();
	info.implicit = implicit.get();
	info.attributes = attribs.get();
	info.exact = exact.get();
	info.expl = expl.get();
	info.none = none.get();
	self->m_Class->getConversionInfo(info);
	PyObject_SetAttrString(hints.get(), "returns", ret.get());
	PyObject_SetAttrString(hints.get(), "implicit", implicit.get());
	PyObject_SetAttrString(hints.get(), "exact", exact.get());
	PyObject_SetAttrString(hints.get(), "explicit", expl.get());
	PyObject_SetAttrString(hints.get(), "none", none.get());
	PyObject_SetAttrString(hints.get(), "attributes", attribs.get());
	return hints.keep();
	JP_PY_CATCH(nullptr);
}

static int PyJPClass_setHints(PyObject *self, PyObject *value, PyObject *closure)
{
	JP_PY_TRY("PyJPClass_setHints", self);
	PyJPModule_getContext();
	auto *cls = (PyJPClass*) self;
	PyObject *hints = cls->m_Class->getHints();
	if (hints != nullptr)
	{
		PyErr_SetString(PyExc_AttributeError, "_hints can't be set");
		return -1;
	}
	cls->m_Class->setHints(value);
	return 0;
	JP_PY_CATCH(-1);
}

PyObject* PyJPClass_instancecheck(PyTypeObject *self, PyObject *test)
{
	// Issue #1329: Check if JVM is running before creating JPJavaFrame
	// This prevents crashes when isinstance() is called with JException
	// after a failed JVM initialization
	if (!JPContext_global->isRunning())
	{
		// Fall back to Python-only type checking when JVM is not running
		return PyJPClass_subclasscheck(self, Py_TYPE(test));
	}

	// JInterface is a meta
	if ((PyObject*) self == _JInterface)
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		JPClass *testClass = PyJPClass_getJPClass((PyObject*) test);
		return PyBool_FromLong(testClass != nullptr && testClass->isInterface());
	}
	if ((PyObject*) self == _JException)
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		JPClass *testClass = PyJPClass_getJPClass((PyObject*) test);
		if (testClass)
			return PyBool_FromLong(testClass->isThrowable());
	}
	return PyJPClass_subclasscheck(self, Py_TYPE(test));
}

static PyObject *PyJPClass_canCast(PyJPClass *self, PyObject *other)
{
	JP_PY_TRY("PyJPClass_canCast");
	JPJavaFrame frame = JPJavaFrame::outer();

	JPClass *cls = self->m_Class;

	// Test the conversion
	JPMatch match(&frame, other);
	cls->findJavaConversion(match);

	// Report to user
	return PyBool_FromLong(match.type == JPMatch::_exact || match.type == JPMatch::_implicit);
	JP_PY_CATCH(nullptr);
}
// Added for auditing

static PyObject *PyJPClass_canConvertToJava(PyJPClass *self, PyObject *other)
{
	JP_PY_TRY("PyJPClass_canConvertToJava");
	JPJavaFrame frame = JPJavaFrame::outer();

	JPClass *cls = self->m_Class;

	// Test the conversion
	JPMatch match(&frame, other);
	cls->findJavaConversion(match);

	// Report to user
	if (match.type == JPMatch::_none)
		return JPPyString::fromStringUTF8("none").keep();
	if (match.type == JPMatch::_explicit)
		return JPPyString::fromStringUTF8("explicit").keep();
	if (match.type == JPMatch::_implicit)
		return JPPyString::fromStringUTF8("implicit").keep();
	if (match.type == JPMatch::_derived)
		return JPPyString::fromStringUTF8("derived").keep();
	if (match.type == JPMatch::_exact)
		return JPPyString::fromStringUTF8("exact").keep();

	// Not sure how this could happen
	Py_RETURN_NONE; // GCOVR_EXCL_LINE
	JP_PY_CATCH(nullptr);
}

// Return true if the slice is all indices

static bool PySlice_CheckFull(PyObject *item)
{
	if (!PySlice_Check(item))
		return false;
	Py_ssize_t start, stop, step;
	int rc = PySlice_Unpack(item, &start, &stop, &step);
	return (rc == 0)&&(start == 0)&&(step == 1)&&(stop == PY_SSIZE_T_MAX);
}

static PyObject *handleGeneric(JPJavaFrame &frame, PyJPClass *self, PyObject *item)
{
	// TODO: Re-enable this check once GENERIC modifier is working
	// Currently the flag isn't being set properly for generic types
	// if (!self->m_Class->isGeneric())
	// {
	// 	PyErr_Format(PyExc_TypeError, "Type is not generic");
	// 	return 0;
	// }

	Py_ssize_t dims = PyTuple_Size(item);
	Py_ssize_t i = 0;

	std::stringstream name;
	for (; i < dims; ++i)
	{
		PyObject* t = PyTuple_GetItem(item, i);
		if (!PyJPClass_Check(t))
			break;

		if (i > 0)
			name << ',';
		name << ((PyTypeObject*) t)->tp_name;
	}

	if (i != dims)
	{
		PyErr_Format(PyExc_TypeError, "Generic parameters must be Java");
		return 0;
	}

	// Return a generic type wrapper.
	JPPyObject key = JPPyObject::call(PyUnicode_FromFormat("%s<%s>",
			((PyTypeObject*) self)->tp_name,
			name.str().c_str()));
	PyObject *cache = PyDict_GetItem(PyJClass_Generics, key.get());
	if (cache != nullptr)
	{
		Py_INCREF(cache);
		return cache;
	}

	// Allocate a new type
	JPPyObject bases = JPPyObject::call(PyTuple_Pack(1, self));
	JPPyObject members = JPPyObject::call(PyDict_New());
	JPPyObject args = JPPyObject::call(PyTuple_Pack(3, key.get(), bases.get(), members.get()));
	JPPyObject generic = JPPyObject::call(PyObject_Call((PyObject*) PyJPClass_Type, args.get(), PyJPClassMagic));
	PyObject_SetAttrString(generic.get(), "_generics", item);
	PyDict_SetItem(PyJClass_Generics, key.get(), generic.get());
	PyJPClass *cls2 = (PyJPClass*) generic.get();
	cls2->m_Class = self->m_Class;
	PyJPValue_assignJavaSlot(frame, (PyObject*) cls2, JPValue(frame.getContext()->_java_lang_Class,
			(jobject) self->m_Class->getJavaClass()));
	return generic.keep();
}

static PyObject *PyJPClass_typed(PyJPClass *self, PyObject *args)
{
	JP_PY_TRY("PyJPClass_typed");
	JPJavaFrame frame = JPJavaFrame::outer();

	// Accept either a single type or multiple types
	if (PyTuple_Check(args) && PyTuple_Size(args) > 0)
	{
		return handleGeneric(frame, self, args);
	}

	PyErr_Format(PyExc_TypeError, "typed() requires at least one type argument");
	return nullptr;
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPClass_array(PyJPClass *self, PyObject *item)
{
	JP_PY_TRY("PyJPClass_array");
	JPJavaFrame frame = JPJavaFrame::outer();
	JPContext *context = frame.getContext();

	if (self->m_Class == NULL)
	{
		// This is only reachable through an internal Jpype type that doesn't have a
		// Java class equivalent such as JArray.
		// __getitem__ takes precedence over __class_getitem__ which forces us through
		// this path.
		// If __class_getitem__ is not implemented in this class, the raised AttributeError
		// will make its way back to Python.
		PyObject *res = PyObject_CallMethod((PyObject *)self, "__class_getitem__", "O", item);
		Py_DECREF(item);
		return res;
	}

	if (PyIndex_Check(item))
	{
		long sz = PyLong_AsLong(item);
		auto *cls = dynamic_cast<JPArrayClass*>( self->m_Class->newArrayType(frame, 1));
		JPValue v = cls->newArray(frame, sz);
		return cls->convertToPythonObject(frame, v.getValue(), true).keep();
	}

	if (PySlice_Check(item))
	{
		if (PySlice_CheckFull(item))
		{
			JPClass *cls = self->m_Class->newArrayType(frame, 1);
			return PyJPClass_create(frame, cls).keep();
		}
		PyErr_Format(PyExc_TypeError, "Bad array specification on slice");
		return nullptr;
	}

	if (PyTuple_Check(item))
	{
		Py_ssize_t dims = PyTuple_Size(item);
		Py_ssize_t i = 0;
		Py_ssize_t defined = 0;
		Py_ssize_t undefined = 0;


		std::vector<int> sz;
		for (; i < dims; ++i)
		{
			PyObject* t = PyTuple_GetItem(item, i);
			if (PyIndex_Check(t) && PyLong_AsLong(t) > 0)
			{
				defined++;
				sz.push_back(PyLong_AsLong(t));
			} else
				break;
		}
		for (; i < dims; ++i)
			if (PySlice_CheckFull(PyTuple_GetItem(item, i)))
				undefined++;
			else
				break;
		if (defined + undefined != dims)
		{
			PyErr_SetString(PyExc_TypeError, "Invalid array definition");
			return nullptr;
		}

		// Get the type
		JPClass *cls;
		if (undefined > 0)
			cls = self->m_Class->newArrayType(frame, undefined);
		else
			cls = self->m_Class;

		// If no dimensions were defined then just return the type
		if (defined == 0)
			return PyJPClass_create(frame, cls).keep();

		// Otherwise create an array
		jintArray u = frame.NewIntArray(defined);
		JPPrimitiveArrayAccessor<jintArray, jint*> accessor(frame, u,
				&JPJavaFrame::GetIntArrayElements, &JPJavaFrame::ReleaseIntArrayElements);
		for (size_t j = 0; j < sz.size(); ++j)
			accessor.get()[j] = sz[j];
		accessor.commit();

		jvalue v;
		v.l = frame.newArrayInstance(cls->getJavaClass(), u);
		return context->_java_lang_Object->convertToPythonObject(frame, v, false).keep();
	}

	PyErr_Format(PyExc_TypeError, "Bad array specification");
	JP_PY_CATCH(nullptr);
}

static PyObject * PyJPClass_cast(PyJPClass *self, PyObject * other)
{
	JP_PY_TRY("PyJPClass_cast");
	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass *type = self->m_Class;
	JPClass *valCls = PyJPValue_getJPClass(other);

	// Cast on non-Java
	if (valCls == nullptr || valCls->isPrimitive())
	{
		JPMatch match(&frame, other);
		type->findJavaConversion(match);
		// Otherwise, see if we can convert it
		if (match.type == JPMatch::_none)
		{
			PyErr_Format(PyExc_TypeError,
					"Unable to cast '%s' to java type '%s'",
					Py_TYPE(other)->tp_name,
					type->getCanonicalName().c_str()
					);
			return nullptr;
		}
		jvalue v = match.convert();
		return type->convertToPythonObject(frame, v, true).keep();
	}

	// Cast on java object
	//	if (!type->isSubTypeOf(valCls))
	jvalue val = PyJPValue_getJValue(frame, other);
	jobject obj = val.l;
	if (obj == nullptr)
	{
		jvalue v;
		v.l = nullptr;
		return type->convertToPythonObject(frame, v, true).keep();
	}
	JPClass *otherClass = frame.findClassForObject(obj);
	if (otherClass == nullptr)
	{
		return type->convertToPythonObject(frame, val, true).keep();
	}

	if (!otherClass->isAssignableFrom(frame, type))
	{
		PyErr_Format(PyExc_TypeError,
				"Unable to cast '%s' to java type '%s'",
				otherClass->getCanonicalName().c_str(),
				type->getCanonicalName().c_str()
				);
		return nullptr;
	}

	// Special case.  If the otherClass is an array and the array is
	// a slice then we need to copy it here.
	if (PyObject_IsInstance(other, (PyObject*) PyJPArray_Type))
	{
		auto *array = (PyJPArray*) other;
		if (array->m_Array->isSlice())
		{
			jvalue v;
			v.l = array->m_Array->clone(frame, other);
			return type->convertToPythonObject(frame, v, true).keep();
		}
	}

	return type->convertToPythonObject(frame, val, true).keep();

	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static PyObject * PyJPClass_castEq(PyJPClass *self, PyObject * other)
{
	PyErr_Format(PyExc_TypeError, "Invalid operation");
	return nullptr;
}

// Added for auditing

static PyObject * PyJPClass_convertToJava(PyJPClass *self, PyObject * other)
{
	JP_PY_TRY("PyJPClass_convertToJava");
	JPJavaFrame frame = JPJavaFrame::outer();

	JPClass *cls = self->m_Class;

	// Test the conversion
	JPMatch match(&frame, other);
	cls->findJavaConversion(match);

	// If there is no conversion report a failure
	if (match.type == JPMatch::_none)
	{
		PyErr_SetString(PyExc_TypeError, "Unable to create an instance.");
		return nullptr;
	}

	// Otherwise give back a PyJPValue
	jvalue v = match.convert();
	return cls->convertToPythonObject(frame, v, true).keep();
	JP_PY_CATCH(nullptr);
}

static PyObject * PyJPClass_repr(PyJPClass * self)
{
	JP_PY_TRY("PyJPClass_repr");
	string name = ((PyTypeObject*) self)->tp_name;
	return PyUnicode_FromFormat("<java class '%s'>", name.c_str());
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static PyObject * PyJPClass_getDoc(PyJPClass *self, void *ctxt)
{
	JP_PY_TRY("PyJPMethod_getDoc");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (self->m_Doc)
	{
		Py_INCREF(self->m_Doc);
		return self->m_Doc;
	}

	// Pack the arguments
	{
		JP_TRACE("Pack arguments");
		JPPyObject args = JPPyTuple_Pack(self);
		JP_TRACE("Call Python");
		self->m_Doc = PyObject_Call(_JClassDoc, args.get(), nullptr);
		Py_XINCREF(self->m_Doc);
		return self->m_Doc;
	}
	JP_PY_CATCH(nullptr);
}

int PyJPClass_setDoc(PyJPClass *self, PyObject *obj, void *ctxt)
{
	JP_PY_TRY("PyJPClass_setDoc");
	Py_CLEAR(self->m_Doc);
	self->m_Doc = obj;
	Py_XINCREF(self->m_Doc);
	return 0;
	JP_PY_CATCH(-1);
}

PyObject * PyJPClass_customize(PyJPClass *self, PyObject *args, PyObject * kwargs)
{
	JP_PY_TRY("PyJPClass_customize");
	PyObject *name = nullptr;
	PyObject *value = nullptr;
	if (!PyArg_ParseTuple(args, "OO", &name, &value))
		return nullptr;
	if (PyType_Type.tp_setattro((PyObject*) self, name, value) == -1)
		return nullptr;
	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static PyMethodDef classMethods[] = {
	{"__instancecheck__", (PyCFunction) PyJPClass_instancecheck, METH_O, ""},
	{"__subclasscheck__", (PyCFunction) PyJPClass_subclasscheck, METH_O, ""},
	{"mro", (PyCFunction) PyJPClass_mro, METH_NOARGS, ""},
	{"_canConvertToJava", (PyCFunction) PyJPClass_canConvertToJava, METH_O, ""},
	{"_convertToJava", (PyCFunction) PyJPClass_convertToJava, METH_O, ""},
	{"_cast", (PyCFunction) PyJPClass_cast, METH_O, ""},
	{"_canCast", (PyCFunction) PyJPClass_canCast, METH_O, ""},
	{"__getitem__", (PyCFunction) PyJPClass_array, METH_O | METH_COEXIST, ""},
	{"typed", (PyCFunction) PyJPClass_typed, METH_VARARGS, "Create a generic type with the specified type parameters"},
	{"_customize", (PyCFunction) PyJPClass_customize, METH_VARARGS, ""},
	{nullptr},
};

static PyGetSetDef classGetSets[] = {
	{"class_", (getter) PyJPClass_class, (setter) PyJPClass_setClass, ""},
	{"_hints", (getter) PyJPClass_hints, (setter) PyJPClass_setHints, ""},
	{"__doc__", (getter) PyJPClass_getDoc, (setter) PyJPClass_setDoc, nullptr, nullptr},
	{nullptr}
};

static PyType_Slot classSlots[] = {
	// No Py_tp_alloc override: struct PyJPClass's own JPValue ("extra") is
	// now a compile-time-fixed trailing field (see PyJPClass_getOffset's
	// PyJPClass_Type special case), so classSpec.basicsize already accounts
	// for it and the ordinary inherited (PyType_Type) allocator is correct.
	{ Py_tp_finalize, (void*) PyJPValue_finalize},
	{ Py_tp_init, (void*) PyJPClass_init},
	{ Py_tp_dealloc, (void*) PyJPClass_dealloc},
	{ Py_tp_traverse, (void*) PyJPClass_traverse},
	{ Py_tp_clear, (void*) PyJPClass_clear},
	{ Py_tp_repr, (void*) PyJPClass_repr},
	{ Py_tp_getattro, (void*) PyJPClass_getattro},
	{ Py_tp_setattro, (void*) PyJPClass_setattro},
	{ Py_tp_methods, (void*) classMethods},
	{ Py_tp_getset, (void*) classGetSets},
	{ Py_mp_subscript, (void*) PyJPClass_array},
	{ Py_nb_matrix_multiply, (void*) PyJPClass_cast},
	{ Py_nb_inplace_matrix_multiply, (void*) PyJPClass_castEq},
	{0}
};

PyTypeObject* PyJPClass_Type = nullptr;
static PyType_Spec classSpec = {
	"_jpype._JClass",
	sizeof (PyJPClass),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	classSlots
};

#ifdef __cplusplus
}
#endif

void PyJPClass_initType(PyObject* module)
{
	JPPyObject bases = JPPyTuple_Pack(&PyType_Type);
	PyJPClass_Type = (PyTypeObject*) PyType_FromSpecWithBases(&classSpec, bases.get());
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JClass", (PyObject*) PyJPClass_Type);
	JP_PY_CHECK();
}

PyJPValueFn PyJPClass_GetJValueFn(PyTypeObject* type)
{
	// Unlike m_Class (set explicitly on every wrapped Java class, see
	// PyJPClass_hook), tp_jvalue is only ever set once, on a family's root
	// type (e.g. PyJPNumberLong_Type -- see PyJPNumber_initType). Every real
	// wrapped class (java.lang.Integer, ...) is a plain single-inheritance
	// subclass of that root and must inherit it, so walk tp_base until we
	// leave the PyJPClass-metaclass family entirely.
	while (type != nullptr && Py_TYPE(type) == (PyTypeObject*) PyJPClass_Type)
	{
		PyJPValueFn fn = ((PyJPClass*) type)->tp_jvalue;
		if (fn != nullptr)
			return fn;
		type = type->tp_base;
	}
	return nullptr;
}

void PyJPClass_SetJValueFn(PyTypeObject* type, PyJPValueFn fn)
{
	if (type == nullptr || Py_TYPE(type) != (PyTypeObject*) PyJPClass_Type)
		return;
	((PyJPClass*) type)->tp_jvalue = fn;
}

JPClass* PyJPClass_getJPClass(PyObject* obj)
{
	try
	{
		if (obj == nullptr)
			return nullptr;
		if (PyJPClass_Check(obj))
		{
			JPClass *cls = ((PyJPClass*) obj)->m_Class;
			if (cls != nullptr)
				return cls;
			// obj may be either half of an abstract/concrete pair --
			// m_Class is only ever set on the canonical type returned to
			// PyJPClass_hook's caller, so hop to whichever paired type obj
			// points to (at most one of tp_concrete/tp_abstract is ever
			// non-null on a given type) to find it. This is what keeps the
			// fast subtype check and isinstance/issubclass correct
			// regardless of which paired type a live instance's Py_TYPE
			// happens to be.
			PyTypeObject *paired = ((PyJPClass*) obj)->tp_concrete;
			if (paired == nullptr)
				paired = ((PyJPClass*) obj)->tp_abstract;
			return (paired != nullptr) ? ((PyJPClass*) paired)->m_Class : nullptr;
		}
		JPClass* cls = PyJPValue_getJPClass(obj);
		if (cls == nullptr)
			return nullptr;
		JPJavaFrame frame = JPJavaFrame::outer();
		if (cls != frame.getContext()->_java_lang_Class)
			return nullptr;
		return frame.findClass((jclass) PyJPValue_getJValue(frame, obj).l);
	} catch (...) // GCOVR_EXCL_LINE
	{
		return nullptr; // GCOVR_EXCL_LINE
	}
}

JPPyObject PyJPClass_getBases(JPJavaFrame &frame, JPClass* cls)
{
	JP_TRACE_IN("PyJPClass_bases");

	cls->ensureMembers(frame);

	// Decide the base for this object
	JPPyObject baseType;
	JPContext *context = frame.getContext();
	JPClass *super = cls->getSuperClass();
	if (dynamic_cast<JPBoxedType*> (cls) == cls)
	{
		if (cls == context->_java_lang_Boolean)
		{
			baseType = JPPyObject::use((PyObject*) PyJPNumberBool_Type);
		} else if (cls == context->_java_lang_Character)
		{
			baseType = JPPyObject::use((PyObject*) PyJPChar_Type);
		} else if (cls == context->_java_lang_Boolean
				|| cls == context->_java_lang_Byte
				|| cls == context->_java_lang_Short
				|| cls == context->_java_lang_Integer
				|| cls == context->_java_lang_Long
				)
		{
			baseType = JPPyObject::use((PyObject*) PyJPNumberLong_Type);
		} else if (cls == context->_java_lang_Float
				|| cls == context->_java_lang_Double
				)
		{
			baseType = JPPyObject::use((PyObject*) PyJPNumberFloat_Type);
		}
	} else if (JPModifier::isBuffer(cls->getModifiers()))
	{
		baseType = JPPyObject::use((PyObject*) PyJPBuffer_Type);
	} else if (cls == context->_java_lang_Throwable)
	{
		baseType = JPPyObject::use((PyObject*) PyJPException_Type);
	} else if (cls->isArray())
	{
		auto* acls = dynamic_cast<JPArrayClass*>( cls);
		if (acls->getComponentType()->isPrimitive())
			baseType = JPPyObject::use((PyObject*) PyJPArrayPrimitive_Type);
		else
			baseType = JPPyObject::use((PyObject*) PyJPArray_Type);
	} else if (cls->getCanonicalName() == "java.lang.Comparable")
	{
		baseType = JPPyObject::use((PyObject*) PyJPComparable_Type);
	} else if (super == nullptr)
	{
		baseType = JPPyObject::use((PyObject*) PyJPObject_Type);
	}

	const JPClassList& baseItf = cls->getInterfaces();
	Py_ssize_t count = baseItf.size() + (!baseType.isNull() ? 1 : 0) + (super != nullptr ? 1 : 0);

	// Pack into a tuple
	JPPyObject result = JPPyObject::call(PyList_New(count));
	unsigned int i = 0;
	for (; i < baseItf.size(); i++)
	{
		PyList_SetItem(result.get(), i, PyJPClass_create(frame, baseItf[i]).keep());
	}
	if (super != nullptr)
	{
		PyList_SetItem(result.get(), i++, PyJPClass_create(frame, super).keep());
	}
	if (!baseType.isNull())
	{
		PyList_SetItem(result.get(), i++, baseType.keep());
	}
	return result;
	JP_TRACE_OUT;
}

/**
 * Internal method for wrapping a returned Java class instance.
 *
 * This checks the cache for existing wrappers and then
 * transfers control to JClassFactory.	This is required because all of
 * the post load stuff needs to be in Python.
 *
 * @param cls
 * @return
 */
JPPyObject PyJPClass_create(JPJavaFrame &frame, JPClass* cls)
{
	JP_TRACE_IN("PyJPClass_create", cls);
	// Check the cache for speed

	auto *host = (PyObject*) cls->getHost();
	if (host == nullptr)
	{
		frame.newWrapper(cls);
		host = (PyObject*) cls->getHost();
	}
	return JPPyObject::use(host);
	JP_TRACE_OUT;
}

void PyJPClass_hook(JPJavaFrame &frame, JPClass* cls)
{
	JPContext *context = frame.getContext();
	auto *host = (PyObject*) cls->getHost();
	if (host != nullptr)
		return;


	JPPyObject members = JPPyObject::call(PyDict_New());
	JPPyObject args = JPPyTuple_Pack(
			JPPyString::fromStringUTF8(cls->getCanonicalName()).get(),
			PyJPClass_getBases(frame, cls).get(),
			members.get());

	// Catch creation loop,  the process of creating our parent
	host = (PyObject*) cls->getHost();
	if (host != nullptr)
		return;

	const JPFieldList & instFields = cls->getFields();
	for (auto instField : instFields)
	{
		JPPyObject fieldName(JPPyString::fromStringUTF8(instField->getName()));
		PyDict_SetItem(members.get(), fieldName.get(), PyJPField_create(instField).get());
	}
	const JPMethodDispatchList& m_Methods = cls->getMethods();
	for (auto m_Method : m_Methods)
	{
		JPPyObject methodName(JPPyString::fromStringUTF8(m_Method->getName()));
		PyDict_SetItem(members.get(), methodName.get(),
				PyJPMethod_create(m_Method, nullptr).get());
	}

	if (cls->isInterface())
	{
		const JPMethodDispatchList& m_Methods2 = context->_java_lang_Object->getMethods();
		for (auto m_Method : m_Methods2)
		{
			JPPyObject methodName(JPPyString::fromStringUTF8(m_Method->getName()));
			PyDict_SetItem(members.get(), methodName.get(),
					PyJPMethod_create(m_Method, nullptr).get());
		}
	}

	// Call the customizer to make any required changes to the tables.
	JP_TRACE("call pre");
	JPPyObject rc = JPPyObject::call(PyObject_Call(_JClassPre, args.get(), nullptr));

	JP_TRACE("type new");
	// Create the type using the meta class magic
	JPPyObject vself = JPPyObject::call(PyJPClass_Type->tp_call((PyObject*) PyJPClass_Type, rc.get(), PyJPClassMagic));
	auto *self = (PyJPClass*) vself.get();

	// Attach the javaSlot
	self->m_Class = cls;
	//	self->m_Class->postLoad();
	PyJPValue_assignJavaSlot(frame, (PyObject*) self, JPValue(context->_java_lang_Class,
			(jobject) self->m_Class->getJavaClass()));

	// Attach the cache  (adds reference, thus wrapper lives to end of JVM)
	JP_TRACE("set host");
	cls->setHost((PyObject*) self);

	// Call the post load routine to attach inner classes
	JP_TRACE("call post");
	args = JPPyTuple_Pack(self);
	JPPyObject rc2 = JPPyObject::call(PyObject_Call(_JClassPost, args.get(), nullptr));
}
