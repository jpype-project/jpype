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
#ifndef PYJP_H
#define PYJP_H
#include <Python.h>
#include "jpype.h"
#include "jp_pythontypes.h"

// Py_SET_TYPE/Py_SET_REFCNT/Py_SET_SIZE became public macros in CPython 3.9
// (bpo-39573); before that, Py_TYPE(obj)/Py_REFCNT(obj)/Py_SIZE(obj)
// themselves were assignable lvalue macros. Needed for the
// polymorph-back-to-canonical-type step in jp_class.cpp/pyjp_object.cpp and
// the tagged-number recycling pool in pyjp_number.cpp on Python 3.8, the
// oldest version this project still supports.
#if PY_VERSION_HEX < 0x03090000
#define Py_SET_TYPE(obj, type) ((Py_TYPE(obj) = (type)))
#define Py_SET_REFCNT(obj, refcnt) ((Py_REFCNT(obj) = (refcnt)))
#define Py_SET_SIZE(obj, size) ((Py_SIZE(obj) = (size)))
#endif

// Critical sections (per-object locks of the free-threaded build) exist since CPython 3.13 and are no-ops
// in builds with a global interpreter lock. Older versions always have the lock, so plain blocks are enough.
// NOTE: the macros open a block; never "return", "goto" or "break" out of it.
#if PY_VERSION_HEX < 0x030D0000
#ifndef Py_BEGIN_CRITICAL_SECTION
#define Py_BEGIN_CRITICAL_SECTION(op) {
#define Py_END_CRITICAL_SECTION() }
#endif
#endif

// ---------------------------------------------------------------------------------------------------------------------
// Lazily filled slots of Python objects (documentation, annotations, ...), shared by all threads.
//
// Every access to such a slot happens inside a critical section on the owning object (a no-op with a global
// interpreter lock) and takes or releases a strong reference before the section ends; a slot is never read twice for
// one decision. Reference counts are dropped outside of the section, because that can run arbitrary code.

// New reference to the value of a slot, or NULL (without an error) if it is not set yet.
inline PyObject *PyJPSlot_peek(PyObject *owner, PyObject **slot)
{
	PyObject *value = nullptr;
	Py_BEGIN_CRITICAL_SECTION(owner);
	value = *slot;
	Py_XINCREF(value);
	Py_END_CRITICAL_SECTION();
	return value;
}

// Publish a lazily computed value. Steals the reference to "computed" (NULL passes through, with the Python error
// already set). If another thread was faster, its value wins and ours is dropped, so all callers observe the same
// object. Returns a new reference.
//
// Racing threads therefore each compute the value; only one result is kept. This is deliberate. Holding the
// owner's critical section while computing would not prevent it: the builders (_jclassDoc, getMethodDoc, ...) call
// Java, which detaches the thread (PyEval_SaveThread) and so suspends its critical sections. A real lock held across
// the computation would make it single-flight but deadlocks: the builder creates class wrappers under the type
// factory monitor, whose holder may be running a customizer that reads this slot (DocBuildDeadlockTest). The cost
// of racing is one extra build per racing thread on the first access only (about 2.5 ms for a class __doc__).
inline PyObject *PyJPSlot_publish(PyObject *owner, PyObject **slot, PyObject *computed)
{
	if (computed == nullptr)
		return nullptr;
	PyObject *result = nullptr;
	Py_BEGIN_CRITICAL_SECTION(owner);
	if (*slot == nullptr)
	{
		*slot = computed;       // the slot owns one reference ...
		Py_INCREF(computed);    // ... and the caller keeps the original one
		result = computed;
		computed = nullptr;
	}
	else
	{
		result = *slot;
		Py_INCREF(result);
	}
	Py_END_CRITICAL_SECTION();
	Py_XDECREF(computed);
	return result;
}

// Replace the value of a slot (the __doc__ / __annotations__ setters). Does not steal a reference.
inline void PyJPSlot_replace(PyObject *owner, PyObject **slot, PyObject *value)
{
	Py_XINCREF(value);
	PyObject *old = nullptr;
	Py_BEGIN_CRITICAL_SECTION(owner);
	old = *slot;
	*slot = value;
	Py_END_CRITICAL_SECTION();
	Py_XDECREF(old);
}

// Visit a slot from tp_traverse. Read it directly, without the critical section or a temporary reference, like
// CPython's own types do. On free-threaded builds every caller of tp_traverse stops the world first (the collector,
// gc.get_referents and gc.get_referrers), so no other thread can replace the slot meanwhile; on GIL builds the GIL
// excludes the writers. Taking a reference inside tp_traverse is outside its contract.
inline int PyJPSlot_visit(PyObject *owner, PyObject **slot, visitproc visit, void *arg)
{
	(void) owner;
	Py_VISIT(*slot);
	return 0;
}

