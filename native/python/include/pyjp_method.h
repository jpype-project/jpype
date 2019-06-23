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
#include <pyjp.h>

struct PyJPMethod
{
	PyObject_HEAD

	static PyTypeObject Type;

	// Python-visible methods
	static void       initType(PyObject *module);
	static JPPyObject alloc(JPMethodDispatch *mth, PyObject *obj);

	static PyObject*  __new__(PyTypeObject *self, PyObject *args, PyObject *kwargs);
	static void       __dealloc__(PyJPMethod *o);
	static int        traverse(PyJPMethod *self, visitproc visit, void *arg);
	static int        clear(PyJPMethod *self);

	static PyObject*  __get__(PyJPMethod *self, PyObject *obj, PyObject *type);
	static PyObject*  __str__(PyJPMethod *o);
	static PyObject*  __repr__(PyJPMethod *self);
	static PyObject*  __call__(PyJPMethod *self, PyObject *args, PyObject *kwargs);

	static PyObject*  isBeanMutator(PyJPMethod *self, PyObject *arg);
	static PyObject*  isBeanAccessor(PyJPMethod *self, PyObject *arg);
	static PyObject*  getName(PyJPMethod *self, PyObject *arg);
	static PyObject*  matchReport(PyJPMethod *self, PyObject *arg);
	static PyObject*  dump(PyJPMethod *self, PyObject *arg);
	static PyObject*  __doc__(PyJPMethod *method, void *context);

	JPMethodDispatch *m_Method;
	PyObject *m_Instance;
	PyJPContext *m_Context;

} ;

#endif // _PYMETHOD_H_
