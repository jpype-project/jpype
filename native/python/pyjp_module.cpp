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
#include "jpype.h"
#include "pyjp.h"
#include "jp_arrayclass.h"
#include "jp_reference_queue.h"


extern void PyJPArray_initType(PyObject* module);
extern void PyJPClass_initType(PyObject* module);
extern void PyJPField_initType(PyObject* module);
extern void PyJPMethod_initType(PyObject* module);
extern void PyJPMonitor_initType(PyObject* module);
extern void PyJPProxy_initType(PyObject* module);
extern void PyJPObject_initType(PyObject* module);
extern void PyJPNumber_initType(PyObject* module);

// To ensure no leaks (requires C++ linkage)

class JPViewWrapper
{
public:

	JPViewWrapper()
	{
		view = new Py_buffer();
	}

	~JPViewWrapper()
	{
		delete view;
	}
	Py_buffer *view;
} ;


PyObject* _JArray = NULL;
PyObject* _JObject = NULL;
PyObject* _JInterface = NULL;
PyObject* _JException = NULL;
PyObject* _JClassPre = NULL;
PyObject* _JClassPost = NULL;
PyObject* _JMethodDoc = NULL;
PyObject* _JMethodAnnotations = NULL;
PyObject* _JMethodCode = NULL;
PyObject* _JObjectKey = NULL;

static void PyJPModule_loadResources(PyObject* module)
{
	// Note that if any resource is missing the user will get
	// the message:
	//
	//    AttributeError: module '_jpype' has no attribute 'SomeResource'
	//    The above exception was the direct cause of the following exception:
	//
	//    Traceback (most recent call last):
	//      File ...
	//    RuntimeError: JPype resource is missing
	try
	{
		// Complete the initialization here
		_JObject = PyObject_GetAttrString(module, "JObject");
		JP_PY_CHECK();
		Py_INCREF(_JObject);
		_JInterface = PyObject_GetAttrString(module, "JInterface");
		JP_PY_CHECK();
		Py_INCREF(_JInterface);
		_JArray = PyObject_GetAttrString(module, "JArray");
		JP_PY_CHECK();
		Py_INCREF(_JArray);
		_JException = PyObject_GetAttrString(module, "JException");
		JP_PY_CHECK();
		Py_INCREF(_JException);
		_JClassPre = PyObject_GetAttrString(module, "_JClassPre");
		JP_PY_CHECK();
		Py_INCREF(_JClassPre);
		_JClassPost = PyObject_GetAttrString(module, "_JClassPost");
		JP_PY_CHECK();
		Py_INCREF(_JClassPost);
		_JMethodDoc = PyObject_GetAttrString(module, "getMethodDoc");
		JP_PY_CHECK();
		Py_INCREF(_JMethodDoc);
		_JMethodAnnotations = PyObject_GetAttrString(module, "getMethodAnnotations");
		JP_PY_CHECK();
		Py_INCREF(_JMethodAnnotations);
		_JMethodCode = PyObject_GetAttrString(module, "getMethodCode");
		JP_PY_CHECK();
		Py_INCREF(_JMethodCode);

		_JObjectKey = PyCapsule_New(module, "constructor key", NULL);

	}	catch (JPypeException&)
	{
		Py_SetStringWithCause(PyExc_RuntimeError, "JPype resource is missing");
		JP_RAISE_PYTHON("resource failed");
	}
}

