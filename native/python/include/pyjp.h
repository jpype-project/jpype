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
#include <atomic>
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

// Use after a CPython C-API call whose *only* failure signal is a NULL
// return (the common case) -- unlike JP_PY_CHECK(), only calls
// PyErr_Occurred() when obj is actually NULL, since a real API contract
// never returns non-NULL with an exception left pending.
#define JP_PY_CHECK_NULL(obj) { if ((obj) == nullptr) { JP_PY_CHECK(); } }

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
	PyJPModuleState* m_State;
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

struct PyJPModuleState
{
	PyObject* module;
	JPContext* context;
	PyObject* module_dict; // borrowed
	PyInterpreterState* interp_state;
	// The thread state Py_NewInterpreterFromConfig() returned when this
	// subinterpreter was created (nullptr for the main interpreter). It is
	// swapped out (detached) once startup finishes, but stays allocated -
	// Py_EndInterpreter() requires being called with the interpreter's sole
	// remaining thread state, so finishSub() must reattach to this exact
	// state rather than creating a new one (which would leave this one as an
	// orphan and make Py_EndInterpreter fail with "not the last thread").
	PyThreadState* root_tstate;
	bool is_main_interpreter;  // true if this is the main Python interpreter
	bool is_shutting_down;     // true when interpreter is finalizing - don't call Python APIs
	int count;
	int held;

	// Types (installed by init*)
	PyTypeObject* PyJPClass_Type;
	PyTypeObject* PyJPObject_Type;
	PyTypeObject* PyJPException_Type;
	PyTypeObject* PyJPComparable_Type;
	PyTypeObject* PyJPArray_Type;
	PyTypeObject* PyJPArrayPrimitive_Type;
	PyTypeObject* PyJPArrayIter_Type;
	PyTypeObject* PyJPBuffer_Type;
	PyTypeObject* PyJPChar_Type;
	PyTypeObject* PyJPField_Type;
	PyTypeObject* PyJPMethod_Type;
	PyTypeObject* PyJPMonitor_Type;
	PyTypeObject* PyJPProxy_Type;
	PyTypeObject* PyJPNumberLong_Type;
	PyTypeObject* PyJPNumberFloat_Type;
	PyTypeObject* PyJPNumberBool_Type;
	PyTypeObject* PyJPClassHints_Type;
	PyTypeObject* PyJPPackage_Type;

	// Per-(sub)interpreter recycling pool for the JByte/JShort/JInt/JLong
	// tagged-number leaves (see pyjp_number.cpp's intfreelist). Deliberately
	// not process-wide: it recycles raw allocated blocks, and under a
	// per-interpreter GIL/allocator build (PEP 684) a block freed under one
	// interpreter's obmalloc arena and popped back out under another's would
	// corrupt that interpreter's heap.
	struct
	{
		std::atomic<void*> head;
		std::atomic<int> count;
		PyTypeObject* eligibleTypes[4];
	} intfreelist;

	// Per-interpreter JBoolean(True)/JBoolean(False) singletons (see
	// pyjp_number.cpp's PyJPBoolean_new) -- each (sub)interpreter builds its
	// own JBoolean leaf type, so the cached instances must be scoped the
	// same way, not shared process-wide.
	PyObject* boolSingleton[2];
	PyTypeObject* boolLeafType;

	PyObject* class_magic;
	PyObject* class_magic_concrete;
	PyObject* Py_JP_CALL;
	PyObject* strings_dict;

	// Resources (loadResources)

	// Frontend
	PyObject* JObject;
	PyObject* JInterface;
	PyObject* JArray;
	PyObject* JChar;
	PyObject* JException;

	// Class
	PyObject* JClassPre;
	PyObject* JClassPost;

	// Cache
	PyObject* cacheDict;
	PyObject* cacheInterfacesDict;
	PyObject* cacheMethodsDict;
	PyObject* package_dict;

	// Doc
	PyObject* JClassDoc;
	PyObject* JMethodDoc;
	PyObject* JMethodAnnotations;
	PyObject* JMethodCode;

	// GC
	PyObject* python_gc;
    PyObject* gc_callbacks;
    PyObject* collect;

	// Guards
	PyObject* JObjectKey;

	// Bridge
	PyObject* concreteDict;
	PyObject* protocolDict;
	PyObject* methodsDict;

