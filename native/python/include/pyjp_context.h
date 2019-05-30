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
#ifndef _PYJP_CONTEXT_H_
#define _PYJP_CONTEXT_H_

struct PyJPContext
{
	PyObject_HEAD

	static PyTypeObject Type;
	static void        initType(PyObject* module);
	static bool        check(PyObject* o);

	// Object A
	static PyObject*   __new__(PyTypeObject* self, PyObject* args, PyObject* kwargs);
	static int         __init__(PyJPContext* self, PyObject* args, PyObject* kwargs);
	static void        __dealloc__(PyJPContext* self);
	static PyObject*   __str__(PyJPContext* self);
//	static PyObject*   toString(PyJPContext* self);
//	static PyObject*   toUnicode(PyJPContext* self);
	
	PyObject* startup(PyJPContext* obj, PyObject* args);
	PyObject* shutdown(PyJPContext* obj);
	PyObject* isStarted(PyJPContext* obj);
	PyObject* attachThread(PyJPContext* obj);
	PyObject* detachThread(PyJPContext* obj);
	PyObject* isThreadAttached(PyJPContext* obj);
	PyObject* attachThreadAsDaemon(PyJPContext* obj);

	/** Memory map a byte buffer between java and python, so 
	 * that both have direct access.  This is mainly used for io classes.
	 */
	PyObject* convertToDirectByteBuffer(PyJPContext* self, PyObject* args);

	JPContext* m_Context;
} ;

#endif // _PYJP_CONTEXT_H_2
