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
#ifndef _PYMONITOR_H_
#define _PYMONITOR_H_

#include "object.h"

/** Python object to support jpype.synchronized(object) command.
 */
struct PyJPMonitor
{
	PyObject_HEAD

	static PyTypeObject Type;

	// Python-visible methods
	static void       initType(PyObject* module);
	static int        __init__(PyJPMonitor* self, PyObject* args);
	static void       __dealloc__(PyJPMonitor* o);
	static PyObject*  __str__(PyJPMonitor* o);
	static PyObject*  __enter__(PyJPMonitor* self, PyObject* args);
	static PyObject*  __exit__(PyJPMonitor* self, PyObject* args);

	JPMonitor* m_Monitor;
	PyJPContext* m_Context;
} ;

#endif // _PYMONITOR_H_
