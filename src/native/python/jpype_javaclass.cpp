/*****************************************************************************
   Copyright 2004-2008 Steve Menard

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

PyObject* JPypeJavaClass::setJavaLangObjectClass(PyObject* self, PyObject* arg)
{
	try {
		PyObject* t;
		JPyArg::parseTuple(arg, "O", &t);
		hostEnv->setJavaLangObjectClass(t);


		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaClass::setGetClassMethod(PyObject* self, PyObject* arg)
{
	try {
		PyObject* t;
		JPyArg::parseTuple(arg, "O", &t);
		hostEnv->setGetJavaClassMethod(t);


		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}


PyObject* JPypeJavaClass::setSpecialConstructorKey(PyObject* self, PyObject* arg)
{
	try {
		PyObject* t;
		JPyArg::parseTuple(arg, "O", &t);
		hostEnv->setSpecialConstructorKey(t);


		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaClass::findClass(PyObject* obj, PyObject* args)
{
	TRACE_IN("JPypeModule::findClass");
	try {
		char* cname;
		JPyArg::parseTuple(args, "s", &cname);
		TRACE1(cname);

		JPTypeName name = JPTypeName::fromSimple(cname);

		JPClass* claz = JPTypeManager::findClass(name);
		if (claz == NULL)
		{
			Py_INCREF(Py_None);
			return Py_None;
		}

		PyObject* res = (PyObject*)PyJPClass::alloc(claz);

		return res;
	}
	PY_STANDARD_CATCH;  

	PyErr_Clear();

	Py_INCREF(Py_None);
	return Py_None;

	TRACE_OUT;
}


