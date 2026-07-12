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

	PyObject* class_magic;
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
	PyObject* abc_collection;
	PyObject* abc_container;

	// Numpy
	PyObject* numpy_generic_type;
	PyObject* numpy_bool_type;
	PyObject* numpy_int8_type;
	PyObject* numpy_int16_type;
	PyObject* numpy_int32_type;

	PyObject* protocol_pipeline[15];

	int numpy_typepos;
	int numpy_genericpos;
	int cpp_exceptions;

	// Temporary extracted jar to clean up on shutdown, if any. Heap pointer
	// (not embedded by value) because PyJPModuleState is memset-constructed
	// and freed without destructors running, same as `context` above.
	std::string* jarTmpPath;
};

struct PyJPClass
{
	PyHeapTypeObject ht_type;
	JPClass *m_Class;
	PyObject *m_Doc;
	PyJPModuleState *m_State;
} ;



// Class wrapper functions
int		PyJPClass_Check(PyObject* obj);
PyObject  *PyJPClass_FromSpecWithBases(PyObject* mod, PyType_Spec *spec, PyObject *bases);

// Class methods to add to the spec tables
PyObject  *PyJPValue_alloc(PyTypeObject* type, Py_ssize_t nitems );
void	   PyJPValue_free(void* obj);
void	   PyJPValue_finalize(void* obj);
int		PyJPValue_traverse(PyObject *self, visitproc visit, void *arg);
int		PyJPValue_clear(PyObject *self);

// Generic methods that operate on any object with a Java slot
PyObject  *PyJPValue_str(PyObject* self);
bool	   PyJPValue_hasJavaSlot(PyTypeObject* type);
Py_ssize_t PyJPValue_getJavaSlotOffset(PyObject* self);
JPValue   *PyJPValue_getJavaSlot(PyObject* obj);

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
	return ((PyJPClass*) Py_TYPE(self))->m_State->context;
}

static inline JPContext* PyJPType_getContext(PyTypeObject* type)
{
	return ((PyJPClass*) type)->m_State->context;
}

#ifdef __cplusplus
}
#endif


#endif /* PYJP_H */