	PyObject* abc_sequence;
	PyObject* abc_mapping;
	PyObject* abc_generator;
	PyObject* abc_iterator;
	PyObject* abc_iterable;
	PyObject* abc_coroutine;
	PyObject* abc_awaitable;
	PyObject* abc_set;
	PyObject* abc_mutable_set;
	PyObject* abc_collection;
	PyObject* abc_container;

	// Numpy
	PyObject* numpy_generic_type;
	PyObject* numpy_bool_type;
	PyObject* numpy_int8_type;
	PyObject* numpy_int16_type;
	PyObject* numpy_int32_type;

	PyObject* protocol_pipeline[16];

	int numpy_typepos;
	int numpy_genericpos;
	int cpp_exceptions;

	// Temporary extracted jar to clean up on shutdown, if any. Heap pointer
	// (not embedded by value) because PyJPModuleState is memset-constructed
	// and freed without destructors running, same as `context` above.
	std::string* jarTmpPath;
};

// Per-type hook that reconstructs a jvalue on demand for families with no
// live per-instance value at all (boxed/primitive numeric types,
// Character). Null for families that still store jvalue directly (general
// objects/arrays/exceptions, Float/Double). Declared here (ahead of struct
// PyJPClass below, which needs it for tp_jvalue) rather than down with the
// rest of the C++-only declarations after extern "C" closes.
typedef jvalue (*PyJPValueFn)(JPJavaFrame&, PyObject*);

