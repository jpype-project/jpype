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
#ifndef _PYJP_ARRAY_H_
#define _PYJP_ARRAY_H_

/** This is a wrapper for accessing the array method.  It is structured to
 * be like a bound method.  It should not be the primary handle to the object.
 * That will be a PyJPValue.
 */
struct PyJPArray
{
	PyObject_HEAD

	static JPPyObject alloc(JPArray* value);

	static PyTypeObject Type;
	static void        initType(PyObject* module);
	static bool        check(PyObject* o);

	// Object A
	static PyObject*   __new__(PyTypeObject* self, PyObject* args, PyObject* kwargs);
	static int         __init__(PyJPArray* self, PyObject* args, PyObject* kwargs);
	static void        __dealloc__(PyJPArray* self);
	static PyObject*   __str__(PyJPArray* self);

	// Python-visible methods
	static PyObject* getArrayLength(PyJPArray* self, PyObject* arg);
	static PyObject* getArrayItem(PyJPArray* self, PyObject* arg);
	static PyObject* setArrayItem(PyJPArray* self, PyObject* arg);
	static PyObject* getArraySlice(PyJPArray* self, PyObject* arg);
	static PyObject* setArraySlice(PyJPArray* self, PyObject* arg);

	JPArray* m_Array;
	PyJPContext* m_Context;
} ;

#endif // _PYJP_ARRAY_H_
