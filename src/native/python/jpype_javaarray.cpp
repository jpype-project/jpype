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
PyObject* JPypeJavaArray::findArrayClass(PyObject* obj, PyObject* args)
{
	try {
		char* cname;
		JPyArg::parseTuple(args, "s", &cname);

		JPTypeName name = JPTypeName::fromSimple(cname);
		JPArrayClass* claz = JPTypeManager::findArrayClass(name);
		if (claz == NULL)
		{
			Py_INCREF(Py_None);
			return Py_None;
		}

		PyObject* res = JPyCObject::fromVoidAndDesc((void*)claz, (void*)"jclass", NULL);

		return res;
	}
	PY_STANDARD_CATCH;

	PyErr_Clear();

	Py_INCREF(Py_None);
	return Py_None;
}


PyObject* JPypeJavaArray::setJavaArrayClass(PyObject* self, PyObject* arg)
{
	try {
		PyObject* t;
		JPyArg::parseTuple(arg, "O", &t);
		hostEnv->setJavaArrayClass(t);

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::setGetJavaArrayClassMethod(PyObject* self, PyObject* arg)
{
	try {
		PyObject* t;
		JPyArg::parseTuple(arg, "O", &t);
		hostEnv->setGetJavaArrayClassMethod(t);


		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::getArrayLength(PyObject* self, PyObject* arg)
{
	try {
		PyObject* arrayObject;
		JPyArg::parseTuple(arg, "O!", &PyCObject_Type, &arrayObject);
		JPArray* a = (JPArray*)JPyCObject::asVoidPtr(arrayObject);

		int res = a->getLength();
		return JPyInt::fromLong(res);

	
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::getArrayItem(PyObject* self, PyObject* arg)
{
	try {
		PyObject* arrayObject;
		int ndx;
		JPyArg::parseTuple(arg, "O!i", &PyCObject_Type, &arrayObject, &ndx);
		JPArray* a = (JPArray*)JPyCObject::asVoidPtr(arrayObject);

		HostRef* res = a->getItem(ndx);
		return detachRef(res);
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::getArraySlice(PyObject* self, PyObject* arg)
{
	try {
		PyObject* arrayObject;
		int ndx;
		int ndx2;
		JPyArg::parseTuple(arg, "O!ii", &PyCObject_Type, &arrayObject, &ndx, &ndx2);
		JPArray* a = (JPArray*)JPyCObject::asVoidPtr(arrayObject);

		vector<HostRef*> values = a->getRange(ndx, ndx2);

		JPCleaner cleaner;
		PyObject* res = JPySequence::newList((int)values.size());
		for (unsigned int i = 0; i < values.size(); i++)
		{
			JPySequence::setItem(res, i, (PyObject*)values[i]->data());
			cleaner.add(values[i]);
		}

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::setArraySlice(PyObject* self, PyObject* arg)
{
	try {
		PyObject* arrayObject;
		int ndx = -1;
		int ndx2 = -1;
		PyObject* val;
		JPyArg::parseTuple(arg, "O!iiO", &PyCObject_Type, &arrayObject, &ndx, &ndx2, &val);
		JPArray* a = (JPArray*)JPyCObject::asVoidPtr(arrayObject);

		Py_ssize_t len = JPyObject::length(val);
		vector<HostRef*> values;
		JPCleaner cleaner;
		for (Py_ssize_t i = 0; i < len; i++)
		{
			HostRef* v = new HostRef(JPySequence::getItem(val, i), false);
			values.push_back(v);
			cleaner.add(v);
		}

		a->setRange(ndx, ndx2, values);

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::setArrayItem(PyObject* self, PyObject* arg)
{
	try {
		PyObject* arrayObject;
		int ndx;
		PyObject* value;
		JPyArg::parseTuple(arg, "O!iO", &PyCObject_Type, &arrayObject, &ndx, &value);
		JPArray* a = (JPArray*)JPyCObject::asVoidPtr(arrayObject);

		JPCleaner cleaner;
		HostRef* v = new HostRef(value);
		cleaner.add(v);

		a->setItem(ndx, v);
		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::newArray(PyObject* self, PyObject* arg)
{
	try {
		PyObject* arrayObject;
		int sz;
		JPyArg::parseTuple(arg, "O!i", &PyCObject_Type, &arrayObject, &sz);
		JPArrayClass* a = (JPArrayClass*)JPyCObject::asVoidPtr(arrayObject);

		JPArray* v = a->newInstance(sz);
		PyObject* res = JPyCObject::fromVoidAndDesc(v, (void*)"JPArray", PythonHostEnvironment::deleteJPArrayDestructor);

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::setArrayValues(PyObject* self, PyObject* arg)
{
	try {
		PyObject* arrayObject;
		PyObject* values;
		JPyArg::parseTuple(arg, "O!O", &PyCObject_Type, &arrayObject, &values);
		JPArray* a = (JPArray*)JPyCObject::asVoidPtr(arrayObject);
		JPArrayClass* arrayClass = a->getClass();
		HostRef valuesRef(values);
		arrayClass->getComponentType()->setArrayValues((jarray)a->getObject(), &valuesRef);
		
		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}