#ifdef __cplusplus
extern "C"
{
#endif

void Py_SetStringWithCause(PyObject *exception,
		const char *str)
{
	// See _PyErr_TrySetFromCause
	PyObject *exc1, *val1, *tb1;
	PyErr_Fetch(&exc1, &val1, &tb1);
	PyErr_NormalizeException(&exc1, &val1, &tb1);
	if (tb1 != NULL)
	{
		PyException_SetTraceback(val1, tb1);
		Py_DECREF(tb1);
	}
	Py_DECREF(exc1);
	PyErr_SetString(exception, str);
	PyObject *exc2, *val2, *tb2;
	PyErr_Fetch(&exc2, &val2, &tb2);
	PyErr_NormalizeException(&exc2, &val2, &tb2);
	PyException_SetCause(val2, val1);
	PyErr_Restore(exc2, val2, tb2);
}

PyObject* Py_GetAttrDescriptor(PyTypeObject *type, PyObject *attr_name)
{
	JP_PY_TRY("Py_GetAttrDescriptor");
	if (type->tp_mro == NULL)
		return NULL;

	PyObject *mro = type->tp_mro;
	Py_ssize_t n = PyTuple_Size(mro);
	for (Py_ssize_t i = 0; i < n; ++i)
	{
		PyTypeObject *type2 = (PyTypeObject*) PyTuple_GetItem(mro, i);
		PyObject *res = PyDict_GetItem(type2->tp_dict, attr_name);
		if (res)
		{
			Py_INCREF(res);
			return res;
		}
	}

	// Last check is id in the parent
	{
		PyObject *res = PyDict_GetItem(Py_TYPE(type)->tp_dict, attr_name);
		if (res)
		{
			Py_INCREF(res);
			return res;
		}
	}

	return NULL;
	JP_PY_CATCH(NULL);
}

int Py_IsSubClassSingle(PyTypeObject* type, PyTypeObject* obj)
{
	if (type == NULL || obj == NULL)
		return 0;
	PyObject* mro1 = obj->tp_mro;
	Py_ssize_t n1 = PyTuple_Size(mro1);
	Py_ssize_t n2 = PyTuple_Size(type->tp_mro);
	if (n1 < n2)
		return 0;
	return PyTuple_GetItem(mro1, n1 - n2) == (PyObject*) type;
}

int Py_IsInstanceSingle(PyTypeObject* type, PyObject* obj)
{
	if (type == NULL || obj == NULL)
		return 0;
	return Py_IsSubClassSingle(type, Py_TYPE(obj));
}

static PyObject* PyJPModule_startup(PyObject* module, PyObject* args)
{
	JP_PY_TRY("PyJPModule_startup");
	if (JPEnv::isInitialized())
	{
		PyErr_SetString(PyExc_OSError, "JVM is already started");
		return NULL;
	}

	PyObject* vmOpt;
	PyObject* vmPath;
	char ignoreUnrecognized = true;
	char convertStrings = false;

	if (!PyArg_ParseTuple(args, "OO!bb", &vmPath, &PyTuple_Type, &vmOpt,
			&ignoreUnrecognized, &convertStrings))
	{
		return NULL;
	}

	if (!(JPPyString::check(vmPath)))
	{
		JP_RAISE(PyExc_RuntimeError, "Java JVM path must be a string");
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
			JP_RAISE(PyExc_RuntimeError, "VM Arguments must be strings");
		}
	}

	PyJPModule_loadResources(module);
	JPEnv::loadJVM(cVmPath, args, ignoreUnrecognized != 0, convertStrings != 0);
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

static PyObject* PyJPModule_attach(PyObject* module, PyObject* args)
{
	JP_PY_TRY("PyJPModule_attach");
	if (JPEnv::isInitialized())
	{
		PyErr_SetString(PyExc_OSError, "JVM is already started");
		return NULL;
	}
	PyObject* vmPath;

	if (!PyArg_ParseTuple(args, "O", &vmPath))
	{
		return NULL;
	}

	if (!(JPPyString::check(vmPath)))
	{
		JP_RAISE(PyExc_RuntimeError, "First parameter must be a string or unicode");
	}

	string cVmPath = JPPyString::asStringUTF8(vmPath);
	JPEnv::attachJVM(cVmPath);
	PyJPModule_loadResources(module);
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

static PyObject* PyJPModule_dumpJVMStats(PyObject* obj)
{
	cerr << "JVM activity report     :" << endl;
	cerr << "\tclasses loaded       : " << JPTypeManager::getLoadedClasses() << endl;
	Py_RETURN_NONE;
}

static PyObject* PyJPModule_shutdown(PyObject* obj)
{
	JP_PY_TRY("PyJPModule_shutdown");
	JPEnv::shutdown();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

static PyObject* PyJPModule_isStarted(PyObject* obj)
{
	return PyBool_FromLong(JPEnv::isInitialized());
}

static PyObject* PyJPModule_attachThread(PyObject* obj)
{
	JP_PY_TRY("PyJPModule_attachThread");
	ASSERT_JVM_RUNNING();
	JPEnv::attachCurrentThread();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

static PyObject* PyJPModule_attachThreadAsDaemon(PyObject* obj)
{
	JP_PY_TRY("PyJPModule_attachThreadAsDaemon");
	ASSERT_JVM_RUNNING();
	JPEnv::attachCurrentThreadAsDaemon();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

static PyObject* PyJPModule_detachThread(PyObject* obj)
{
	JP_PY_TRY("PyJPModule_detachThread");
	if (JPEnv::isInitialized())
		JPEnv::detachCurrentThread();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

static PyObject* PyJPModule_isThreadAttached(PyObject* obj)
{
	JP_PY_TRY("PyJPModule_isThreadAttached");
	if (!JPEnv::isInitialized())
		return PyBool_FromLong(0);
	return PyBool_FromLong(JPEnv::isThreadAttached());
	JP_PY_CATCH(NULL);
}

// Cleanup hook for Py_buffer

static void releaseView(void* view)
{
	if (view != 0)
	{
		PyBuffer_Release((Py_buffer*) view);
		delete (Py_buffer*) view;
	}
}

static PyObject* PyJPModule_convertToDirectByteBuffer(PyObject* self, PyObject* src)
{
	JP_PY_TRY("PyJPModule_convertToDirectByteBuffer");
	ASSERT_JVM_RUNNING();
	JPJavaFrame frame;

	if (PyObject_CheckBuffer(src))
	{
		JPViewWrapper vw;
		if (PyObject_GetBuffer(src, vw.view, PyBUF_WRITABLE) == -1)
			return NULL;

		// Create a byte buffer
		jvalue v;
		v.l = frame.NewDirectByteBuffer(vw.view->buf, vw.view->len);

		// Bind lifespan of the view to the java object.
		JPReferenceQueue::registerRef(v.l, vw.view, &releaseView);
		vw.view = 0;
		JPClass* type = JPTypeManager::findClassForObject(v.l);
		return type->convertToPythonObject(v).keep();
	}
	JP_RAISE(PyExc_TypeError, "convertToDirectByteBuffer requires buffer support");
	JP_PY_CATCH(NULL);
}

PyObject *PyJPModule_newArrayType(PyObject *module, PyObject *args)
{
	JP_PY_TRY("PyJPModule_getArrayType");

	PyObject *type, *dims;
	if (!PyArg_ParseTuple(args, "OO", &type, &dims))
		JP_RAISE_PYTHON("bad args");
	if (!PyIndex_Check(dims))
		JP_RAISE(PyExc_TypeError, "dims must be an integer");
	Py_ssize_t d = PyNumber_AsSsize_t(dims, PyExc_IndexError);
	if (d > 255)
		JP_RAISE(PyExc_ValueError, "dims too large");
	JPClass* cls = PyJPClass_getJPClass(type);
	if (cls == NULL)
		JP_RAISE(PyExc_TypeError, "Java class required");

	stringstream ss;
	for (int i = 0; i < d; ++i)
		ss << "[";
	if (cls->isPrimitive())
		ss << ((JPPrimitiveType*) cls)->getTypeCode();
	else if (dynamic_cast<JPArrayClass*> (cls) == cls)
		ss << JPJni::getName(cls->getJavaClass());
	else
		ss << "L" << JPJni::getName(cls->getJavaClass()) << ";";
	JPClass* arraycls = JPTypeManager::findClass(ss.str());
	return PyJPClass_create(arraycls).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPModule_getClass(PyObject* module, PyObject *obj)
{
	JP_PY_TRY("PyJPModule_getClass");
	ASSERT_JVM_RUNNING();
	JPJavaFrame frame;

	JPClass* cls;
	if (JPPyString::check(obj))
	{
		// String From Python
		cls = JPTypeManager::findClass(JPPyString::asStringUTF8(obj));
		if (cls == NULL)
			JP_RAISE(PyExc_ValueError, "Unable to find Java class");
	} else
	{
		// From an existing java.lang.Class object
		JPValue *value = PyJPValue_getJavaSlot(obj);
		if (value == 0 || value->getClass() != JPTypeManager::_java_lang_Class)
		{
			std::stringstream ss;
			ss << "JClass requires str or java.lang.Class instance, not `" << Py_TYPE(obj)->tp_name << "`";
			JP_RAISE(PyExc_TypeError, ss.str().c_str());
		}
		cls = JPTypeManager::findClass((jclass) value->getValue().l);
		if (cls == NULL)
			JP_RAISE(PyExc_ValueError, "Unable to find class");
	}

	return PyJPClass_create(cls).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPModule_hasClass(PyObject* module, PyObject *obj)
{
	JP_PY_TRY("PyJPModule_getClass");
	if (!JPEnv::isInitialized())
		Py_RETURN_FALSE;
	ASSERT_JVM_RUNNING();
	JPJavaFrame frame;

	JPClass* cls;
	if (JPPyString::check(obj))
	{
		// String From Python
		cls = JPTypeManager::findClass(JPPyString::asStringUTF8(obj));
		if (cls == NULL)
			JP_RAISE(PyExc_ValueError, "Unable to find Java class");
	} else
	{
		JP_RAISE(PyExc_TypeError, "str is required");
	}

	PyObject *host = cls->getHost();
	return PyBool_FromLong(host != NULL);
	JP_PY_CATCH(NULL);
}

PyObject* examine(PyObject *module, PyObject *other)
{
	JP_PY_TRY("examine");
	int ret = 0;
	PyTypeObject *type;
	if (PyType_Check(other))
		type = (PyTypeObject*) other;
	else
		type = Py_TYPE(other);

	printf("======\n");
	if (!PyType_Check(other))
	{
		printf("  Object:\n");
		printf("    size: %lld\n", Py_SIZE(other));
		printf("    dictoffset: %lld\n", ((long long) _PyObject_GetDictPtr(other)-(long long) other));
		printf("    javaoffset: %ld\n", PyJPValue_getJavaSlotOffset(other));
	}
	printf("  Type: %p\n", type);
	printf("    name: %s\n", type->tp_name);
	printf("    typename: %s\n", Py_TYPE(type)->tp_name);
	printf("    gc: %d\n", (type->tp_flags & Py_TPFLAGS_HAVE_GC) == Py_TPFLAGS_HAVE_GC);
	printf("    basicsize: %ld\n", type->tp_basicsize);
	printf("    itemsize: %ld\n", type->tp_itemsize);
	printf("    dictoffset: %ld\n", type->tp_dictoffset);
	printf("    weaklistoffset: %ld\n", type->tp_weaklistoffset);
	printf("    hasJavaSlot: %d\n", PyJPValue_hasJavaSlot(type));
	printf("    getattro: %p\n", type->tp_getattro);
	printf("    setattro: %p\n", type->tp_setattro);
	printf("    getattr: %p\n", type->tp_getattr);
	printf("    setattr: %p\n", type->tp_setattr);
	printf("    alloc: %p\n", type->tp_alloc);
	printf("    free: %p\n", type->tp_free);
	printf("    finalize: %p\n", type->tp_finalize);
	printf("======\n");
	fflush(stdout);

	return PyBool_FromLong(ret);
	JP_PY_CATCH(NULL);
}

static PyMethodDef moduleMethods[] = {
	// Startup and initialization
	{"isStarted", (PyCFunction) (&PyJPModule_isStarted), METH_NOARGS, ""},
	{"startup", (PyCFunction) (&PyJPModule_startup), METH_VARARGS, ""},
	{"attach", (PyCFunction) (&PyJPModule_attach), METH_VARARGS, ""},
	{"shutdown", (PyCFunction) (&PyJPModule_shutdown), METH_NOARGS, ""},
	{"_getClass", (PyCFunction) (&PyJPModule_getClass), METH_O, ""},
	{"_hasClass", (PyCFunction) (&PyJPModule_hasClass), METH_O, ""},
	{"examine", (PyCFunction) (&examine), METH_O, ""},
	{"_newArrayType", (PyCFunction) (&PyJPModule_newArrayType), METH_VARARGS, ""},

	// Threading
	{"isThreadAttachedToJVM", (PyCFunction) (&PyJPModule_isThreadAttached), METH_NOARGS, ""},
	{"attachThreadToJVM", (PyCFunction) (&PyJPModule_attachThread), METH_NOARGS, ""},
	{"detachThreadFromJVM", (PyCFunction) (&PyJPModule_detachThread), METH_NOARGS, ""},
	{"attachThreadAsDaemon", (PyCFunction) (&PyJPModule_attachThreadAsDaemon), METH_NOARGS, ""},

	{"dumpJVMStats", (PyCFunction) (&PyJPModule_dumpJVMStats), METH_NOARGS, ""},

	{"convertToDirectBuffer", (PyCFunction) (&PyJPModule_convertToDirectByteBuffer), METH_O, ""},

	// sentinel
	{NULL}
};

static struct PyModuleDef moduledef = {
	PyModuleDef_HEAD_INIT,
	"_jpype",
	"jpype module",
	-1,
	moduleMethods,
};

PyObject *PyJPModule = NULL;

PyMODINIT_FUNC PyInit__jpype()
{
	JP_PY_TRY("PyInit__jpype");
	// This is required for python versions prior to 3.7.
	// It is called by the python initialization starting from 3.7,
	// but is safe to call afterwards.
	PyEval_InitThreads();

	// Initialize the module (depends on python version)
	PyObject* module = PyModule_Create(&moduledef);
	// PyJPModule = module;
	Py_INCREF(module);
	PyJPModule = module;
	PyModule_AddStringConstant(module, "__version__", "0.7.2");

	// Initialize each of the python extension types
	PyJPClass_initType(module);
	PyJPObject_initType(module);

	PyJPArray_initType(module);
	PyJPField_initType(module);
	PyJPMethod_initType(module);
	PyJPNumber_initType(module);
	PyJPMonitor_initType(module);
	PyJPProxy_initType(module);

	return module;
	JP_PY_CATCH(NULL);
}

#ifdef __cplusplus
}
#endif

void PyJPModule_rethrow(const JPStackInfo& info)
{
	JP_TRACE_IN("PyJPModule_rethrow");
	JP_TRACE(info.getFile(), info.getLine());
	try
	{
		throw;
	} catch (JPypeException& ex)
	{
		ex.from(info); // this likely wont be necessary, but for now we will add the entry point.
		ex.toPython();
		return;
	}
	JP_TRACE_OUT;
}
