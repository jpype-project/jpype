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
#else
#define JP_PY_TRY(...)  try { do {} while(0)
#define JP_PY_CATCH(...)  } catch(...) \
  { PyJPModule_rethrow(JP_STACKINFO()); } \
  return __VA_ARGS__
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

struct PyJPProxy
{
	PyObject_HEAD
	JPProxy* m_Proxy;
	PyObject* m_Target;
} ;

// JPype types
extern PyTypeObject *PyJPArray_Type;
extern PyTypeObject *PyJPArrayPrimitive_Type;
extern PyTypeObject *PyJPClass_Type;
extern PyTypeObject *PyJPMethod_Type;
extern PyTypeObject *PyJPObject_Type;
extern PyTypeObject *PyJPProxy_Type;
extern PyTypeObject *PyJPException_Type;
extern PyTypeObject *PyJPNumberLong_Type;
extern PyTypeObject *PyJPNumberFloat_Type;
extern PyTypeObject *PyJPNumberChar_Type;
extern PyTypeObject *PyJPNumberBool_Type;

// JPype resources
extern PyObject *_JArray;
extern PyObject *_JObject;
extern PyObject *_JInterface;
extern PyObject *_JException;
extern PyObject *_JClassPre;
extern PyObject *_JClassPost;
extern PyObject *_JMethodDoc;
extern PyObject *_JMethodAnnotations;
extern PyObject *_JMethodCode;

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

#define ASSERT_JVM_RUNNING() JPEnv::assertJVMRunning(JP_STACKINFO())

// C++ methods
JPPyObject PyJPArray_create(PyTypeObject* wrapper, JPValue& value);
JPPyObject PyJPClass_create(JPClass* cls);
JPPyObject PyJPField_create(JPField* m);
JPPyObject PyJPMethod_create(JPMethod *m, PyObject *instance);
JPPyObject PyJPNumber_create(JPPyObject& wrapper, const JPValue& value);
JPPyObject PyJPValue_create(const JPValue& value);

JPClass*   PyJPClass_getJPClass(PyObject* obj);
JPProxy*   PyJPProxy_getJPProxy(PyObject* obj);
JPPyObject PyJPProxy_getCallable(PyObject* obj, const string& name);
void       PyJPModule_rethrow(const JPStackInfo& info);
void       PyJPValue_assignJavaSlot(PyObject* obj, const JPValue& value);
bool       PyJPValue_isSetJavaSlot(PyObject* self);

#endif /* PYJP_H */