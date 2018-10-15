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

#include <pyjp.h>
#include <jpype_memory_view.h>

#ifdef HAVE_NUMPY
//	#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL jpype_ARRAY_API
#include <numpy/arrayobject.h>
#endif

static PyMethodDef jpype_methods[] = {
	// Startup and initialization
	{"isStarted", (PyCFunction) (&PyJPModule::isStarted), METH_NOARGS, ""},
	{"startup", (PyCFunction) (&PyJPModule::startup), METH_VARARGS, ""},
	{"attach", (PyCFunction) (&PyJPModule::attach), METH_VARARGS, ""},
	{"shutdown", (PyCFunction) (&PyJPModule::shutdown), METH_NOARGS, ""},
	{"setResource", (PyCFunction) (&PyJPModule::setResource), METH_VARARGS, ""},

	// Threading
	{"isThreadAttachedToJVM", (PyCFunction) (&PyJPModule::isThreadAttached), METH_NOARGS, ""},
	{"attachThreadToJVM", (PyCFunction) (&PyJPModule::attachThread), METH_NOARGS, ""},
	{"detachThreadFromJVM", (PyCFunction) (&PyJPModule::detachThread), METH_NOARGS, ""},
	{"attachThreadAsDaemon", (PyCFunction) (&PyJPModule::attachThreadAsDaemon), METH_NOARGS, ""},

	{"dumpJVMStats", (PyCFunction) (&PyJPModule::dumpJVMStats), METH_NOARGS, ""},

	{"convertToDirectBuffer", (PyCFunction) (&PyJPModule::convertToDirectByteBuffer), METH_VARARGS, ""},

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
	// This is required for python versions prior to 3.7.  
	// It is called by the python initialization starting from 3.7,
	// but is safe to call afterwards.
	PyEval_InitThreads();

	// Initalize the module (depends on python version)
#if PY_MAJOR_VERSION >= 3
	PyObject* module = PyModule_Create(&moduledef);
#else
	PyObject* module = Py_InitModule("_jpype", jpype_methods);
#endif
	Py_INCREF(module);
	PyModule_AddStringConstant(module, "__version__", "0.7.0");

	// Initialize the Java static resources
	JPEnv::init();

	// Initialize the Python static resources
	JPPythonEnv::init();

	// Initialize each of the python extension types
	PyJPArray::initType(module);
	PyJPClass::initType(module);
	PyJPField::initType(module);
	PyJPMethod::initType(module);
	PyJPMonitor::initType(module);
	PyJPProxy::initType(module);
	PyJPValue::initType(module);

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

PyObject* PyJPModule::startup(PyObject* obj, PyObject* args)
{
	JP_TRACE_IN("PyJPModule::startup");
	if (JPEnv::isInitialized())
	{
		PyErr_SetString(PyExc_OSError, "JVM is already started");
		return NULL;
	}

	try
	{
		PyObject* vmOpt;
		PyObject* vmPath;
		char ignoreUnrecognized = true;

		if (!PyArg_ParseTuple(args, "OO!b|", &vmPath, &PyTuple_Type, &vmOpt, &ignoreUnrecognized))
		{
			return NULL;
		}

		if (!(JPPyString::check(vmPath)))
		{
			JP_RAISE_RUNTIME_ERROR("Java JVM path must be a string");
		}

		string cVmPath = JPPyString::asStringUTF8(vmPath);
		JP_TRACE("vmpath", cVmPath);

		StringVector args;
		JPPySequence seq(JPPyRef::_use, vmOpt);

		for (int i = 0; i < seq.size(); i++)
		{
			JPPyObject obj(seq[i]);

			if (JPPyString::check(obj.get()))
			{
				// TODO support unicode
				string v = JPPyString::asStringUTF8(obj.get());
				JP_TRACE("arg", v);
				args.push_back(v);
			}
			else
			{
				JP_RAISE_RUNTIME_ERROR("VM Arguments must be strings");
			}
		}

		JPEnv::loadJVM(cVmPath, ignoreUnrecognized, args);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;
	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPModule::attach(PyObject* obj, PyObject* args)
{
	JP_TRACE_IN("PyJPModule::attach");
	if (JPEnv::isInitialized())
	{
		PyErr_SetString(PyExc_OSError, "JVM is already started");
		return NULL;
	}

	try
	{
		PyObject* vmPath;

		if (!PyArg_ParseTuple(args, "O", &vmPath))
		{
			return NULL;
		}

		if (!(JPPyString::check(vmPath)))
		{
			JP_RAISE_RUNTIME_ERROR("First parameter must be a string or unicode");
		}

		string cVmPath = JPPyString::asStringUTF8(vmPath);
		JPEnv::attachJVM(cVmPath);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;

	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPModule::dumpJVMStats(PyObject* obj)
{
	cerr << "JVM activity report     :" << endl;
	//cerr << "\tmethod calls         : " << methodCalls << endl;
	//cerr << "\tstatic method calls  : " << staticMethodCalls << endl;
	//cerr << "\tconstructor calls    : " << constructorCalls << endl;
	//cerr << "\tproxy callbacks      : " << JProxy::getCallbackCount() << endl;
	//cerr << "\tfield gets           : " << fieldGets << endl;
	//cerr << "\tfield sets           : " << fieldSets << endl;
	cerr << "\tclasses loaded       : " << JPTypeManager::getLoadedClasses() << endl;
	Py_RETURN_NONE;
}

PyObject* PyJPModule::shutdown(PyObject* obj)
{
	JP_TRACE_IN("PyJPModule::shutdown");
	try
	{
		JPEnv::shutdown();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;

	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPModule::isStarted(PyObject* obj)
{
	return PyBool_FromLong(JPEnv::isInitialized());
}

PyObject* PyJPModule::attachThread(PyObject* obj)
{
	JP_TRACE_IN("PyJPModule::attachThread");
	try
	{
		ASSERT_JVM_RUNNING("JPypeModule::attachThread");
		JPEnv::attachCurrentThread();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;

	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPModule::attachThreadAsDaemon(PyObject* obj)
{
	JP_TRACE_IN("PyJPModule::attachThreadAsDaemon");
	try
	{
		ASSERT_JVM_RUNNING("JPypeModule::attachThreadAsDaemon");
		JPEnv::attachCurrentThreadAsDaemon();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;

	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPModule::detachThread(PyObject* obj)
{
	JP_TRACE_IN("PyJPModule::detachThread");
	try
	{
		ASSERT_JVM_RUNNING("JPypeModule::detachThread");
		JPEnv::detachCurrentThread();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;

	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPModule::isThreadAttached(PyObject* obj)
{
	try
	{
		ASSERT_JVM_RUNNING("JPypeModule::isThreadAttached");
		return PyBool_FromLong(JPEnv::isThreadAttached());
	}
	PY_STANDARD_CATCH;

	return NULL;

}

PyObject* PyJPModule::setResource(PyObject* self, PyObject* arg)
{
	JP_TRACE_IN("PyJPModule::setResource");
	try
	{
		char* tname;
		PyObject* value;
		PyArg_ParseTuple(arg, "sO", &tname, &value);
		JP_PY_CHECK();
		JPPythonEnv::setResource(tname, value);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH

	return NULL;
	JP_TRACE_OUT;
}

PyObject* PyJPModule::convertToDirectByteBuffer(PyObject* self, PyObject* args)
{
	JP_TRACE_IN("PyJPModule::convertToDirectByteBuffer");
	try
	{
		ASSERT_JVM_RUNNING("PyJPModule::convertToDirectByteBuffer");
		JPJavaFrame frame;

		// Use special method defined on the TypeConverter interface ...
		PyObject* src;

		PyArg_ParseTuple(args, "O", &src);
		JP_PY_CHECK();

		PyObject* res = NULL;
		if (JPPyMemoryView::check(src))
		{
			JP_TRACE("Converting");
			jobject ref = JPTypeManager::_byte->convertToDirectBuffer(src);

			// Bind lifespan of the python to the java object.
			JPReferenceQueue::registerRef(ref, src);

			// Convert to python object 

			jvalue v;
			v.l = ref;
			JPClass* type = JPTypeManager::findClassForObject(ref);
			res = type->convertToPythonObject(v).keep();
		}

		if (res != NULL)
		{
			return res;
		}

		JP_RAISE_RUNTIME_ERROR("Do not know how to convert to direct byte buffer, only memory view supported");
	}
	PY_STANDARD_CATCH
	return NULL;
	JP_TRACE_OUT;
}

