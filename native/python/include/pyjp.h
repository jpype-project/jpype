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

#define ASSERT_JVM_RUNNING(context) ((JPContext*)context)->assertJVMRunning(JP_STACKINFO())
#define PY_STANDARD_CATCH(...) catch(...) { JPPythonEnv::rethrow(JP_STACKINFO()); } return __VA_ARGS__

/** This is the module specifically for the cpython modules to include.
 */
#include <Python.h>
#include <jpype.h>

PyObject* PyType_Lookup(PyTypeObject *type, PyObject *attr_name);

// Forward declare the types
extern PyObject* PyJPArray_Type;
extern PyObject* PyJPContext_Type;
extern PyObject *PyJPClass_Type;
extern PyObject *PyJPClassHints_Type;
extern PyObject *PyJPClassMeta_Type;
extern PyObject *PyJPField_Type;
extern PyObject *PyJPMethod_Type;
extern PyObject *PyJPMonitor_Type;
extern PyObject *PyJPProxy_Type;
extern PyObject *PyJPValueBase_Type;
extern PyObject *PyJPValue_Type;
extern PyObject *PyJPValueLong_Type;
extern PyObject *PyJPValueFloat_Type;
extern PyObject *PyJPValueExc_Type;

struct PyJPContext
{
	PyObject_HEAD
	JPContext *m_Context;
	PyObject *m_Dict;
	PyObject *m_Classes;
} ;

struct PyJPClassHints
{
	PyObject_HEAD
	JPClassHints *m_Hints;
} ;

struct PyJPValue
{
	PyObject_HEAD
	JPValue m_Value;
	PyJPContext *m_Context;
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
	PyJPContext *m_Context;
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
	PyJPContext *m_Context;
} ;

struct PyJPProxy
{
	PyObject_HEAD
	JPProxy *m_Proxy;
	PyObject *m_Target;
	PyJPContext *m_Context;
} ;

inline JPContext* PyJPValue_getContext(PyJPValue* value)
{
	JPContext *context = value->m_Context->m_Context;
	if (context == NULL)
	{
		JP_RAISE_RUNTIME_ERROR("Context is null");
	}
	ASSERT_JVM_RUNNING(context);
	return context;
}
#define PyJPValue_GET_CONTEXT(X) PyJPValue_getContext((PyJPValue*)X)

namespace PyJPModule
{
	extern PyObject *module;
	/** Set a JPype Resource.
	 *
	 * JPype needs to know about a number of python objects to function
	 * properly. These resources are set in the initialization methods
	 * as those resources are created in python.
	 */
	PyObject* setResource(PyObject *obj, PyObject *args);
	extern PyInterpreterState *s_Interpreter;
}

bool PyJPContext_check(PyObject *o);

JPPyObject PyJPValue_create(PyTypeObject *type, JPContext *context, const JPValue& value);
JPPyObject PyJPArray_create(PyTypeObject *wrapper, JPContext *context, const JPValue& value);
JPPyObject PyJPClass_create(PyTypeObject *type, JPContext *context, JPClass *cls);
PyJPValue* PyJPValue_asValue(PyObject *self);

void PyJPMethod_initType(PyObject *module);
void PyJPProxy_initType(PyObject *module);
void PyJPContext_initType(PyObject *module);
void PyJPClassHints_initType(PyObject *module);
void PyJPValue_initType(PyObject* module);
void PyJPArray_initType(PyObject *module);
void PyJPClass_initType(PyObject *module);
void PyJPMonitor_initType(PyObject *module);
void PyJPField_initType(PyObject *module);

JPPyObject PyJPField_alloc(JPField *field);
JPPyObject PyJPMethod_create(JPMethodDispatch *m, PyObject *instance);

#endif /* PYJP_H */
