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

namespace JPypeModule
{
	PyObject* startup(PyObject* obj, PyObject* args);
	PyObject* attach(PyObject* obj, PyObject* args);
	PyObject* dumpJVMStats(PyObject* obj);
	PyObject* shutdown(PyObject* obj);
	PyObject* synchronized(PyObject* obj, PyObject* args);
	PyObject* isStarted(PyObject* obj);
	PyObject* attachThread(PyObject* obj);
	PyObject* detachThread(PyObject* obj);
	PyObject* isThreadAttached(PyObject* obj);
	PyObject* getJException(PyObject* obj, PyObject* args);
	PyObject* raiseJava(PyObject* obj, PyObject* args);
	PyObject* attachThreadAsDaemon(PyObject* obj);
	PyObject* startReferenceQueue(PyObject* obj, PyObject* args);
	PyObject* stopReferenceQueue(PyObject* obj);

	PyObject* setConvertStringObjects(PyObject* obj, PyObject* args);
}

namespace JPypeJavaClass
{
	PyObject* findClass(PyObject* obj, PyObject* args);
	PyObject* setJavaLangObjectClass(PyObject* self, PyObject* arg);
	PyObject* setGetClassMethod(PyObject* self, PyObject* arg);
	PyObject* setSpecialConstructorKey(PyObject* self, PyObject* arg);
};

namespace JPypeJavaProxy
{
	PyObject* setProxyClass(PyObject* self, PyObject* arg);
	PyObject* createProxy(PyObject*, PyObject* arg);
};

namespace JPypeJavaArray
{
	PyObject* findArrayClass(PyObject* obj, PyObject* args);
	PyObject* getArrayLength(PyObject* self, PyObject* arg);
	PyObject* getArrayItem(PyObject* self, PyObject* arg);
	PyObject* getArraySlice(PyObject* self, PyObject* arg);
	PyObject* setArraySlice(PyObject* self, PyObject* arg);
	PyObject* newArray(PyObject* self, PyObject* arg);
	PyObject* setArrayItem(PyObject* self, PyObject* arg);
	PyObject* setGetJavaArrayClassMethod(PyObject* self, PyObject* arg);
	PyObject* setJavaArrayClass(PyObject* self, PyObject* arg);
	PyObject* setArrayValues(PyObject* self, PyObject* arg);
};

namespace JPypeJavaNio
{
	PyObject* convertToDirectBuffer(PyObject* self, PyObject* arg);
};

namespace JPypeJavaWrapper
{
	PyObject* setWrapperClass(PyObject* self, PyObject* arg);
	PyObject* setStringWrapperClass(PyObject* self, PyObject* arg);
};

namespace JPypeJavaException
{
	void errorOccurred();
};

#include "py_monitor.h"
#include "py_method.h"
#include "py_class.h"
#include "py_field.h"

#include "py_hostenv.h"

#include "jpype_memory_view.h"

extern PythonHostEnvironment* hostEnv;

// Utility method
PyObject* detachRef(HostRef* ref);

#endif // _JPYPE_PYTHON_H_