struct PyJPClass
{
	PyHeapTypeObject ht_type;
	JPClass *m_Class;
	PyObject *m_Doc;
	PyJPModuleState *m_State;
	// Java-value slot bookkeeping for the fixed-offset object model.  See
	// the offset parameter documentation below (PyJPClass_FromSpecWithBases).
	// Once a type is fully created this is ALWAYS the real, resolved byte
	// offset -- for an abstract/concrete pair, the same value is written
	// onto BOTH halves (see the concreteCall branch of PyJPClass_init), so
	// no caller ever needs to branch or chase a companion to use it. Never
	// -1 in steady state; 0 is a hard-error sentinel for legacy families
	// that no longer exist.
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
// for caching type checks with Numpy bool after np version 2.1
extern PyObject* _num_bool_type;

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
PyObject  *PyJPClass_FromSpecWithBases(PyObject* module, PyType_Spec *spec, PyObject *bases, Py_ssize_t offset);
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
void          PyJPClass_SetNullBoxed(PyTypeObject* type, PyObject* obj);

// Class methods to add to the spec tables
void       PyJPValue_free(void* obj);
void       PyJPValue_finalize(void* obj);
int        PyJPValue_traverse(PyObject *self, visitproc visit, void *arg);
int        PyJPValue_clear(PyObject *self);

// Generic methods that operate on any object with a Java slot
PyObject  *PyJPValue_str(PyObject* self);
bool	   PyJPValue_hasJavaSlot(PyTypeObject* type);
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
PyObject  *PyJPValue_getattro(PyObject *obj, PyObject *name);
int		PyJPValue_setattro(PyObject *self, PyObject *name, PyObject *value);
PyObject  *PyJPChar_Create(PyTypeObject *type, Py_UCS2 p);
PyTypeObject* PyJP_GetNumPyBaseType(PyJPModuleState* st, PyTypeObject* obj);

PyObject* PyJP_probe(PyJPModuleState* st, PyTypeObject *other);
PyObject* PyJP_pyobject(PyJPModuleState* st, PyTypeObject* type, PyObject *object);
PyObject *PyJPModule_convertBuffer(PyJPModuleState* st, JPPyBuffer& buffer, PyObject *dtype);

void	   PyJPClass_hook(JPJavaFrame &frame, JPClass* cls);

JPPyObject PyJPArray_create(JPJavaFrame &frame, PyTypeObject* wrapper, const JPValue& value);
JPPyObject PyJPBuffer_create(JPJavaFrame &frame, PyTypeObject *type, const JPValue & value);
JPPyObject PyJPClass_create(JPJavaFrame &frame, JPClass* cls);
JPPyObject PyJPNumber_create(JPJavaFrame &frame, JPPyObject& wrapper, const JPValue& value);
JPPyObject PyJPField_create(JPJavaFrame &frame, JPField* m);
JPPyObject PyJPMethod_create(JPJavaFrame &frame, JPMethodDispatch *m, PyObject *instance);

JPClass*   PyJPClass_getJPClass(PyObject* obj);
JPProxy*   PyJPProxy_getJPProxy(PyJPModuleState* st, PyObject* obj);
void	   PyJPModule_rethrow(const JPStackInfo& info);
void	   PyJPValue_assignJavaSlot(JPJavaFrame &frame, PyObject* obj, const JPValue& value);
bool	   PyJPValue_isSetJavaSlot(PyObject* self);
JPPyObject PyTrace_FromJavaException(JPJavaFrame& frame, jthrowable th, jthrowable prev);
void	   PyJPException_normalize(JPJavaFrame frame, JPPyObject exc, jthrowable th, jthrowable enclosing);

void PyJPModule_installGC(PyObject* module);
void PyJPModule_loadResources(PyObject* module, PyJPModuleState* st);

void PyJPArray_initType(PyObject* module, PyJPModuleState* st);
void PyJPBuffer_initType(PyObject* module, PyJPModuleState* st);
void PyJPClass_initType(PyObject* module, PyJPModuleState* st);
void PyJPField_initType(PyObject* module, PyJPModuleState* st);
void PyJPMethod_initType(PyObject* module, PyJPModuleState* st);
void PyJPMonitor_initType(PyObject* module, PyJPModuleState* st);
void PyJPProxy_initType(PyObject* module, PyJPModuleState* st);
void PyJPObject_initType(PyObject* module, PyJPModuleState* st);
void PyJPNumber_initType(PyObject* module, PyJPModuleState* st);
void PyJPClassHints_initType(PyObject* module, PyJPModuleState* st);
void PyJPPackage_initType(PyObject* module, PyJPModuleState* st);
void PyJPChar_initType(PyObject* module, PyJPModuleState* st);


#define _ASSERT_JVM_RUNNING(context) assertJVMRunning((JPContext*)context, JP_STACKINFO())

inline JPContext* PyJPObject_getContext(PyObject* self)
{
	// self may itself be a generated wrapper TYPE object (e.g. `JClass
	// ("java.lang.Integer")` passed to jpype.synchronized()), not just an
	// ordinary instance -- PyJPClass_Check(self) is the same self-vs-
	// Py_TYPE(self) distinction already established for
	// PyJPValue_getJPClass/getJValue. Using Py_TYPE(self) unconditionally
	// treated a wrapper type's own metaclass (PyJPClass_Type) as if it
	// were a struct PyJPClass instance, reading garbage past its real
	// PyHeapTypeObject layout.
	//
	// m_State is set exactly once, at type-creation time, and no code
	// path ever nulls it out afterward -- but CPython's interpreter
	// shutdown does not guarantee a type outlives its instances'
	// tp_finalize calls the way ordinary runtime GC does, so a still-
	// pending instance's tp_finalize can run after its own type's C-level
	// state has already been torn down as part of unwinding the owning
	// module. Every caller of this function already tolerates a null
	// JPContext* return (context == nullptr is the first thing checked);
	// this null-checks m_State itself so that shutdown-time race doesn't
	// crash before reaching those checks (see the "no ambient is-the-JVM-
	// up-at-all bail here" comment in PyJPValue_finalize for the sibling
	// per-context safety net this complements).
	if (PyJPClass_Check(self))
	{
		auto* state = ((PyJPClass*) self)->m_State;
		return state == nullptr ? nullptr : state->context;
	}
	auto* state = ((PyJPClass*) Py_TYPE(self))->m_State;
	return state == nullptr ? nullptr : state->context;
}

static inline JPContext* PyJPType_getContext(PyTypeObject* type)
{
	// See PyJPObject_getContext's comment: m_State can be null here during
	// interpreter shutdown, when a type's own C-level state may already be
	// torn down by the time some instance's cleanup path still runs.
	auto* state = ((PyJPClass*) type)->m_State;
	return state == nullptr ? nullptr : state->context;
}

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

// See the PyJPValueFn typedef above (declared ahead of struct PyJPClass).
PyJPValueFn PyJPClass_GetJValueFn(PyTypeObject* type);
void        PyJPClass_SetJValueFn(PyTypeObject* type, PyJPValueFn fn);

// See PyJPValue_getJPClass above -- the jvalue half of the old JPValue pair.
jvalue     PyJPValue_getJValue(JPJavaFrame& frame, PyObject* obj);

// See PyJPModule_loadResources(PyObject*, PyJPModuleState*) above -- this
// zero-st overload is used by pyjp_module.cpp's own bootstrap path (before
// a state pointer is threaded down that far).
void PyJPModule_loadResources(PyObject* module);

#endif /* PYJP_H */
