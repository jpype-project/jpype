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
#ifndef _PYFIELD_H_
#define _PYFIELD_H_

struct PyJPField
{
	PyObject_HEAD
	
	// Python-visible methods
	static void         initType(PyObject* module);
	static PyJPField*   alloc(JPField* mth);

	static void         __dealloc__(PyObject* o);
	static PyObject*    getName(PyObject* self, PyObject* arg);
	static PyObject* getStaticAttribute(PyObject* self, PyObject* arg);
	static PyObject* setStaticAttribute(PyObject* self, PyObject* arg);
	static PyObject* setInstanceAttribute(PyObject* self, PyObject* arg);
	static PyObject* getInstanceAttribute(PyObject* self, PyObject* arg);
	static PyObject* isStatic(PyObject* self, PyObject* arg);
	static PyObject* isFinal(PyObject* self, PyObject* arg);


	JPField* m_Field;
};

#endif // _PYFIELD_H_