// Detach a slot from tp_clear and return its reference. The caller owns the returned reference and must drop it
// outside of the critical section, because that can run arbitrary __del__ code.
inline PyObject *PyJPSlot_take(PyObject *owner, PyObject **slot)
{
	PyObject *value = nullptr;
	Py_BEGIN_CRITICAL_SECTION(owner);
	value = *slot;
	*slot = nullptr;
	Py_END_CRITICAL_SECTION();
	return value;
}

class JPStackInfo;
#ifdef JP_TRACING_ENABLE
#define JP_PY_TRY(...) \
  JPypeTracer _trace(__VA_ARGS__); \
  try { do {} while(0)
#define JP_PY_CATCH(...) \
  } catch(...) { \
  PyJPModule_rethrow(JP_STACKINFO()); } \
  return __VA_ARGS__
#define JP_PY_CATCH_NONE(...)  } catch(...) {} return __VA_ARGS__
#else
#ifndef JP_INSTRUMENTATION
#define JP_PY_TRY(...)  try { do {} while(0)
#else
#define JP_PY_TRY(...)  JP_TRACE_IN(__VA_ARGS__)
#endif
#define JP_PY_CATCH(...)  } catch(...) \
  { PyJPModule_rethrow(JP_STACKINFO()); } \
  return __VA_ARGS__
#define JP_PY_CATCH_NONE(...)  } catch(...) {} return __VA_ARGS__
#endif

// Macro to all after executing a Python command that can result in
// a failure to convert it to an exception.
#define JP_PY_CHECK() { if (PyErr_Occurred() != 0) JP_RAISE_PYTHON();  } // GCOVR_EXCL_LINE

