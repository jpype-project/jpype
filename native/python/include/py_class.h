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
#ifndef _PYCLASS_H_
#define _PYCLASS_H_

struct PyJPClass
{
	//AT's comments on porting:
	//  1) Some Unix compilers do not tolerate the semicolumn after PyObject_HEAD	
	//PyObject_HEAD;
	PyObject_HEAD
	
	// Python-visible methods
	static void         initType(PyObject* module);
	static PyJPClass*   alloc(JPClass* cls);
	static bool   check(PyObject* o);

	static void        __dealloc__(PyObject* o);

	static PyObject* getName(PyObject* self, PyObject* arg);
	static PyObject* getBaseClass(PyObject* self, PyObject* arg);
	static PyObject* getBaseInterfaces(PyObject* self, PyObject* arg);
	static PyObject* getClassMethods(PyObject* self, PyObject* arg);
	static PyObject* getClassFields(PyObject* self, PyObject* arg);
	static PyObject* newClassInstance(PyObject* self, PyObject* arg);
	static PyObject* isInterface(PyObject* self, PyObject* arg);
	static PyObject* isPrimitive(PyObject* self, PyObject* arg);
	static PyObject* isSubclass(PyObject* self, PyObject* arg);
	static PyObject* isException(PyObject* self, PyObject* arg);
	static PyObject* isArray(PyObject* self, PyObject* arg);

	static PyObject* getConstructors(PyObject* self);
	static PyObject* getDeclaredConstructors(PyObject* self);
	static PyObject* getDeclaredFields(PyObject* self);
	static PyObject* getDeclaredMethods(PyObject* self);
	static PyObject* getFields(PyObject* self);
	static PyObject* getMethods(PyObject* self);
	static PyObject* getModifiers(PyObject* self);

	JPClass* m_Class;
};

#endif // _PYCLASS_H_
