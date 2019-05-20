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
#ifndef _PYFIELD_H_
#define _PYFIELD_H_

struct PyJPField
{
	PyObject_HEAD

	static PyTypeObject Type;

	// Python-visible methods
	static void       initType(PyObject* module);
	static JPPyObject alloc(JPField* mth);

	static void       __dealloc__(PyJPField* o);
	static PyObject*  getName(PyJPField* self, PyObject* arg);
	static PyObject*  __get__(PyJPField* self, PyObject* obj, PyObject* type);
	static int        __set__(PyJPField* self, PyObject* obj, PyObject* val);
	static PyObject*  isStatic(PyJPField* self, PyObject* arg);
	static PyObject*  isFinal(PyJPField* self, PyObject* arg);

	JPField* m_Field;
} ;

#endif // _PYFIELD_H_