#ifdef __cplusplus
extern "C"
{
#endif

// Needed to write common code with older versions
#ifndef Py_TRASHCAN_BEGIN
// Introduced in Python 3.8
#define Py_TRASHCAN_BEGIN(X, Y)
#define Py_TRASHCAN_END
#endif

PyMODINIT_FUNC PyInit__jpype();

/**
 * Set the current exception as the cause of a new exception.
 *
 * @param exception
 * @param str
 */
void PyJP_SetStringWithCause(PyObject *exception, const char *str);

/**
 * Get a new reference to a method or property in the type dictionary without
 * dereferencing.
 *
 * @param type
 * @param attr_name
 * @return
 */
PyObject* PyJP_GetAttrDescriptor(PyTypeObject *type, PyObject *attr_name);

/**
 * Fast check to see if a type derives from another.
 *
 * This depends on the MRO order.  It is useful of our base types where
 * the order is fixed.
 *
 * @param type
 * @param obj
 * @return 1 if object derives from type.
 */
int PyJP_IsInstanceSingle(PyObject* obj, PyTypeObject* type);
int PyJP_IsSubClassSingle(PyTypeObject* type, PyTypeObject* obj);

struct PyJPArray
{
	PyObject_HEAD
	JPArray *m_Array;
	JPArrayView *m_View;
	// Fixed-offset Java-value slot (see PyJPClass_FromSpecWithBases in
	// pyjp.h/pyjp_class.cpp) -- Array is always single-inheritance below
	// Object, so it can go straight to concrete like Exception. A bare
	// jvalue, not a full JPValue: the class is always derived from the
	// wrapper type instead (see PyJPValue_getJPClass), so there's no need
	// to duplicate a class pointer on every instance.
	jvalue extra;
} ;

struct PyJPClassHints
{
	PyObject_HEAD
	JPClassHints *m_Hints;
} ;

struct PyJPProxy
{
	PyObject_HEAD
	JPProxy* m_Proxy;
	PyObject* m_Target;
	PyObject* m_Dispatch;
	bool m_Convert;
} ;

struct JPConversionInfo
{
	PyObject *ret;
	PyObject *exact;
	PyObject *implicit;
	PyObject *attributes;
	PyObject *expl;
	PyObject *none;
} ;


// JPype types
extern PyTypeObject *PyJPArray_Type;
extern PyTypeObject *PyJPArrayPrimitive_Type;
extern PyTypeObject *PyJPBuffer_Type;
extern PyTypeObject *PyJPClass_Type;
extern PyTypeObject *PyJPComparable_Type;
extern PyTypeObject *PyJPMethod_Type;
extern PyTypeObject *PyJPObject_Type;
extern PyTypeObject *PyJPProxy_Type;
extern PyTypeObject *PyJPException_Type;
extern PyTypeObject *PyJPNumberLong_Type;
extern PyTypeObject *PyJPNumberFloat_Type;
extern PyTypeObject *PyJPNumberBool_Type;
extern PyTypeObject *PyJPChar_Type;

// Data descriptor equivalent to a single PyGetSetDef entry, except that
// accessing it unbound (directly on the owning type, rather than an
// instance) safely returns None instead of the raw descriptor object or
// misinterpreting the type object as an instance of itself. See its
// definition in pyjp_class.cpp for the full rationale (#1213).
PyObject* PyJP_NewUnboundSafeGetSet(getter get, setter set);


// JPype resources
extern PyObject *PyJPModule;
extern PyObject *_JArray;
extern PyObject *_JChar;
extern PyObject *_JObject;
extern PyObject *_JInterface;
extern PyObject *_JException;
extern PyObject *_JClassPre;
extern PyObject *_JClassPost;
extern PyObject *_JClassDoc;
extern PyObject *_JMethodDoc;
extern PyObject *_JMethodAnnotations;
extern PyObject *_JMethodCode;
extern PyObject *_JObjectKey;
extern PyObject *_JVMNotRunning;
extern PyObject *PyJPClassMagic;
extern PyObject *PyJPClassMagicConcrete;
// for caching type checks with Numpy bool after np version 2.1
extern PyObject* _num_bool_type;
extern PyObject* _numpy_int8_type;
extern PyObject* _numpy_int16_type;
extern PyObject* _numpy_int32_type;
extern PyObject* _numpy_bool_type;

extern JPContext* JPContext_global;

// Class wrapper functions
int        PyJPClass_Check(PyObject* obj);
// offset: the Java-value slot family for the resulting type. Every family
// bakes its slot's location into tp_basicsize itself, so callers must
// always pass one of:
//   -1  -> abstract: kept layout-trivial (safe to mix with any foreign
//          family, e.g. boxed Number/Buffer/Array/Char) and immediately
//          paired with a hidden concrete companion type that carries the
//          real, fixed offset.
//   >0  -> concrete: caller supplies the exact fixed byte offset directly
//          (e.g. Exception, which has a real compile-time C struct), no
//          companion needed.
// (0 used to mean "legacy family, resolved at runtime via the thread-local
// dummy-heap-type allocator" -- that allocator, PyJPValue_alloc, has been
// removed; passing 0 is now a hard internal error.)
PyObject  *PyJPClass_FromSpecWithBases(PyType_Spec *spec, PyObject *bases, Py_ssize_t offset);
// Once a type is fully created, this is ALWAYS the real, resolved byte
// offset -- including for an abstract type, whose offset field is flattened
// to the same value as its hidden concrete companion's the moment that
// companion is built (see the concreteCall branch of PyJPClass_init). There
// is no longer a -1 case to resolve at read time: offset is a hard
// invariant, not something callers branch on or chase a companion for.
// (-1 remains meaningful only as an INPUT to PyJPClass_FromSpecWithBases,
// requesting that a type be created abstract in the first place.)
Py_ssize_t PyJPClass_getOffset(PyTypeObject* type);
// Abstract/concrete pairing, split into two explicitly one-directional
// accessors rather than one ambiguous shared link -- see the struct
// PyJPClass field comments in pyjp_class.cpp for the full reasoning
// (tp_concrete is the owned edge, tp_abstract is a raw non-owned back-edge).
PyTypeObject* PyJPClass_getConcrete(PyTypeObject* type);
PyTypeObject* PyJPClass_getAbstract(PyTypeObject* type);

// The JPClass shared by every instance of this wrapper type, held on the
// metaclass rather than duplicated per instance (a wrapper instance's class
// never varies -- it's a property of its type, not the instance). Null if
// type isn't a Java wrapper type.
JPClass*      PyJPClass_GetClass(PyTypeObject* type);
// Per-boxed-class singleton representing JObject(None, cls) -- a real Java
// null of a specific static boxed type. Lazily built and cached on first
// null cast (JPBoxedType::convertToPythonObject); null until then, and
// always null for non-boxed types.
PyObject*     PyJPClass_GetNullBoxed(PyTypeObject* type);
// Publish "obj" as the null singleton unless another thread was first.
// Returns a new reference to the singleton in use.
PyObject*     PyJPClass_GetOrSetNullBoxed(PyTypeObject* type, PyObject* obj);

// Class methods to add to the spec tables
void       PyJPValue_free(void* obj);
void       PyJPValue_finalize(void* obj);
int        PyJPValue_traverse(PyObject *self, visitproc visit, void *arg);
int        PyJPValue_clear(PyObject *self);

// Generic methods that operate on any object with a Java slot
PyObject  *PyJPValue_str(PyObject* self);
bool       PyJPValue_hasJavaSlot(PyTypeObject* type);
Py_ssize_t PyJPValue_getJavaSlotOffset(PyObject* self);

// JPValue (the bundled class+jvalue struct) is never embedded per-instance;
// its two halves are read independently instead. getJPClass never needs the
// JVM (a wrapper instance's class is a property of its type, set once when
// the wrapper type itself was created -- see PyJPClass_GetClass) and
// returns nullptr only when self's type carries no Java slot at all (an
// ordinary Python object). getJValue lives in the C++-only section below
// since it needs a JPJavaFrame&: for most families it's still a live
// per-instance value; for families with no per-instance value at all (see
// PyJPValueFn below) it requires an actual JNI call to reconstruct.
JPClass*   PyJPValue_getJPClass(PyObject* obj);

// Access point for creating classes
PyObject  *PyJPModule_getClass(PyObject* module, PyObject *obj);
PyObject  *PyJPValue_getattro(PyObject *obj, PyObject *name);
int        PyJPValue_setattro(PyObject *self, PyObject *name, PyObject *value);
PyObject  *PyJPChar_Create(PyTypeObject *type, Py_UCS2 p);
PyTypeObject* PyJP_GetNumPyBaseType(PyTypeObject* obj);

// Build a boxed/primitive-wrapper int value directly: an ordinary PyLong
// subtype instance (via CPython's own long_subtype_new, dispatched through
// PyLong_Type.tp_new with the real subtype) holding `value`. No per-instance
// Java-value storage is attached -- see longJValue (pyjp_number.cpp), which
// reconstructs a jvalue from the instance's own digits on demand instead.
// Used by every construction path for Long/Boolean/int-like primitive
// wrappers, including JPPrimitiveType::convertLong (jp_primitivetype.cpp).
PyObject  *PyJPNumber_longFromLongLong(PyTypeObject* type, long long value);

#ifdef __cplusplus
}
#endif

