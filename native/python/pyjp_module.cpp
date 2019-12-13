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

static int PyJPModule_clear(PyObject *m);
static int PyJPModule_traverse(PyObject *m, visitproc visit, void *arg);
static void PyJPModule_free( void *arg);
PyObject *PyJPModule_startup(PyObject *self, PyObject *args);
PyObject *PyJPModule_shutdown(PyObject *self, PyObject *args);
PyObject *PyJPModule_isStarted(PyObject *self, PyObject *args);
PyObject *PyJPModule_attachThread(PyObject *self, PyObject *args);
PyObject *PyJPModule_attachThreadAsDaemon(PyObject *self, PyObject *args);
PyObject *PyJPModule_detachThread(PyObject *self, PyObject *args);
PyObject *PyJPModule_isThreadAttached(PyObject *self, PyObject *args);
PyObject *PyJPModule_convertToDirectByteBuffer(PyObject *self, PyObject *args);

const char *check_doc =
		"Checks if a thread is attached to the JVM.\n"
		"\n"
		"Python automatically attaches threads when a Java method is called.\n"
		"This creates a resource in Java for the Python thread. This method\n"
		"can be used to check if a Python thread is currently attached so that\n"
		"it can be disconnected prior to thread termination to prevent leaks.\n"
		"\n"
		"Returns:\n"
		"  True if the thread is attached to the JVM, False if the thread is\n"
		"  not attached or the JVM is not running.\n";

const char *shutdown_doc =
		"Shuts down the JVM.\n"
		"\n"
		"This method shuts down the JVM and thus disables access to existing\n"
		"Java objects. Due to limitations in the JPype, it is not possible to\n"
		"restart the JVM after being terminated.\n";

const char *attach_doc =
		"Attaches a thread to the JVM.\n"
		"\n"
		"The function manually connects a thread to the JVM to allow access to\n"
		"Java objects and methods. JPype automatically attaches when a Java\n"
		"resource is used, so a call to this is usually not needed.\n"
		"\n"
		"Raises:\n"
		"  RuntimeError: If the JVM is not running.\n";

const char *detach_doc =
		"Detaches a thread from the JVM.\n"
		"\n"
		"This function detaches the thread and frees the associated resource in\n"
		"the JVM. For codes making heavy use of threading this should be used\n"
		"to prevent resource leaks. The thread can be reattached, so there\n"
		"is no harm in detaching early or more than once. This method cannot fail\n"
		"and there is no harm in calling it when the JVM is not running.\n";

static PyMethodDef moduleMethods[] = {
	{"isStarted", (PyCFunction) (&PyJPModule_isStarted), METH_NOARGS, ""},
	{"startup", (PyCFunction) (&PyJPModule_startup), METH_VARARGS, ""},
	{"shutdown", (PyCFunction) (&PyJPModule_shutdown), METH_NOARGS, shutdown_doc},

	// Threading
	{"isThreadAttached", (PyCFunction) (&PyJPModule_isThreadAttached), METH_NOARGS, check_doc},
	{"attachThread", (PyCFunction) (&PyJPModule_attachThread), METH_NOARGS, attach_doc},
	{"detachThread", (PyCFunction) (&PyJPModule_detachThread), METH_NOARGS, detach_doc},
	{"attachThreadAsDaemon", (PyCFunction) (&PyJPModule_attachThreadAsDaemon), METH_NOARGS, ""},

	// ByteBuffer
	{"_convertToDirectBuffer", (PyCFunction) (&PyJPModule_convertToDirectByteBuffer), METH_VARARGS, ""},
	{"_getClass", (PyCFunction) (&PyJPModule_getClass), METH_VARARGS, ""},
	{NULL}
};

extern PyModuleDef PyJPModuleDef;
PyModuleDef PyJPModuleDef = {
	PyModuleDef_HEAD_INIT,
	"_jpype",
	"jpype module",
	sizeof (PyJPModuleState),
	moduleMethods,
	NULL,
	(traverseproc) PyJPModule_clear,
	(inquiry) PyJPModule_traverse,
	(freefunc) PyJPModule_free
};

extern PyType_Spec PyJPArraySpec;
extern PyType_Spec PyJPClassSpec;
extern PyType_Spec PyJPFieldSpec;
extern PyType_Spec PyJPProxySpec;
extern PyType_Spec PyJPMethodSpec;
extern PyType_Spec PyJPMonitorSpec;
extern PyType_Spec PyJPClassMetaSpec;
extern PyType_Spec PyJPValueBaseSpec;
extern PyType_Spec PyJPValueSpec;
extern PyType_Spec PyJPClassHintsSpec;
extern PyType_Spec PyJPExceptionSpec;

PyJPModuleState *PyJPModuleState_global = NULL;
#define PyJPModule_global PyState_FindModule(&PyJPModuleDef)

