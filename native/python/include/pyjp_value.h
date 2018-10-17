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

struct PyJPValue
{
	PyObject_HEAD

	static JPPyObject alloc(const JPValue& value);
	static JPPyObject alloc(JPClass* cls, jvalue value);

	static PyTypeObject Type;
	static void        initType(PyObject* module);
	static bool        check(PyObject* o);

	// Object A
	static PyObject*   __new__(PyTypeObject* self, PyObject* args, PyObject* kwargs);
	static int         __init__(PyJPValue* self, PyObject* args, PyObject* kwargs);
	static void        __dealloc__(PyJPValue* self);
	static PyObject*   __str__(PyJPValue* self);
	static PyObject*   toString(PyJPValue* self);
	static PyObject*   toUnicode(PyJPValue* self);

	JPValue m_Value;
	PyObject* m_Cache;
} ;

#endif // _PYJP_VALUE_H_2
