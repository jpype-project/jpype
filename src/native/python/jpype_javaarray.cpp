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

namespace { // impl detail
	inline bool is_primitive(char t) {
		switch(t) {
			case 'B': case 'S': case 'I': case 'J': case 'F': case 'D': case 'Z': case 'C':
				return true;
			default:
				return false;
		}
	}
}

PyObject* JPypeJavaArray::findArrayClass(PyObject* obj, PyObject* args)
{
	try {
		char* cname;
		JPyArg::parseTuple(args, "s", &cname);

		JPTypeName name = JPTypeName::fromSimple(cname);
		JPArrayClass* claz = JPTypeManager::findArrayClass(name);
		if (claz == NULL)
		{
			Py_RETURN_NONE;
		}

		PyObject* res = JPyCObject::fromVoidAndDesc((void*)claz, (void*)"jclass", NULL);

		return res;
	}
	PY_STANDARD_CATCH;

	PyErr_Clear();

	Py_RETURN_NONE;
}


PyObject* JPypeJavaArray::setJavaArrayClass(PyObject* self, PyObject* arg)
{
	try {
		PyObject* t;
		JPyArg::parseTuple(arg, "O", &t);
		hostEnv->setJavaArrayClass(t);

		Py_RETURN_NONE;
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
	PyObject* arrayObject;
	int lo = -1;
	int hi = -1;
	try
	{

		JPyArg::parseTuple(arg, "O!ii", &PyCObject_Type, &arrayObject, &lo, &hi);
		JPArray* a = (JPArray*)JPyCObject::asVoidPtr(arrayObject);
		int length = a->getLength();
		// stolen from jcc, to get nice slice support
		if (lo < 0) lo = length + lo;
		if (lo < 0) lo = 0;
		else if (lo > length) lo = length;
		if (hi < 0) hi = length + hi;
		if (hi < 0) hi = 0;
		else if (hi > length) hi = length;
		if (lo > hi) lo = hi;

		const string& name = a->getType()->getObjectType().getComponentName().getNativeName();
		if(is_primitive(name[0]))
		{
			// for primitive types, we have fast sequence generation available
			return a->getSequenceFromRange(lo, hi);
		}
		else
		{
			// slow wrapped access for non primitives
			vector<HostRef*> values = a->getRange(lo, hi);

			JPCleaner cleaner;
			PyObject* res = JPySequence::newList((int)values.size());
			for (unsigned int i = 0; i < values.size(); i++)
			{
				JPySequence::setItem(res, i, (PyObject*)values[i]->data());
				cleaner.add(values[i]);
			}

			return res;
		}
	} PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::setArraySlice(PyObject* self, PyObject* arg)
{

	PyObject* arrayObject;
	int lo = -1;
	int hi = -1;
	PyObject* sequence;
	try {
		JPyArg::parseTuple(arg, "O!iiO", &PyCObject_Type, &arrayObject, &lo, &hi, &sequence);
		JPArray* a = (JPArray*)JPyCObject::asVoidPtr(arrayObject);

		Py_ssize_t length = a->getLength();
		if(length == 0)
			Py_RETURN_NONE;

		if (lo < 0) lo = length + lo;
		if (lo < 0) lo = 0;
		else if (lo > length) lo = length;
		if (hi < 0) hi = length + hi;
		if (hi < 0) hi = 0;
		else if (hi > length) hi = length;
		if (lo > hi) lo = hi;

		const string& name = a->getType()->getObjectType().getComponentName().getNativeName();

		if(is_primitive(name[0]))
		{
			// for primitive types, we have fast setters available
			a->setRange(lo, hi, sequence);
		}
		else
		{
			// slow wrapped access for non primitive types
			vector<HostRef*> values;
			values.reserve(hi - lo);
			JPCleaner cleaner;
			for (Py_ssize_t i = 0; i < hi - lo; i++)
			{
				HostRef* v = new HostRef(JPySequence::getItem(sequence, i), false);
				values.push_back(v);
				cleaner.add(v);
			}

			a->setRange(lo, hi, values);
		}

		Py_RETURN_NONE;
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
		Py_RETURN_NONE;
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
