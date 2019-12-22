/*****************************************************************************
   Copyright 2019 nelson85

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
#include <pyjp.h>
#include <structmember.h>

#ifdef __cplusplus
extern "C"
{
#endif

PyObject* PyJPException_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPException_new");
	return ((PyTypeObject*) PyExc_BaseException)->tp_new(type, args, kwargs);
	JP_PY_CATCH(NULL);
}

int PyJPException_init(PyObject *self, PyObject *pyargs, PyObject *kwargs)
{
	// This method is here to ensure that we hit the right area of the mro
	JP_PY_TRY("PyJPException_init", self);
	if (PyJPValueBase_init(self, pyargs, kwargs) == -1)
		return -1;
	return ((PyTypeObject*) PyExc_BaseException)->tp_init(self, pyargs, kwargs);
	JP_PY_CATCH(-1);
}

void PyJPException_dealloc(PyObject *self)
{
	JP_PY_TRY("PyJPException_dealloc", self);
	((PyTypeObject*) PyExc_BaseException)->tp_dealloc(self);
	JP_PY_CATCH();
}

int PyJPException_traverse(PyObject *self, visitproc visit, void *arg)
{
	JP_PY_TRY("PyJPException_traverse", self);
	return ((PyTypeObject*) PyExc_BaseException)->tp_traverse(self, visit, arg);
	JP_PY_CATCH(-1);
}

int PyJPException_clear(PyObject *self)
{
	JP_PY_TRY("PyJPException_clear", self);
	return ((PyTypeObject*) PyExc_BaseException)->tp_clear(self);
	JP_PY_CATCH(-1);
}

static PyType_Slot valueExcSlots[] = {
	{ Py_tp_new,      (void*) PyJPException_new},
	{ Py_tp_init,     (void*) PyJPException_init},
	{ Py_tp_dealloc,  (void*) PyJPException_dealloc},
	{ Py_tp_traverse, (void*) PyJPException_traverse},
	{ Py_tp_clear,    (void*) PyJPException_clear},
	{0}
};

PyType_Spec PyJPExceptionSpec = {
	"_jpype.PyJPException",
	0, // sizeof (PyBaseExceptionObject),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	valueExcSlots
};

#ifdef __cplusplus
}
#endif