/*****************************************************************************
   Copyright 2004 Steve Ménard

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
#ifndef _PYMETHOD_H_
#define _PYMETHOD_H_

struct PyJPMethod
{
	PyObject_HEAD
	
	// Python-visible methods
	static void         initType(PyObject* module);
	static PyJPMethod*  alloc(JPMethod* mth);

	static void        __dealloc__(PyObject* o);
	static PyObject*   __str__(PyObject* o);
	static PyObject*   __call__(PyObject* self, PyObject* args, PyObject* kwargs);
	static PyObject*   isBeanMutator(PyObject* self, PyObject* arg);
	static PyObject*   isBeanAccessor(PyObject* self, PyObject* arg);
	static PyObject*   getName(PyObject* self, PyObject* arg);
	static PyObject*   matchReport(PyObject* self, PyObject* arg);

	JPMethod* m_Method;
};

struct PyJPBoundMethod
{
	PyObject_HEAD
	
	// Python-visible methods
	static void         initType(PyObject* module);

	static int         __init__(PyObject* self, PyObject* args, PyObject* kwargs);
	static void        __dealloc__(PyObject* o);
	static PyObject*   __str__(PyObject* o);
	static PyObject*   __call__(PyObject* self, PyObject* args, PyObject* kwargs);
	static PyObject*   matchReport(PyObject* self, PyObject* arg);
		
	PyObject* m_Instance;
	PyJPMethod* m_Method;
};

#endif // _PYMETHOD_H_