PyMODINIT_FUNC PyInit__jpype()
{
	JP_PY_TRY("PyInit__jpype");
	PyObject *module;
	module = PyState_FindModule(&PyJPModuleDef);
	if (module != NULL)
	{
		Py_INCREF(module);
		return module;
	}

	// This is required for python versions prior to 3.7.
	// It is called by the python initialization starting from 3.7,
	// but is safe to call afterwards.
	PyEval_InitThreads();
	JP_PY_CHECK();

	module = PyModule_Create(&PyJPModuleDef);
	JP_PY_CHECK();
#ifdef HAVE_NUMPY
	import_array();
#endif
	PyModule_AddStringConstant(module, "__version__", "0.7.0");

	// Initialize each of the python extension types
	PyJPModuleState *state = ((PyJPModuleState *) PyModule_GetState(module));
	PyJPModuleState_global = state;

	state->m_Context = new JPContext();
	JPContext* context = state->m_Context;

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

	valueBase = JPPyObject(JPPyRef::_claim, PyTuple_Pack(2, PyExc_Exception, state->PyJPValueBase_Type ));
	PyModule_AddObject(module, "PyJPException",
			state->PyJPValueExc_Type = PyType_FromSpecWithBases(&PyJPExceptionSpec, valueBase.get()));
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
			PyTuple_Pack(2, &PyFloat_Type, state->PyJPValueBase_Type),
			PyDict_New()));
	state->PyJPValueFloat_Type = PyObject_Call((PyObject*) & PyType_Type, floatArgs.get(), NULL);
	PyModule_AddObject(module, "PyJPValueFloat", state->PyJPValueFloat_Type);
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

	PyTypeObject* type = (PyTypeObject*) state->PyJPClass_Type;
	PyModule_AddObject(module, "_jboolean",
			PyJPClass_create(type, NULL, context->_boolean).keep());
	PyModule_AddObject(module, "_jchar",
			PyJPClass_create(type, NULL, context->_char).keep());
	PyModule_AddObject(module, "_jbyte",
			PyJPClass_create(type, NULL, context->_byte).keep());
	PyModule_AddObject(module, "_jshort",
			PyJPClass_create(type, NULL, context->_short).keep());
	PyModule_AddObject(module, "_jint",
			PyJPClass_create(type, NULL, context->_int).keep());
	PyModule_AddObject(module, "_jlong",
			PyJPClass_create(type, NULL, context->_long).keep());
	PyModule_AddObject(module, "_jfloat",
			PyJPClass_create(type, NULL, context->_float).keep());
	PyModule_AddObject(module, "_jdouble",
			PyJPClass_create(type, NULL, context->_double).keep());

	PyState_AddModule(module, &PyJPModuleDef);
	JP_PY_CHECK();

	return module;
	JP_PY_CATCH(NULL);
}

static int PyJPModule_clear(PyObject *m)
{
	JP_PY_TRY("PyJPModule_clear");
	//	PyJPModuleState *state = PyJPModuleState(m);
	// We should be dereferencing all of the types, but currently we are
	// depending on the module dictionary to hold reference.
	//	PyJPModuleState *state = PyJPModuleState_global;
	//	if (state != NULL && state->m_Context != NULL && state->m_Context->isRunning())
	//		state->m_Context->shutdownJVM();
	return 0;
	JP_PY_CATCH(-1);
}

static int PyJPModule_traverse(PyObject *m, visitproc visit, void *arg)
{
	JP_PY_TRY("PyJPModule_traverse");
	return 0;
	JP_PY_CATCH(-1);
}

static void PyJPModule_free( void *arg)
{
	JP_PY_TRY("PyJPModule_free");

	JP_PY_CATCH();
}

