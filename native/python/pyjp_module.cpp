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

#ifdef HAVE_NUMPY
//	#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL jpype_ARRAY_API
#include <numpy/arrayobject.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

static PyObject *PyJPModule_create(PyObject *spec, PyModuleDef *def);
static int PyJPModule_exec(PyObject *m);
static int PyJPModule_clear(PyObject *m);
static int PyJPModule_traverse(PyObject *m, visitproc visit, void *arg);

static PyMethodDef moduleMethods[] = {
	{NULL}
};

struct PyModuleDef_Slot moduleSlots[] = {
	//	{ Py_mod_create, PyJPModule_create},
	{ Py_mod_exec, PyJPModule_exec},
	{0}
};

extern PyModuleDef PyJPModuleDef;
PyModuleDef PyJPModuleDef = {
	PyModuleDef_HEAD_INIT,
	"_jpype",
	"jpype module",
	sizeof (PyJPModuleState),
	moduleMethods,
	moduleSlots,
	(traverseproc) NULL,
	(inquiry) NULL,
	(freefunc) NULL
};

PyMODINIT_FUNC PyInit__jpype()
{
	try
	{
		JP_TRACE_IN_C("PyInit__jpype");
		// This is required for python versions prior to 3.7.
		// It is called by the python initialization starting from 3.7,
		// but is safe to call afterwards.
		PyEval_InitThreads();

		PyObject *module = PyModuleDef_Init(&PyJPModuleDef);
		//		if (module == NULL) return NULL;
		//		if (PyJPModule_exec(module) != 0)
		//		{
		//			Py_DECREF(module);
		//			return NULL;
		//		}
		return module;
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

extern PyType_Spec PyJPArraySpec;
extern PyType_Spec PyJPClassSpec;
extern PyType_Spec PyJPFieldSpec;
extern PyType_Spec PyJPProxySpec;
extern PyType_Spec PyJPMethodSpec;
extern PyType_Spec PyJPMonitorSpec;
extern PyType_Spec PyJPClassMetaSpec;
extern PyType_Spec PyJPValueBaseSpec;
extern PyType_Spec PyJPValueSpec;
extern PyType_Spec PyJPContextSpec;
extern PyType_Spec PyJPClassHintsSpec;
extern PyType_Spec PyJPValueExceptionSpec;

static int PyJPModule_exec(PyObject *module)
{
	try
	{
		printf("Exec\n");
		JP_TRACE_IN_C("PyJPModule_exec");
		PyModule_AddStringConstant(module, "__version__", "0.7.0");

		// Initialize each of the python extension types
		PyJPModuleState *state = PyJPModuleState(module);
		JPPyObject typeBase = JPPyObject(JPPyRef::_claim, PyTuple_Pack(1, &PyType_Type));
		state->PyJPClassMeta_Type = PyType_FromSpecWithBases(&PyJPClassMetaSpec, typeBase.get());
		PyModule_AddObject(module, "PyJPClassMeta", state->PyJPClassMeta_Type);
		JP_PY_CHECK();

		state->PyJPValueBase_Type = PyType_FromSpec(&PyJPValueBaseSpec);
		PyModule_AddObject(module, "PyJPValueBase", state->PyJPValueBase_Type);

		JPPyObject valueBase = JPPyObject(JPPyRef::_claim, PyTuple_Pack(1, state->PyJPValueBase_Type));
		PyModule_AddObject(module, "PyJPValue",
				state->PyJPValue_Type = PyType_FromSpecWithBases(&PyJPValueSpec, valueBase.get()));
		JP_PY_CHECK();

		PyModule_AddObject(module, "PyJPValueException",
				state->PyJPValueExc_Type = PyType_FromSpecWithBases(&PyJPValueExceptionSpec, valueBase.get()));
		JP_PY_CHECK();

		valueBase = JPPyObject(JPPyRef::_claim, PyTuple_Pack(1, state->PyJPValue_Type));
		state->PyJPArray_Type = PyType_FromSpecWithBases(&PyJPArraySpec, valueBase.get());
		PyModule_AddObject(module, "PyJPArray", state->PyJPArray_Type);
		JP_PY_CHECK();

		state->PyJPClass_Type = PyType_FromSpecWithBases(&PyJPClassSpec, valueBase.get());
		PyModule_AddObject(module, "PyJPClass", state->PyJPClass_Type);
		JP_PY_CHECK();

		state->PyJPField_Type = PyType_FromSpecWithBases(&PyJPFieldSpec, valueBase.get());
		PyModule_AddObject(module, "PyJPField", state->PyJPField_Type);
		JP_PY_CHECK();


		// Int is hard so we need to use the regular type process
		JPPyObject intArgs = JPPyObject(JPPyRef::_claim, Py_BuildValue("sNN",
				"_jpype.PyJPValueLong",
				PyTuple_Pack(2, &PyLong_Type, state->PyJPValueBase_Type),
				PyDict_New()));
		state->PyJPValueLong_Type = PyObject_Call((PyObject*) & PyType_Type, intArgs.get(), NULL);
		PyModule_AddObject(module, "PyJPValueLong", state->PyJPValueLong_Type);
		JP_PY_CHECK();

		JPPyObject floatArgs = JPPyObject(JPPyRef::_claim, Py_BuildValue("sNN",
				"_jpype.PyJPValueFloat",
				PyTuple_Pack(2, &PyLong_Type, state->PyJPValueBase_Type),
				PyDict_New()));
		state->PyJPValueFloat_Type = PyObject_Call((PyObject*) & PyType_Type, floatArgs.get(), NULL);
		PyModule_AddObject(module, "PyJPValueFloat", state->PyJPValueFloat_Type);
		JP_PY_CHECK();


		state->PyJPContext_Type = PyType_FromSpec(&PyJPContextSpec);
		PyModule_AddObject(module, "PyJPContext", state->PyJPContext_Type);
		JP_PY_CHECK();

		state->PyJPClassHints_Type = PyType_FromSpec(&PyJPClassHintsSpec);
		PyModule_AddObject(module, "PyJPClassHints", state->PyJPClassHints_Type);

		// We inherit from PyFunction_Type just so we are an instance
		// for purposes of inspect and tab completion tools.  But
		// we will just ignore their memory layout as we have our own.
		JPPyObject functionBase = JPPyObject(JPPyRef::_claim, PyTuple_Pack(1, &PyFunction_Type));
		unsigned long flags = PyFunction_Type.tp_flags;
		PyFunction_Type.tp_flags |= Py_TPFLAGS_BASETYPE;
		state->PyJPMethod_Type = PyType_FromSpecWithBases(&PyJPMethodSpec, functionBase.get());
		PyModule_AddObject(module, "PyJPMethod", state->PyJPMethod_Type);
		PyFunction_Type.tp_flags = flags;
		JP_PY_CHECK();

		PyModule_AddObject(module, "PyJPMonitor", PyType_FromSpec(&PyJPMonitorSpec));
		JP_PY_CHECK();

		state->PyJPProxy_Type = PyType_FromSpec(&PyJPProxySpec);
		PyModule_AddObject(module, "PyJPProxy", state->PyJPProxy_Type);
		JP_PY_CHECK();

#ifdef HAVE_NUMPY
		import_array();
#endif
		JP_TRACE_OUT_C;
	}
	PY_STANDARD_CATCH(NULL);
}

static int PyJPModule_clear(PyObject *m)
{
	PyJPModuleState *state = PyJPModuleState(m);
	// We should be dereferencing all of the types, but currently we are
	// depending on the module dictionary to hold reference.
	return 0;
}

static int PyJPModule_traverse(PyObject *m, visitproc visit, void *arg)
{
	PyJPModuleState *state = PyJPModuleState(m);
	return 0;
}

#ifdef __cplusplus
}
#endif
