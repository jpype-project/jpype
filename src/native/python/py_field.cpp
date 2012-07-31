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

static PyMethodDef fieldMethods[] = {
  {"getName", &PyJPField::getName, METH_VARARGS, ""},
  {"isFinal", &PyJPField::isFinal, METH_VARARGS, ""},
  {"isStatic", &PyJPField::isStatic, METH_VARARGS, ""},
  {"getStaticAttribute",       &PyJPField::getStaticAttribute, METH_VARARGS, ""},
  {"setStaticAttribute",       &PyJPField::setStaticAttribute, METH_VARARGS, ""},
  {"getInstanceAttribute",     &PyJPField::getInstanceAttribute, METH_VARARGS, ""},
  {"setInstanceAttribute",     &PyJPField::setInstanceAttribute, METH_VARARGS, ""},
	{NULL},
};

static PyTypeObject fieldClassType = 
{
	PyObject_HEAD_INIT(&PyType_Type)
	0,                         /*ob_size*/
	"JavaField",              /*tp_name*/
	sizeof(PyJPField),      /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	PyJPField::__dealloc__,                   /*tp_dealloc*/
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
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	"Java Field",                  /*tp_doc */
	0,		                   /* tp_traverse */
	0,		                   /* tp_clear */
	0,		                   /* tp_richcompare */
	0,		                   /* tp_weaklistoffset */
	0,		                   /* tp_iter */
	0,		                   /* tp_iternext */
	fieldMethods,                   /* tp_methods */
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
void PyJPField::initType(PyObject* module)
{
	PyType_Ready(&fieldClassType);
	PyModule_AddObject(module, "_JavaField", (PyObject*)&fieldClassType); 
}

PyJPField* PyJPField::alloc(JPField* m)
{
	PyJPField* res = PyObject_New(PyJPField, &fieldClassType);
	
	res->m_Field = m;
	
	return res;
}

void PyJPField::__dealloc__(PyObject* o)
{
	PyJPField* self = (PyJPField*)o;

	self->ob_type->tp_free(o);
}

PyObject* PyJPField::getName(PyObject* o, PyObject* arg)
{
	try {
		PyJPField* self = (PyJPField*)o;

		string name = self->m_Field->getName();

		PyObject* res = JPyString::fromString(name.c_str());

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPField::getStaticAttribute(PyObject* o, PyObject* arg)
{
	try {
		PyJPField* self = (PyJPField*)o;

		HostRef* res = self->m_Field->getStaticAttribute();
		PyObject* result = detachRef(res);
		return result;


	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPField::setStaticAttribute(PyObject* o, PyObject* arg)
{
	try {
		PyJPField* self = (PyJPField*)o;

		PyObject* value;
		JPyArg::parseTuple(arg, "O", &value);

		HostRef v(value);

		self->m_Field->setStaticAttribute(&v);

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPField::setInstanceAttribute(PyObject* o, PyObject* arg)
{
	JPCleaner cleaner;
	try {
		PyJPField* self = (PyJPField*)o;

		PyObject* jo;
		PyObject* value;
		JPyArg::parseTuple(arg, "O!O", &PyCObject_Type, &jo, &value);

		JPObject* obj = (JPObject*)JPyCObject::asVoidPtr(jo);

		HostRef* ref = new HostRef(value);
		cleaner.add(ref);
		
		jobject jobj = obj->getObject();
		cleaner.addLocal(jobj);

		self->m_Field->setAttribute(jobj, ref);

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPField::getInstanceAttribute(PyObject* o, PyObject* arg)
{
	TRACE_IN("getInstanceAttribute");

	JPCleaner cleaner;
	try {
		PyJPField* self = (PyJPField*)o;

		PyObject* jo;
		JPyArg::parseTuple(arg, "O!", &PyCObject_Type, &jo);

		JPObject* obj = (JPObject*)JPyCObject::asVoidPtr(jo);

		jobject jobj = obj->getObject();
		cleaner.addLocal(jobj);

		HostRef* res = self->m_Field->getAttribute(jobj);
		return detachRef(res);
	}
	PY_STANDARD_CATCH

	return NULL;

	TRACE_OUT;
}

PyObject* PyJPField::isStatic(PyObject* o, PyObject* arg)
{
	JPCleaner cleaner;
	try {
		PyJPField* self = (PyJPField*)o;

		if (self->m_Field->isStatic())
		{
			return JPyBoolean::getTrue();
		}
		return JPyBoolean::getFalse();
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPField::isFinal(PyObject* o, PyObject* arg)
{
	JPCleaner cleaner;
	try {
		PyJPField* self = (PyJPField*)o;

		if (self->m_Field->isFinal())
		{
			return JPyBoolean::getTrue();
		}
		return JPyBoolean::getFalse();
	}
	PY_STANDARD_CATCH

	return NULL;
}
