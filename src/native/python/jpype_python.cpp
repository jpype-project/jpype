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

PythonHostEnvironment* hostEnv;

PyObject* JPypeJavaWrapper::setWrapperClass(PyObject* self, PyObject* arg)
{
	try {
		PyObject* t;
		JPyArg::parseTuple(arg, "O", &t);
		hostEnv->setWrapperClass(t);

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaWrapper::setStringWrapperClass(PyObject* self, PyObject* arg)
{
	try {
		PyObject* t;
		JPyArg::parseTuple(arg, "O", &t);
		hostEnv->setStringWrapperClass(t);

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}


PyObject* JPypeJavaProxy::setProxyClass(PyObject* self, PyObject* arg)
{
	try {
		PyObject* t;
		JPyArg::parseTuple(arg, "O", &t);
		hostEnv->setProxyClass(t);

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* convertToJValue(PyObject* self, PyObject* arg)
{
	try {
		char* tname;
		PyObject* value;

		JPyArg::parseTuple(arg, "sO", &tname, &value);

		JPTypeName name = JPTypeName::fromSimple(tname);
		JPType* type = JPTypeManager::getType(name);

		HostRef ref(value);
		jvalue v = type->convertToJava(&ref);

		jvalue* pv = new jvalue();
		*pv = v;

		PyObject* res;
		if (type->isObjectType())
		{
			res = JPyCObject::fromVoidAndDesc((void*)pv, (void*)"object jvalue", PythonHostEnvironment::deleteObjectJValueDestructor);
		}
		else
		{
			res = JPyCObject::fromVoidAndDesc((void*)pv, (void*)"jvalue", PythonHostEnvironment::deleteJValueDestructor);
		}

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaProxy::createProxy(PyObject*, PyObject* arg)
{
	try {
		JPCleaner cleaner;

		PyObject* self;
		PyObject* intf;

		JPyArg::parseTuple(arg, "OO", &self, &intf);

		std::vector<jclass> interfaces;
		Py_ssize_t len = JPyObject::length(intf);

		for (Py_ssize_t i = 0; i < len; i++)
		{
			PyObject* subObj = JPySequence::getItem(intf, i);
			cleaner.add(new HostRef(subObj, false));

			PyObject* claz = JPyObject::getAttrString(subObj, "__javaclass__");
			PyJPClass* c = (PyJPClass*)claz;
			jclass jc = c->m_Class->getClass();
			cleaner.addLocal(jc);
			interfaces.push_back(jc);
		}
		
		HostRef ref = HostRef(self);

		JPProxy* proxy = new JPProxy(&ref, interfaces);

		PyObject* res = JPyCObject::fromVoidAndDesc(proxy, (void*)"jproxy", PythonHostEnvironment::deleteJPProxyDestructor);

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

static PyObject* setJavaExceptionClass(PyObject* self, PyObject* arg)
{
	try {
		PyObject* t;
		JPyArg::parseTuple(arg, "O", &t);
		hostEnv->setJavaExceptionClass(t);

		Py_INCREF(Py_None);
		return Py_None;
	}
	PY_STANDARD_CATCH

	return NULL;
}

static PyMethodDef jpype_methods[] = 
{  
  {"isStarted", (PyCFunction)&JPypeModule::isStarted, METH_NOARGS, ""},
  {"startup", &JPypeModule::startup, METH_VARARGS, ""},
  {"attach", &JPypeModule::attach, METH_VARARGS, ""},
  {"shutdown", (PyCFunction)&JPypeModule::shutdown, METH_NOARGS, ""},
  {"findClass", &JPypeJavaClass::findClass, METH_VARARGS, ""},

  {"synchronized", &JPypeModule::synchronized, METH_VARARGS, ""},
  {"isThreadAttachedToJVM", (PyCFunction)&JPypeModule::isThreadAttached, METH_NOARGS, ""}, 
  {"attachThreadToJVM", (PyCFunction)&JPypeModule::attachThread, METH_NOARGS, ""},
  {"detachThreadFromJVM", (PyCFunction)&JPypeModule::detachThread, METH_NOARGS, ""},
  {"dumpJVMStats", (PyCFunction)&JPypeModule::dumpJVMStats, METH_NOARGS, ""},
  {"attachThreadAsDaemon", (PyCFunction)&JPypeModule::attachThreadAsDaemon, METH_NOARGS, ""},
  {"startReferenceQueue", &JPypeModule::startReferenceQueue, METH_VARARGS, ""},
  {"stopReferenceQueue", (PyCFunction)&JPypeModule::stopReferenceQueue, METH_NOARGS, ""},

  {"setJavaLangObjectClass",   &JPypeJavaClass::setJavaLangObjectClass, METH_VARARGS, ""},
  {"setGetClassMethod",        &JPypeJavaClass::setGetClassMethod, METH_VARARGS, ""},
  {"setSpecialConstructorKey", &JPypeJavaClass::setSpecialConstructorKey, METH_VARARGS, ""},
  
  {"setJavaArrayClass", &JPypeJavaArray::setJavaArrayClass, METH_VARARGS, ""},
  {"setGetJavaArrayClassMethod", &JPypeJavaArray::setGetJavaArrayClassMethod, METH_VARARGS, ""},

  {"setWrapperClass", &JPypeJavaWrapper::setWrapperClass, METH_VARARGS, ""},
  {"setProxyClass", &JPypeJavaProxy::setProxyClass, METH_VARARGS, ""},
  {"createProxy", &JPypeJavaProxy::createProxy, METH_VARARGS, ""},
  {"setStringWrapperClass", &JPypeJavaWrapper::setStringWrapperClass, METH_VARARGS, ""},


  {"convertToJValue", &convertToJValue, METH_VARARGS, ""},

  {"findArrayClass", &JPypeJavaArray::findArrayClass, METH_VARARGS, ""},
  {"getArrayLength", &JPypeJavaArray::getArrayLength, METH_VARARGS, ""},
  {"getArrayItem", &JPypeJavaArray::getArrayItem, METH_VARARGS, ""},
  {"setArrayItem", &JPypeJavaArray::setArrayItem, METH_VARARGS, ""},
  {"getArraySlice", &JPypeJavaArray::getArraySlice, METH_VARARGS, ""},
  {"setArraySlice", &JPypeJavaArray::setArraySlice, METH_VARARGS, ""},
  {"setArrayValues", &JPypeJavaArray::setArrayValues, METH_VARARGS, ""},
  {"newArray", &JPypeJavaArray::newArray, METH_VARARGS, ""},

  {"setJavaExceptionClass", &setJavaExceptionClass, METH_VARARGS, ""},

  {"convertToDirectBuffer", &JPypeJavaNio::convertToDirectBuffer, METH_VARARGS, ""},

  {"setConvertStringObjects", &JPypeModule::setConvertStringObjects, METH_VARARGS, ""},

  // sentinel
  {NULL}
};

PyMODINIT_FUNC init_jpype()
{
	Py_Initialize();
	PyEval_InitThreads();
	  
	PyObject* module = Py_InitModule("_jpype", jpype_methods);  
	Py_INCREF(module);
	hostEnv = new PythonHostEnvironment();
	  
	JPEnv::init(hostEnv);

	PyJPMonitor::initType(module);	
	PyJPMethod::initType(module);	
	PyJPBoundMethod::initType(module);	
	PyJPClass::initType(module);	
	PyJPField::initType(module);	
}

PyObject* detachRef(HostRef* ref)
{
	PyObject* data = (PyObject*)ref->data();
	Py_XINCREF(data);

	ref->release();

	return data;

}

void JPypeJavaException::errorOccurred()
{
	TRACE_IN("PyJavaException::errorOccurred");
	JPCleaner cleaner;
	jthrowable th = JPEnv::getJava()->ExceptionOccurred();
	cleaner.addLocal(th);
	JPEnv::getJava()->ExceptionClear();

	jclass ec = JPJni::getClass(th);
	JPTypeName tn = JPJni::getName(ec);
	JPClass* jpclass = JPTypeManager::findClass(tn);
	cleaner.addLocal(ec);

	PyObject* jexclass = hostEnv->getJavaShadowClass(jpclass);
	HostRef* pyth = hostEnv->newObject(new JPObject(tn, th));
	cleaner.add(pyth);

	PyObject* args = JPySequence::newTuple(2);
	PyObject* arg2 = JPySequence::newTuple(1);
	JPySequence::setItem(arg2, 0, args);
	Py_DECREF(args);
	JPySequence::setItem(args, 0, hostEnv->m_SpecialConstructorKey);
	JPySequence::setItem(args, 1, (PyObject*)pyth->data());

	PyObject* pyexclass = JPyObject::getAttrString(jexclass, "PYEXC");
	Py_DECREF(jexclass);
	

	JPyErr::setObject(pyexclass, arg2);

	Py_DECREF(arg2);
	Py_DECREF(pyexclass);

	TRACE_OUT;
}