void       PyJPClass_hook(JPJavaFrame &frame, JPClass* cls);

// C++ methods
JPPyObject PyJPArray_create(JPJavaFrame &frame, PyTypeObject* wrapper, const JPValue& value);
JPPyObject PyJPBuffer_create(JPJavaFrame &frame, PyTypeObject *type, const JPValue & value);
JPPyObject PyJPClass_create(JPJavaFrame &frame, JPClass* cls);
JPPyObject PyJPNumber_create(JPJavaFrame &frame, JPPyObject& wrapper, const JPValue& value);
JPPyObject PyJPField_create(JPField* m);
JPPyObject PyJPMethod_create(JPMethodDispatch *m, PyObject *instance);

JPClass*   PyJPClass_getJPClass(PyObject* obj);
JPProxy*   PyJPProxy_getJPProxy(PyObject* obj);
void       PyJPModule_rethrow(const JPStackInfo& info);
void       PyJPValue_assignJavaSlot(JPJavaFrame &frame, PyObject* obj, const JPValue& value);
bool       PyJPValue_isSetJavaSlot(PyObject* self);

// Per-type hook that reconstructs a jvalue on demand for families with no
// live per-instance value at all (boxed/primitive numeric types,
// Character). Null for families that still store jvalue directly (general
// objects/arrays/exceptions, Float/Double).
typedef jvalue (*PyJPValueFn)(JPJavaFrame&, PyObject*);
PyJPValueFn PyJPClass_GetJValueFn(PyTypeObject* type);
void        PyJPClass_SetJValueFn(PyTypeObject* type, PyJPValueFn fn);

// See PyJPValue_getJPClass above -- the jvalue half of the old JPValue pair.
jvalue     PyJPValue_getJValue(JPJavaFrame& frame, PyObject* obj);
JPPyObject PyTrace_FromJavaException(JPJavaFrame& frame, jthrowable th, jthrowable prev);
void       PyJPException_normalize(JPJavaFrame frame, JPPyObject exc, jthrowable th, jthrowable enclosing);

#define _ASSERT_JVM_RUNNING(context) assertJVMRunning((JPContext*)context, JP_STACKINFO())

/**
 * Use this when getting the context where the context must be running.
 *
 * The context needs to be accessed before accessing and JPClass* or other
 * internal structured.  Those resources are owned by the JVM and thus
 * will be deleted when the JVM is shutdown.  This method will throw if the
 * JVM is not running.
 *
 * If the context may or many not be running access JPContext_global directly.
 */
inline JPContext* PyJPModule_getContext()
{
#ifdef JP_INSTRUMENTATION
	PyJPModuleFault_throw(compile_hash("PyJPModule_getContext"));
#endif
	JPContext* context = JPContext_global;
	_ASSERT_JVM_RUNNING(context); // GCOVR_EXCL_LINE
	return context;
}
void PyJPModule_loadResources(PyObject* module);

#endif /* PYJP_H */
