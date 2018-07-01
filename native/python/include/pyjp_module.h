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
#ifndef PYJP_MODULE_H
#define PYJP_MODULE_H

namespace PyJPModule
{
	PyObject* startup(PyObject* obj, PyObject* args);
	PyObject* attach(PyObject* obj, PyObject* args);
	PyObject* dumpJVMStats(PyObject* obj);
	PyObject* shutdown(PyObject* obj);
	PyObject* isStarted(PyObject* obj);
	PyObject* attachThread(PyObject* obj);
	PyObject* detachThread(PyObject* obj);
	PyObject* isThreadAttached(PyObject* obj);
	PyObject* getJException(PyObject* obj, PyObject* args);
	PyObject* attachThreadAsDaemon(PyObject* obj);

	/** Set a JPype Resource.
	 *
	 * JPype needs to know about a number of python objects to function
	 * properly. These resources are set in the initialization methods
	 * as those resources are created in python. 
	 */
	PyObject* setResource(PyObject* obj, PyObject* args);

	/** Memory map a byte buffer betten java and python, so 
	 * that both have direct access.  This is mainly used for io classes.
	 */
	PyObject* convertToDirectByteBuffer(PyObject* self, PyObject* args);
}

#endif /* PYJP_MODULE_H */

