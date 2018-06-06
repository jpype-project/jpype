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
#ifdef HAVE_NUMPY
//	#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
	#define PY_ARRAY_UNIQUE_SYMBOL jpype_ARRAY_API
	#include <numpy/arrayobject.h>
#endif

PythonHostEnvironment* hostEnv;
PyObject* convertToJValue(PyObject* self, PyObject* arg)
{
	try {
		ASSERT_JVM_RUNNING("convertToJValue");
		JPJavaFrame frame;
		char* tname;
		PyObject* value;

		JPyArg::parseTuple(arg, "sO", &tname, &value);

		JPTypeName name = JPTypeName::fromSimple(tname);
		JPType* type = JPTypeManager::getType(name);

		HostRef ref(value);
		jvalue v = type->convertToJava(&ref);

		jvalue* pv = new jvalue();

		// Transfer ownership to python
		PyObject* res;
		if (type->isObjectType())
		{
			pv->l = frame.NewGlobalRef(v.l);
			res = JPyCObject::fromVoidAndDesc((void*)pv, "object jvalue", PythonHostEnvironment::deleteObjectJValueDestructor);
		}
		else
		{
			*pv = v;
			res = JPyCObject::fromVoidAndDesc((void*)pv, "jvalue", PythonHostEnvironment::deleteJValueDestructor);
		}

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaProxy::createProxy(PyObject*, PyObject* arg)
{
	try {
		ASSERT_JVM_RUNNING("createProxy");
		JPJavaFrame frame;
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
			interfaces.push_back(jc);
		}
		
		HostRef ref = HostRef(self);

		JPProxy* proxy = new JPProxy(&ref, interfaces);

		PyObject* res = JPyCObject::fromVoidAndDesc(proxy, "jproxy", PythonHostEnvironment::deleteJPProxyDestructor);

		return res;
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
  {"setResource", &JPypeModule::setResource, METH_VARARGS, ""},

  {"synchronized", &JPypeModule::synchronized, METH_VARARGS, ""},
  {"isThreadAttachedToJVM", (PyCFunction)&JPypeModule::isThreadAttached, METH_NOARGS, ""}, 
  {"attachThreadToJVM", (PyCFunction)&JPypeModule::attachThread, METH_NOARGS, ""},
  {"detachThreadFromJVM", (PyCFunction)&JPypeModule::detachThread, METH_NOARGS, ""},
  {"dumpJVMStats", (PyCFunction)&JPypeModule::dumpJVMStats, METH_NOARGS, ""},
  {"attachThreadAsDaemon", (PyCFunction)&JPypeModule::attachThreadAsDaemon, METH_NOARGS, ""},
  {"startReferenceQueue", &JPypeModule::startReferenceQueue, METH_VARARGS, ""},

  {"createProxy", &JPypeJavaProxy::createProxy, METH_VARARGS, ""},

  {"convertToJValue", &convertToJValue, METH_VARARGS, ""},

  {"findArrayClass", &JPypeJavaArray::findArrayClass, METH_VARARGS, ""},
  {"getArrayLength", &JPypeJavaArray::getArrayLength, METH_VARARGS, ""},
  {"getArrayItem", &JPypeJavaArray::getArrayItem, METH_VARARGS, ""},
  {"setArrayItem", &JPypeJavaArray::setArrayItem, METH_VARARGS, ""},
  {"getArraySlice", &JPypeJavaArray::getArraySlice, METH_VARARGS, ""},
  {"setArraySlice", &JPypeJavaArray::setArraySlice, METH_VARARGS, ""},
  {"newArray", &JPypeJavaArray::newArray, METH_VARARGS, ""},

  {"convertToDirectBuffer", &JPypeJavaNio::convertToDirectBuffer, METH_VARARGS, ""},

  {"setConvertStringObjects", &JPypeModule::setConvertStringObjects, METH_VARARGS, ""},

  // sentinel
  {NULL}
};
#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_jpype",
    "jpype module",
    -1,
    jpype_methods,
};
#endif

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit__jpype()
#else
PyMODINIT_FUNC init_jpype()
#endif
{
	Py_Initialize();
	PyEval_InitThreads();
	  
#if PY_MAJOR_VERSION >= 3
    PyObject* module = PyModule_Create(&moduledef);
#else
	PyObject* module = Py_InitModule("_jpype", jpype_methods);
#endif
	Py_INCREF(module);
	hostEnv = new PythonHostEnvironment();
	  
	JPEnv::init(hostEnv);

	PyJPMonitor::initType(module);	
	PyJPMethod::initType(module);	
	PyJPBoundMethod::initType(module);	
	PyJPClass::initType(module);	
	PyJPField::initType(module);	

#if (PY_VERSION_HEX < 0x02070000)
	jpype_memoryview_init(module);
#endif

#ifdef HAVE_NUMPY
	import_array();
#endif
#if PY_MAJOR_VERSION >= 3
    return module;
#endif
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
	JPJavaFrame frame(8);
	JPCleaner cleaner;
	jthrowable th = frame.ExceptionOccurred();
	frame.ExceptionClear();

	jclass ec = JPJni::getClass(th);
	JPTypeName tn = JPJni::getName(ec);
	JPClass* jpclass = JPTypeManager::findClass(tn);

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
