/*
 * Copyright 2018 nelson85.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Set the current exception as the cause of a new exception.
 *
 * @param exception
 * @param str
 */
void Py_SetStringWithCause(PyObject *exception, const char *str);

/**
 * Get a new reference to a method or property in the type dictionary without
 * dereferencing.
 *
 * @param type
 * @param attr_name
 * @return
 */
PyObject* Py_GetAttrDescriptor(PyTypeObject *type, PyObject *attr_name);

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
int Py_IsInstanceSingle(PyTypeObject* type, PyObject* obj);
int Py_IsSubClassSingle(PyTypeObject* type, PyTypeObject* obj);

struct PyJPArray
{
	PyObject_HEAD
	JPArray *m_Array;
	JPArrayView *m_View;
} ;

struct PyJPBuffer
{
	PyObject_HEAD
	JPBuffer *m_Buffer;
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
	bool m_Convert;
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
extern PyTypeObject *PyJPNumberChar_Type;
extern PyTypeObject *PyJPNumberBool_Type;

// JPype resources
extern PyObject *PyJPModule;
extern PyObject *_JArray;
extern PyObject *_JObject;
extern PyObject *_JInterface;
extern PyObject *_JException;
extern PyObject *_JClassPre;
extern PyObject *_JClassPost;
extern PyObject *_JMethodDoc;
extern PyObject *_JMethodAnnotations;
extern PyObject *_JMethodCode;
extern PyObject *_JObjectKey;

extern JPContext* JPContext_global;

// Class wrapper functions
int        PyJPClass_Check(PyObject* obj);
PyObject  *PyJPClass_FromSpecWithBases(PyType_Spec *spec, PyObject *bases);

// Class methods to add to the spec tables
PyObject  *PyJPValue_alloc(PyTypeObject* type, Py_ssize_t nitems );
void       PyJPValue_free(void* obj);
void       PyJPValue_finalize(void* obj);

// Generic methods that operate on any object with a Java slot
PyObject  *PyJPValue_str(PyObject* self);
bool       PyJPValue_hasJavaSlot(PyTypeObject* type);
Py_ssize_t PyJPValue_getJavaSlotOffset(PyObject* self);
JPValue   *PyJPValue_getJavaSlot(PyObject* obj);

// Access point for creating classes
PyObject  *PyJPModule_getClass(PyObject* module, PyObject *obj);
PyObject  *PyJPValue_getattro(PyObject *obj, PyObject *name);
int        PyJPValue_setattro(PyObject *self, PyObject *name, PyObject *value);

#ifdef __cplusplus
}
#endif

// C++ methods
JPPyObject PyJPArray_create(JPJavaFrame &frame, PyTypeObject* wrapper, const JPValue& value);
JPPyObject PyJPBuffer_create(JPJavaFrame &frame, PyTypeObject *type, const JPValue & value);
JPPyObject PyJPClass_create(JPJavaFrame &frame, JPClass* cls);
JPPyObject PyJPNumber_create(JPJavaFrame &frame, JPPyObject& wrapper, const JPValue& value);
JPPyObject PyJPField_create(JPField* m);
JPPyObject PyJPMethod_create(JPMethodDispatch *m, PyObject *instance);

JPClass*   PyJPClass_getJPClass(PyObject* obj);
JPProxy*   PyJPProxy_getJPProxy(PyObject* obj);
JPPyObject PyJPProxy_getCallable(PyObject* obj, const string& name);
void       PyJPModule_rethrow(const JPStackInfo& info);
void       PyJPValue_assignJavaSlot(JPJavaFrame &frame, PyObject* obj, const JPValue& value);
bool       PyJPValue_isSetJavaSlot(PyObject* self);

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

#endif /* PYJP_H */
