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
#ifndef _PYJP_VALUE_H_
#define _PYJP_VALUE_H_

extern PyObject *PyJPClassMeta_Type;

extern PyObject *PyJPValueBase_Type;
extern PyObject *PyJPValue_Type;
extern PyObject *PyJPValueInt_Type;
extern PyObject *PyJPValueExc_Type;

struct PyJPValue
{
	PyObject_HEAD
	JPValue m_Value;
	PyJPContext *m_Context;

	static JPPyObject create(PyTypeObject* wrapper, JPContext* context, JPClass* cls, jvalue value);
	static void initType(PyObject *module);
	static JPValue* getValue(PyObject *self);

	//	static JPPyObject alloc(PyTypeObject* wrapper, JPContext *context, JPClass *cls, jvalue value);
	//
	//	static PyTypeObject Type;
	//
	//	// Object A
	//	static PyObject* __new__(PyTypeObject *self, PyObject *args, PyObject *kwargs);
	//	static int __init__(PyJPValue *self, PyObject *args, PyObject *kwargs);
	//	static void __dealloc__(PyJPValue *self);
	//	static int traverse(PyJPValue *self, visitproc visit, void *arg);
	//	static int clear(PyJPValue *self);
	//
	//	static PyObject* __repr__(PyJPValue *self);
	//	static PyObject* toString(PyJPValue *self);
	//	static PyObject* toUnicode(PyJPValue *self);

} ;

#endif // _PYJP_VALUE_H_2
