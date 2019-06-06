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
	static PyTypeObject Type;

	// Python-visible methods
	static void         initType(PyObject* module);

	/** Create a PyJPClass instance from within the module.
	 */
	static JPPyObject   alloc(JPClass* cls);

	/**
	 * Check if the Object is a PyJPClass
	 *  
	 * @param o
	 * @return true if the object is PyJPClass, otherwise false. 
	 */
	static bool   check(PyObject* o);

	static PyObject*  __new__(PyTypeObject* self, PyObject* args, PyObject* kwargs);
	static int __init__(PyJPClass* self, PyObject* args, PyObject* kwargs);
	static void __dealloc__(PyJPClass* o);

	/** Create a new instance of this class.
	 * 
	 * Operates on either object or array classes. 
	 */
	static PyObject* newInstance(PyJPClass* self, PyObject* arg);

	/** Get the java name for this class. */
	static PyObject* getCanonicalName(PyJPClass* self, PyObject* arg);

	static PyObject* getSuperClass(PyJPClass* self, PyObject* arg);
	static PyObject* getInterfaces(PyJPClass* self, PyObject* arg);
	static PyObject* getClassMethods(PyJPClass* self, PyObject* arg);
	static PyObject* getClassFields(PyJPClass* self, PyObject* arg);

	static PyObject* isInterface(PyJPClass* self, PyObject* arg);
	static PyObject* isPrimitive(PyJPClass* self, PyObject* arg);
	static PyObject* isThrowable(PyJPClass* self, PyObject* arg);
	static PyObject* isArray(PyJPClass* self, PyObject* arg);
	static PyObject* isAbstract(PyJPClass* self, PyObject* arg);
	static PyObject* isAssignableFrom(PyJPClass* self, PyObject* arg);

	/** Create an new PyJPValue with this class as the object. */
	static PyObject* asJavaValue(PyJPClass* self, PyObject* arg);

	/** For diagnostics */
	static PyObject* canConvertToJava(PyJPClass* self, PyObject* args);
	static PyObject* convertToJava(PyJPClass* self, PyObject* args);
    static PyObject* dumpCtor(PyJPClass* self, PyObject* args);

	JPClass* m_Class;
	PyJPContext* m_Context;
} ;

#endif // _PYCLASS_H_
