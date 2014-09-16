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
#include <jpype_python.h>

static PyMethodDef methods[] = {
	{NULL},
};

static PyTypeObject monitorClassType = 
{
	PyObject_HEAD_INIT(&PyType_Type)
	0,                         /*ob_size*/
	"JavaMonitor",              /*tp_name*/
	sizeof(PyJPMonitor),      /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	PyJPMonitor::__dealloc__,                   /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	PyJPMonitor::__str__,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	"Java Monitor",                  /*tp_doc */
	0,		                   /* tp_traverse */
	0,		                   /* tp_clear */
	0,		                   /* tp_richcompare */
	0,		                   /* tp_weaklistoffset */
	0,		                   /* tp_iter */
	0,		                   /* tp_iternext */
	methods,                   /* tp_methods */
	0,						   /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	0,                         /* tp_init */
	0,                         /* tp_alloc */
	PyType_GenericNew          /* tp_new */

};

// Static methods
void PyJPMonitor::initType(PyObject* module)
{
	PyType_Ready(&monitorClassType);
}

PyJPMonitor* PyJPMonitor::alloc(JPMonitor* o)
{
	PyJPMonitor* res = PyObject_New(PyJPMonitor, &monitorClassType);

	res->state = o;
	
	return res;
}

void PyJPMonitor::__dealloc__(PyObject* o)
{
	PyJPMonitor* self = (PyJPMonitor*)o;

	delete self->state;

	self->ob_type->tp_free(o);
}

PyObject* PyJPMonitor::__str__(PyObject* o)
{
	// TODO
	JPyErr::setString(PyExc_RuntimeError, "__str__ Not implemented");
	return NULL;
}

