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
#include <jpype.h>

#ifdef JP_TRACING_ENABLE
#define JP_PY_TRY(...) \
  JPypeTracer _trace(__VA_ARGS__); \
  try {
#define JP_PY_CATCH(...) \
  } catch(...) { \
  JPPythonEnv::rethrow(JP_STACKINFO()); } \
  return __VA_ARGS__
#else
#define JP_PY_TRY(...)  try {
#define JP_PY_CATCH(...)  } catch(...) { JPPythonEnv::rethrow(JP_STACKINFO()); } return __VA_ARGS__
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Search for a method in the tree.
 *
 * The regular method in Python always de-references the descriptor before
 * we can examine it.  We need to lookup it out without de-referencing it.
 * This will only searches the type dictionaries.
 *
 * @param type
 * @param attr_name
 * @return
 */
PyObject* PyType_Lookup(PyTypeObject *type, PyObject *attr_name);

/** Structure to hold all types associated with the module.
 *
 * Rather than holding a bunch of global variables, the preferred method
 * is hold everything in the state.  This will allow sub-interpreters to have
 * their own state if needed.  To access these type variables use the
 * PyJPModuleState macros.  We have not do extensive testing on sub-interpreters
 * and it is likely something is still wrong with our implementation.
 */
typedef struct
{
	JPContext* m_Context;
	PyObject* PyJPArray_Type;
	PyObject *PyJPClass_Type;
	PyObject *PyJPClassHints_Type;
	PyObject *PyJPClassMeta_Type;
	PyObject *PyJPField_Type;
	PyObject *PyJPMethod_Type;
	PyObject *PyJPProxy_Type;
	PyObject *PyJPValueBase_Type;
	PyObject *PyJPValue_Type;
	PyObject *PyJPValueLong_Type;
	PyObject *PyJPValueFloat_Type;
	PyObject *PyJPValueExc_Type;
} PyJPModuleState;

extern PyModuleDef PyJPModuleDef;
extern PyJPModuleState *PyJPModuleState_global;

#define PyJPModule_global PyState_FindModule(&PyJPModuleDef)

struct PyJPClassHints
{
	PyObject_HEAD
	JPClassHints *m_Hints;
} ;

struct PyJPValue
{
	PyObject_HEAD
	JPValue m_Value;
} ;

/** This is a wrapper for accessing the array method.  It is structured to
 * be like a bound method.  It should not be the primary handle to the object.
 * That will be a PyJPValue.
 */
struct PyJPArray
{
	PyJPValue m_Value;
	JPArray *m_Array;
} ;

struct PyJPClass
{
	PyJPValue m_Value;
	JPClass *m_Class;
} ;

struct PyJPMethod
{
	PyFunctionObject func;
	JPMethodDispatch* m_Method;
	PyObject* m_Instance;
	PyObject* m_Doc;
	PyObject* m_Annotations;
	PyObject* m_CodeRep;
} ;

struct PyJPField
{
	PyJPValue m_Value;
	JPField *m_Field;
} ;

/** Python object to support jpype.synchronized(object) command.
 */
struct PyJPMonitor
{
	PyObject_HEAD
	JPMonitor *m_Monitor;
} ;

struct PyJPProxy
{
	PyObject_HEAD
	JPProxy *m_Proxy;
	PyObject *m_Target;
} ;



PyJPValue* PyJPValue_asValue(PyObject *self);
PyObject *PyJPModule_getClass(PyObject *module, PyObject *args);

extern const char *__javavalue__;
extern const char *__javaproxy__;
extern const char *__javaclass__;

#ifdef __cplusplus
}
#endif

// C++ methods

inline JPContext* PyJPModule_getContext()
{
	PyJPModuleState *state = PyJPModuleState_global;
	JPContext* context = state->m_Context;
	ASSERT_JVM_RUNNING(context);
	return context;
}

JPPyObject PyJPValue_create(PyTypeObject *type, JPContext *context, const JPValue& value);
JPPyObject PyJPArray_create(PyTypeObject *wrapper, JPContext *context, const JPValue& value);
JPPyObject PyJPClass_create(PyTypeObject *type, JPContext *context, JPClass *cls);
JPPyObject PyJPField_create(JPField *field);
JPPyObject PyJPMethod_create(JPMethodDispatch *m, PyObject *instance);
JPPyObject PyJPValue_createInstance(PyTypeObject *wrapper, JPContext *context, const JPValue& value);
JPPyObject PyJPValue_createBase(PyTypeObject *wrapper, JPContext *context, const JPValue& value);
JPPyObject PyJPValue_createBoxed(PyTypeObject *wrapper, JPContext *context, const JPValue& value);

#endif /* PYJP_H */