PyObject *PyJPModule_startup(PyObject *self, PyObject *args)
{
	JP_PY_TRY("PyJPModule_startup", self);
	PyJPModuleState *state = PyJPModuleState_global;
	if (state->m_Context->isRunning())
	{
		PyErr_SetString(PyExc_OSError, "JVM is already started");
		return NULL;
	}
	if (state->m_Context->isShutdown())
	{
		PyErr_SetString(PyExc_OSError, "JVM cannot be restarted");
		return NULL;
	}
	PyObject *vmOpt;
	PyObject *vmPath;
	bool ignoreUnrecognized = true;
	bool convertStrings = true;

	if (!PyArg_ParseTuple(args, "OO!bb", &vmPath, &PyTuple_Type, &vmOpt,
			&ignoreUnrecognized, &convertStrings))
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
		} else
		{
			JP_RAISE_RUNTIME_ERROR("VM Arguments must be strings");
		}
	}

	state->m_Context->startJVM(cVmPath, args, ignoreUnrecognized, convertStrings);
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPModule_shutdown(PyObject *self, PyObject *args)
{
	JP_PY_TRY("PyJPModule_shutdown", self);
	PyJPModuleState *state = PyJPModuleState_global;
	// Stop the JVM
	state->m_Context->shutdownJVM();

	// Disconnect the classes
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPModule_isStarted(PyObject *self, PyObject *args)
{
	PyJPModuleState *state = PyJPModuleState_global;
	return PyBool_FromLong(state->m_Context->isRunning());
}

PyObject *PyJPModule_attachThread(PyObject *self, PyObject *args)
{
	JP_PY_TRY("PyJPModule_attachThread", self);
	PyJPModuleState *state = PyJPModuleState_global;
	JPContext *context = state->m_Context;
	ASSERT_JVM_RUNNING(context);
	context->attachCurrentThread();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPModule_attachThreadAsDaemon(PyObject *self, PyObject *args)
{
	JP_PY_TRY("PyJPModule_attachThreadAsDaemon", self);
	PyJPModuleState *state = PyJPModuleState_global;
	JPContext *context = state->m_Context;
	ASSERT_JVM_RUNNING(context);
	context->attachCurrentThreadAsDaemon();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPModule_detachThread(PyObject *self, PyObject *args)
{
	JP_PY_TRY("PyJPModule_detachThread", self);
	PyJPModuleState *state = PyJPModuleState_global;
	if (state->m_Context->isRunning())
		state->m_Context->detachCurrentThread();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPModule_isThreadAttached(PyObject *self, PyObject *args)
{
	JP_PY_TRY("PyJPModule_isThreadAttached", self);
	PyJPModuleState *state = PyJPModuleState_global;
	if (!state->m_Context->isRunning())
		return PyBool_FromLong(0);
	return PyBool_FromLong(state->m_Context->isThreadAttached());
	JP_PY_CATCH(NULL);
}

PyObject *PyJPModule_convertToDirectByteBuffer(PyObject *self, PyObject *args)
{
	JP_PY_TRY("PyJPModule_convertToDirectByteBuffer", self);
	PyJPModuleState *state = PyJPModuleState_global;
	JPContext *context = state->m_Context;
	ASSERT_JVM_RUNNING(context);
	JPJavaFrame frame(context);

	// Use special method defined on the TypeConverter interface ...
	PyObject *src;

	PyArg_ParseTuple(args, "O", &src);
	JP_PY_CHECK();

	PyObject *res = NULL;
	if (JPPyMemoryView::check(src))
	{
		JP_TRACE("Converting");
		jobject ref = context->_byte->convertToDirectBuffer(frame, src);

		// Bind lifespan of the python to the java object.
		context->getReferenceQueue()->registerRef(ref, src);

		// Convert to python object

		jvalue v;
		v.l = ref;
		JPClass *type = context->getTypeManager()->findClassForObject(ref);
		res = type->convertToPythonObject(frame, v).keep();
	}

	if (res != NULL)
	{
		return res;
	}

	JP_RAISE_RUNTIME_ERROR("Do not know how to convert to direct byte buffer, only memory view supported");
	JP_PY_CATCH(NULL);
}

// Call from Python

PyObject *PyJPModule_getClass(PyObject *self, PyObject *args)
{
	JP_PY_TRY("PyJPModule_getClass", self);
	PyJPModuleState *state = PyJPModuleState_global;

	PyJPClass *cls = NULL;
	if (!PyArg_ParseTuple(args, "O!", state->PyJPClass_Type, &cls))
		return NULL;

	JPClass *javaClass = cls->m_Class;
	if (javaClass->getHost() != NULL)
	{
		PyObject* out = javaClass->getHost();
		Py_INCREF(out);
		return out;
	}

	// Get the type factory
	JPPyObject factory(JPPyRef::_accept,
			PyObject_GetAttrString(PyJPModule_global, "_JClassFactory"));

	//	PyObject* factory = PyDict_GetItemString(Py_TYPE(self)->tp_dict, "_JClassFactory");
	if (factory.isNull())
		JP_RAISE_RUNTIME_ERROR("Factory not set");

	// Call the factory
	JPPyObject out = factory.call(args, NULL);

	// Store caches
	javaClass ->setHost(out.get());
	return out.keep();
	JP_PY_CATCH(NULL);
}

#ifdef __cplusplus
}
#endif

JPPyObject JPPythonEnv::newJavaClass(JPClass *javaClass)
{
	JP_TRACE_IN("JPPythonEnv::newJavaClass");
	PyJPModuleState *state = PyJPModuleState_global;

	ASSERT_NOT_NULL(javaClass);

	// Check the cache
	if (javaClass->getHost() != NULL)
		return JPPyObject(JPPyRef::_use, javaClass->getHost());

	JPContext *context = javaClass->getContext();

	// Get the type factory
	JPPyObject factory(JPPyRef::_claim,
			PyObject_GetAttrString(PyJPModule_global, "_JClassFactory"));

	// Pack the args
	JPPyTuple args(JPPyTuple::newTuple(1));
	args.setItem(0, PyJPClass_create((PyTypeObject*) state->PyJPClass_Type,
			context, javaClass).get());

	// Call the factory in Python
	return JPPyObject(JPPyRef::_call, PyObject_Call(factory.get(), args.get(), NULL));
	JP_TRACE_OUT;
}
