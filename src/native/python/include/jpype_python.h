/*****************************************************************************
   Copyright 2004 Steve M�nard

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
*****************************************************************************/   
#ifndef _JPYPE_PYTHON_H_
#define _JPYPE_PYTHON_H_



// This file defines the _jpype module's interface and initializes it
#include <jpype.h>
#include <Python.h>

// TODO figure a better way to do this .. Python dependencies should not be in common code

#include <pyport.h>
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

#include "pythonenv.h"

class JPypeModule
{
public :
	static PyObject* startup(PyObject* obj, PyObject* args);  
	static PyObject* attach(PyObject* obj, PyObject* args);  
	static PyObject* dumpJVMStats(PyObject* obj);
	static PyObject* shutdown(PyObject* obj);
	static PyObject* synchronized(PyObject* obj, PyObject* args);
	static PyObject* isStarted(PyObject* obj);
	static PyObject* attachThread(PyObject* obj);
	static PyObject* detachThread(PyObject* obj);
	static PyObject* isThreadAttached(PyObject* obj);
	static PyObject* getJException(PyObject* obj, PyObject* args);
	static PyObject* raiseJava(PyObject* obj, PyObject* args);
	static PyObject* attachThreadAsDaemon(PyObject* obj);
	static PyObject* startReferenceQueue(PyObject* obj, PyObject* args);
	static PyObject* stopReferenceQueue(PyObject* obj);

	static PyObject* setConvertStringObjects(PyObject* obj, PyObject* args);
};

class JPypeJavaClass
{
public:
	static PyObject* findClass(PyObject* obj, PyObject* args);
	static PyObject* setJavaLangObjectClass(PyObject* self, PyObject* arg);
	static PyObject* setGetClassMethod(PyObject* self, PyObject* arg);
	static PyObject* setSpecialConstructorKey(PyObject* self, PyObject* arg);
};

class JPypeJavaProxy
{
public:
	static PyObject* setProxyClass(PyObject* self, PyObject* arg);
	static PyObject* createProxy(PyObject*, PyObject* arg);

};

class JPypeJavaArray
{
public:
	static PyObject* findArrayClass(PyObject* obj, PyObject* args);
	static PyObject* getArrayLength(PyObject* self, PyObject* arg);
	static PyObject* getArrayItem(PyObject* self, PyObject* arg);
	static PyObject* getArraySlice(PyObject* self, PyObject* arg);
	static PyObject* setArraySlice(PyObject* self, PyObject* arg);
	static PyObject* newArray(PyObject* self, PyObject* arg);
	static PyObject* setArrayItem(PyObject* self, PyObject* arg);
	static PyObject* setGetJavaArrayClassMethod(PyObject* self, PyObject* arg);
	static PyObject* setJavaArrayClass(PyObject* self, PyObject* arg);
	static PyObject* setArrayValues(PyObject* self, PyObject* arg);
};

class JPypeJavaNio
{
public:
	static PyObject* convertToDirectBuffer(PyObject* self, PyObject* arg);
};

class JPypeJavaWrapper
{
public:
	static PyObject* setWrapperClass(PyObject* self, PyObject* arg);
	static PyObject* setStringWrapperClass(PyObject* self, PyObject* arg);
};

class JPypeJavaException
{
public:
	static void errorOccurred();
};

#include "py_monitor.h"
#include "py_method.h"
#include "py_class.h"
#include "py_field.h"

#include "py_hostenv.h"

extern PythonHostEnvironment* hostEnv;

// Utility method
PyObject* detachRef(HostRef* ref);

#endif // _JPYPE_PYTHON_H_
