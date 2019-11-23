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

extern PyObject *PyJPMethod_Type;
struct PyJPMethod
{
	PyFunctionObject func;
	PyJPContext *m_Context;
	JPMethodDispatch* m_Method;
	PyObject* m_Instance;
	PyObject* m_Doc;
	PyObject* m_Annotations;
	PyObject* m_CodeRep;

	// Python-visible methods
	static void initType(PyObject *module);
	static JPPyObject alloc(JPMethodDispatch *mth, PyObject *obj);
} ;

#endif // _PYMETHOD_H_
