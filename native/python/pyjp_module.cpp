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
	{"setResource", (PyCFunction) (&PyJPModule::setResource), METH_VARARGS, ""},

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

PyInterpreterState *PyJPModule::s_Interpreter = NULL;

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
	PyThreadState *mainThreadState = PyThreadState_Get();
	PyJPModule::s_Interpreter = mainThreadState->interp;

	// Initialize the module (depends on python version)
#if PY_MAJOR_VERSION >= 3
	PyObject *module = PyModule_Create(&moduledef);
#else
	PyObject *module = Py_InitModule("_jpype", jpype_methods);
#endif
	Py_INCREF(module);
	PyModule_AddStringConstant(module, "__version__", "0.7.0");

	// Initialize the Python static resources
	JPPythonEnv::init();

	// Initialize each of the python extension types
	PyJPValue::initType(module);
	PyJPArray::initType(module);
	PyJPClass::initType(module);
	PyJPContext::initType(module);
	PyJPField::initType(module);
	PyJPMethod::initType(module);
	PyJPMonitor::initType(module);
	PyJPProxy::initType(module);

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

PyObject *PyJPModule::setResource(PyObject *self, PyObject *arg)
{
	JP_TRACE_IN_C("PyJPModule::setResource");
	try
	{
		char *tname;
		PyObject *value;
		PyArg_ParseTuple(arg, "sO", &tname, &value);
		JP_PY_CHECK();
		JPPythonEnv::setResource(tname, value);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

