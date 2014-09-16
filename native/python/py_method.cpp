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
#include <jpype_python.h>

static PyMethodDef methodMethods[] = {
  {"getName", &PyJPMethod::getName, METH_VARARGS, ""},
  {"isBeanAccessor", &PyJPMethod::isBeanAccessor, METH_VARARGS, ""},
  {"isBeanMutator", &PyJPMethod::isBeanMutator, METH_VARARGS, ""},
  {"matchReport", &PyJPMethod::matchReport, METH_VARARGS, ""},
	{NULL},
};

static PyTypeObject methodClassType = 
{
	PyObject_HEAD_INIT(&PyType_Type)
	0,                         /*ob_size*/
	"JavaMethod",              /*tp_name*/
	sizeof(PyJPMethod),      /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	PyJPMethod::__dealloc__,                   /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	PyJPMethod::__call__,                         /*tp_call*/
	PyJPMethod::__str__,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	"Java Method",                  /*tp_doc */
	0,		                   /* tp_traverse */
	0,		                   /* tp_clear */
	0,		                   /* tp_richcompare */
	0,		                   /* tp_weaklistoffset */
	0,		                   /* tp_iter */
	0,		                   /* tp_iternext */
	methodMethods,                   /* tp_methods */
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
void PyJPMethod::initType(PyObject* module)
{
	PyType_Ready(&methodClassType);
	PyModule_AddObject(module, "_JavaMethod", (PyObject*)&methodClassType); 
}

PyJPMethod* PyJPMethod::alloc(JPMethod* m)
{
	PyJPMethod* res = PyObject_New(PyJPMethod, &methodClassType);

	res->m_Method = m;
	
	return res;
}

PyObject* PyJPMethod::__call__(PyObject* o, PyObject* args, PyObject* kwargs)
{
	TRACE_IN("PyJPMethod::__call__");
	try {
		PyJPMethod* self = (PyJPMethod*)o;
		TRACE1(self->m_Method->getName());
		JPCleaner cleaner;

		//JPyHelper::dumpSequenceRefs(args, "start");

		vector<HostRef*> vargs;
		Py_ssize_t len = JPyObject::length(args);
		for (Py_ssize_t i = 0; i < len; i++)
		{
			PyObject* obj = JPySequence::getItem(args, i); // return a new ref
			HostRef* ref = new HostRef((void*)obj);
			cleaner.add(ref);
			vargs.push_back(ref);
			Py_DECREF(obj); // delete the new ref returned by getItem
		}

		//JPyHelper::dumpSequenceRefs(args, "middle");

		HostRef* res = self->m_Method->invoke(vargs);
	
		//JPyHelper::dumpSequenceRefs(args, "end");

		return detachRef(res);
	}
	PY_STANDARD_CATCH

	return NULL;

	TRACE_OUT;
}

void PyJPMethod::__dealloc__(PyObject* o)
{
	PyJPMethod* self = (PyJPMethod*)o;

	self->ob_type->tp_free(o);
}

PyObject* PyJPMethod::__str__(PyObject* o)
{
	PyJPMethod* self = (PyJPMethod*)o;
	stringstream sout;

	sout << "<method " << self->m_Method->getClassName() << "." << self->m_Method->getName() << ">";

	return JPyString::fromString(sout.str().c_str());
}

PyObject* PyJPMethod::isBeanAccessor(PyObject* o, PyObject* arg)
{
	try {
		PyJPMethod* self = (PyJPMethod*)o;

		bool res = self->m_Method->isBeanAccessor();
		if (res)
		{
			return JPyBoolean::getTrue();
		}
		return JPyBoolean::getFalse();
			
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPMethod::isBeanMutator(PyObject* o, PyObject* arg)
{
	try {
		PyJPMethod* self = (PyJPMethod*)o;

		bool res = self->m_Method->isBeanMutator();
		if (res)
		{
			return JPyBoolean::getTrue();
		}
		return JPyBoolean::getFalse();
			
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPMethod::getName(PyObject* o, PyObject* arg)
{
	try {
		PyJPMethod* self = (PyJPMethod*)o;

		string name = self->m_Method->getName();

		PyObject* res = JPyString::fromString(name.c_str());

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPMethod::matchReport(PyObject* o, PyObject* args)
{
	try {
		PyJPMethod* self = (PyJPMethod*)o;
		JPCleaner cleaner;

		vector<HostRef*> vargs;
		Py_ssize_t len = JPyObject::length(args);
		for (Py_ssize_t i = 0; i < len; i++)
		{
			PyObject* obj = JPySequence::getItem(args, i);
			HostRef* ref = new HostRef((void*)obj);
			cleaner.add(ref);
			vargs.push_back(ref);
			Py_DECREF(obj);
		}

		string report = self->m_Method->matchReport(vargs);

		PyObject* res = JPyString::fromString(report.c_str());

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

static PyMethodDef boundMethodMethods[] = {
  {"matchReport", &PyJPBoundMethod::matchReport, METH_VARARGS, ""},
	{NULL},
};

static PyTypeObject boundMethodClassType = 
{
	PyObject_HEAD_INIT(&PyType_Type)
	0,                         /*ob_size*/
	"JavaBoundMethod",              /*tp_name*/
	sizeof(PyJPBoundMethod),      /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	PyJPBoundMethod::__dealloc__,                   /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	PyJPBoundMethod::__call__,                         /*tp_call*/
	PyJPBoundMethod::__str__,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	"Java Bound Method",                  /*tp_doc */
	0,		                   /* tp_traverse */
	0,		                   /* tp_clear */
	0,		                   /* tp_richcompare */
	0,		                   /* tp_weaklistoffset */
	0,		                   /* tp_iter */
	0,		                   /* tp_iternext */
	boundMethodMethods,                   /* tp_methods */
	0,						   /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	PyJPBoundMethod::__init__,                         /* tp_init */
	0,                         /* tp_alloc */
	PyType_GenericNew          /* tp_new */

};

// Static methods
void PyJPBoundMethod::initType(PyObject* module)
{
	PyType_Ready(&boundMethodClassType);
	PyModule_AddObject(module, "_JavaBoundMethod", (PyObject*)&boundMethodClassType); 
}

int PyJPBoundMethod::__init__(PyObject* o, PyObject* args, PyObject* kwargs)
{
	try {
		PyJPBoundMethod* self = (PyJPBoundMethod*)o;

		PyObject* javaMethod;
		PyObject* inst;
		JPyArg::parseTuple(args, "OO", &javaMethod, &inst);

		Py_INCREF(inst);
		Py_INCREF(javaMethod);
		self->m_Instance = inst;
		self->m_Method = (PyJPMethod*)javaMethod;

		return 0;
	}
	PY_STANDARD_CATCH

	return 1;
}

PyObject* PyJPBoundMethod::__call__(PyObject* o, PyObject* args, PyObject* kwargs)
{
	TRACE_IN("PyJPBoundMethod::__call__");
	try {
		PyObject* result=NULL;
		{
			PyJPBoundMethod* self = (PyJPBoundMethod*)o;
			JPCleaner cleaner;
			TRACE1(self->m_Method->m_Method->getName());
	
			vector<HostRef*> vargs;
			Py_ssize_t len = JPyObject::length(args);
			HostRef* ref = new HostRef((void*)self->m_Instance);
			cleaner.add(ref);
			vargs.push_back(ref);
			for (Py_ssize_t i = 0; i < len; i++)
			{
				PyObject* obj = JPySequence::getItem(args, i); // returns a new ref
				ref = new HostRef((void*)obj);
				cleaner.add(ref);
				vargs.push_back(ref);
				Py_DECREF(obj); // remove ref returned by getItem
			}
	
			HostRef* res = self->m_Method->m_Method->invoke(vargs);
			TRACE2("Call finished, result = ", res);	
			
			result = detachRef(res);
			TRACE1("Cleaning up");
		}
		return result;
	}
	PY_STANDARD_CATCH

	return NULL;

	TRACE_OUT;
}

void PyJPBoundMethod::__dealloc__(PyObject* o)
{
	TRACE_IN("PyJPBoundMethod::__dealloc__");
	PyJPBoundMethod* self = (PyJPBoundMethod*)o;

	Py_DECREF(self->m_Instance);
	Py_DECREF(self->m_Method);

	self->ob_type->tp_free(o);
	TRACE1("Method freed");
	TRACE_OUT;
}

PyObject* PyJPBoundMethod::__str__(PyObject* o)
{
	PyJPBoundMethod* self = (PyJPBoundMethod*)o;
	stringstream sout;

	sout << "<bound method " << self->m_Method->m_Method->getClassName() << "." << self->m_Method->m_Method->getName() << ">";

	return JPyString::fromString(sout.str().c_str());
}

PyObject* PyJPBoundMethod::matchReport(PyObject* o, PyObject* args)
{

	try {
		PyJPBoundMethod* self = (PyJPBoundMethod*)o;

		cout << "Match report for " << self->m_Method->m_Method->getName() << endl;

		vector<HostRef*> vargs;
		Py_ssize_t len = JPyObject::length(args);
		for (Py_ssize_t i = 0; i < len; i++)
		{
			PyObject* obj = JPySequence::getItem(args, i);
			vargs.push_back(new HostRef((void*)obj));
			Py_DECREF(obj);
		}

		string report = self->m_Method->m_Method->matchReport(vargs);

		PyObject* res = JPyString::fromString(report.c_str());

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}
