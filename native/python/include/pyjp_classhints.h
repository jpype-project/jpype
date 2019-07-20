
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
#ifndef _PYJPCLASSHINTS_H_
#define _PYJPCLASSHINTS_H_

struct PyJPClassHints
{
	PyObject_HEAD

	static PyTypeObject Type;
	static void initType(PyObject *module);
	static bool check(PyObject *o);

	static PyObject * __new__(PyTypeObject *self, PyObject *args, PyObject *kwargs);
	static int __init__(PyJPClassHints *self, PyObject *args, PyObject *kwargs);
	static void __dealloc__(PyJPClassHints *self);
	static PyObject *__str__(PyJPClassHints *self);
//	static int traverse(PyJPClassHints *self, visitproc visit, void *arg);
//	static int clear(PyJPClassHints *self);
	
	JPClassHints *m_Hints;

} ;

#endif // _PYJPCLASSHINTS_H_
