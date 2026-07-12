// --- file: python/pyjp_module.cpp ---
/*****************************************************************************
	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.

	See NOTICE file for details.
 *****************************************************************************/
#include "jpype.h"
#include "pyjp.h"
#include "jp_primitive_accessor.h"
#include "jp_gc.h"
#include "jp_proxy.h"

#ifdef WIN32
#include <Windows.h>
#endif

// _jpype lifecycle for Python launch:
//   INIT - The init creates all the basic types
//   CONFIG - jpype module then has to be initialized which will then insert addition resources.
//   GC - Garbage collection hooks are installed.
//   PRE - PyJPModule_loadResources is then called to pick up resources typically at startJVM time>
//   START - JVM is started.
//   POST - when the startJVM is completed then PyJPModule_ready is called to get the last resources.
//   RUN - all resources are available
//   STOP - Ends the JVM and frees all resources in Java side releasing Java referenced Python resources.
//   FREE - Module held resources are released.
//
// Bootstrapping order is critical as exception reporting depends on resources that are loaded in the 
// start stage.  Errors between START and POST may be fatal if the exception path is incomplete.

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

#ifdef __cplusplus
extern "C"
{
#endif



static int PyJPModule_clear(PyObject *module);
static inline PyJPModuleState* PyJPState_GET(PyObject* module)
{
	return (PyJPModuleState*) PyModule_GetState(module);
}

static inline PyJPModuleState* PyJPModule_getState(PyObject* module)
{
	return reinterpret_cast<PyJPModuleState*>(PyModule_GetState(module));
}

static PyObject* loadAttr(PyObject* module, const char* name)
{
	PyObject* obj = PyObject_GetAttrString(module, name);
	JP_PY_CHECK();
	return obj;
}

static PyObject* loadAttrFrom(PyObject* obj, const char* name)
{
	PyObject* out = PyObject_GetAttrString(obj, name);
	JP_PY_CHECK();
	return out;
}

// This must only deal with resources from loadResources
static void PyJPModule_clearResources(PyObject *module)
{
	PyJPModuleState *st = reinterpret_cast<PyJPModuleState*>(PyModule_GetState(module));
	if (st == nullptr)
		return;
	Py_CLEAR(st->JObject);
	Py_CLEAR(st->JInterface);
	Py_CLEAR(st->JArray);
	Py_CLEAR(st->JChar);
	Py_CLEAR(st->JException);
	Py_CLEAR(st->JClassPre);
	Py_CLEAR(st->JClassPost);
	Py_CLEAR(st->JClassDoc);
	Py_CLEAR(st->JMethodDoc);
	Py_CLEAR(st->JMethodAnnotations);
	Py_CLEAR(st->JMethodCode);
	Py_CLEAR(st->JObjectKey);

	Py_CLEAR(st->concreteDict);
	Py_CLEAR(st->protocolDict);
	Py_CLEAR(st->methodsDict);
	Py_CLEAR(st->cacheDict);
	Py_CLEAR(st->cacheInterfacesDict);
	Py_CLEAR(st->cacheMethodsDict);

	Py_CLEAR(st->abc_sequence);
	Py_CLEAR(st->abc_mapping);
	Py_CLEAR(st->abc_generator);
	Py_CLEAR(st->abc_iterator);
	Py_CLEAR(st->abc_iterable);
	Py_CLEAR(st->abc_coroutine);
	Py_CLEAR(st->abc_awaitable);
	Py_CLEAR(st->abc_set);
	Py_CLEAR(st->abc_collection);
	Py_CLEAR(st->abc_container);

	Py_CLEAR(st->numpy_generic_type);
	Py_CLEAR(st->numpy_bool_type);
	Py_CLEAR(st->numpy_int8_type);
	Py_CLEAR(st->numpy_int16_type);
	Py_CLEAR(st->numpy_int32_type);

	for (int i = 0; i < 15; ++i)
	{
		Py_CLEAR(st->protocol_pipeline[i]);
	}

}

// Run before JVM starts but after jpype has loaded
void PyJPModule_loadResources(PyObject* module, PyJPModuleState *st)
{
	try
	{
		PyJPModule_clearResources(module);

		st->numpy_typepos = 0;
		st->numpy_genericpos = 0;

		// Frontends
		st->JObject = loadAttr(module, "JObject");
		st->JInterface = loadAttr(module, "JInterface");
		st->JArray = loadAttr(module, "JArray");
		st->JChar = loadAttr(module, "JChar");
		st->JException = loadAttr(module, "JException");

		// Class (jpype._jclass)
		st->JClassPre = loadAttr(module, "_jclassPre");
		st->JClassPost = loadAttr(module, "_jclassPost");

		// Cache (jpype._core)
		st->cacheDict = loadAttr(module, "_cache");
		st->cacheInterfacesDict = loadAttr(module, "_cache_interfaces");
		st->cacheMethodsDict = loadAttr(module, "_cache_methods");
		st->methodsDict = loadAttr(module, "_methods");

		// Doc (jpype._jmethod)
		st->JClassDoc = loadAttr(module, "_jclassDoc");
		st->JMethodDoc = loadAttr(module, "getMethodDoc");
		st->JMethodAnnotations = loadAttr(module, "getMethodAnnotations");
		st->JMethodCode = loadAttr(module, "getMethodCode");

		// GC
		JPPyObject gc_mod = JPPyObject::call(PyImport_ImportModule("gc"));
		if (gc_mod.isValid())
		{
			st->python_gc = gc_mod.keep(); // Pin the module to the state
			st->gc_callbacks = loadAttrFrom(st->python_gc, "callbacks");
		}
		else
		{
			PyErr_SetString(PyExc_RuntimeError, "Failed to import Python 'gc' module");
			JP_RAISE_PYTHON();
		}
		st->collect = loadAttr(module, "_collect");

		// Guards	
		st->JObjectKey = PyCapsule_New(module, "constructor key", nullptr);
		JP_PY_CHECK();

		// Bridge (jpype._core)
		st->concreteDict = loadAttr(module, "_concrete");
		st->protocolDict = loadAttr(module, "_protocol");
		JPPyObject abc_module = JPPyObject::accept(PyImport_ImportModule("collections.abc"));
		if (abc_module.isValid())
		{
			st->abc_sequence = loadAttrFrom(abc_module.get(), "Sequence");
			st->abc_mapping = loadAttrFrom(abc_module.get(), "Mapping");
			st->abc_generator = loadAttrFrom(abc_module.get(), "Generator");
			st->abc_iterator = loadAttrFrom(abc_module.get(), "Iterator");
			st->abc_iterable = loadAttrFrom(abc_module.get(), "Iterable");
			st->abc_coroutine = loadAttrFrom(abc_module.get(), "Coroutine");
			st->abc_awaitable = loadAttrFrom(abc_module.get(), "Awaitable");
			st->abc_set = loadAttrFrom(abc_module.get(), "Set");
			st->abc_collection = loadAttrFrom(abc_module.get(), "Collection");
			st->abc_container = loadAttrFrom(abc_module.get(), "Container");
		}
		else
		{
			PyErr_Clear();
		}

		// Numpy
		JPPyObject numpy = JPPyObject::accept(PyImport_ImportModule("numpy"));
		if (numpy.isValid())
		{
			st->numpy_generic_type = PyObject_GetAttrString(numpy.get(), "generic");
			PyErr_Clear();
			st->numpy_bool_type = PyObject_GetAttrString(numpy.get(), "bool_");
			PyErr_Clear();
			st->numpy_int8_type = PyObject_GetAttrString(numpy.get(), "int8");
			PyErr_Clear();
			st->numpy_int16_type = PyObject_GetAttrString(numpy.get(), "int16");
			PyErr_Clear();
			st->numpy_int32_type = PyObject_GetAttrString(numpy.get(), "int32");
			PyErr_Clear();

			if (st->numpy_int32_type != nullptr && st->numpy_generic_type != nullptr)
			{
				st->numpy_typepos =
						PyTuple_GET_SIZE(((PyTypeObject*) st->numpy_int32_type)->tp_mro);
				st->numpy_genericpos =
						PyTuple_GET_SIZE(((PyTypeObject*) st->numpy_generic_type)->tp_mro);
			}
		}
		else
		{
			PyErr_Clear();
		}
	}
	catch (JPypeException&)
	{
		PyJP_SetStringWithCause(PyExc_RuntimeError, "JPype resource is missing");
		JP_RAISE_PYTHON();
	}
}

static int PyJPModule_clear(PyObject *module)
{
	PyJPModuleState *st = reinterpret_cast<PyJPModuleState*>(PyModule_GetState(module));
	if (st == nullptr)
		return 0;

	PyJPModule_clearResources(module);

	// Clear type references
	Py_CLEAR(st->PyJPClass_Type);
	Py_CLEAR(st->PyJPObject_Type);
	Py_CLEAR(st->PyJPException_Type);
	Py_CLEAR(st->PyJPComparable_Type);
	Py_CLEAR(st->PyJPArray_Type);
	Py_CLEAR(st->PyJPArrayPrimitive_Type);
	Py_CLEAR(st->PyJPBuffer_Type);
	Py_CLEAR(st->PyJPChar_Type);
	Py_CLEAR(st->PyJPField_Type);
	Py_CLEAR(st->PyJPMethod_Type);
	Py_CLEAR(st->PyJPMonitor_Type);
	Py_CLEAR(st->PyJPProxy_Type);
	Py_CLEAR(st->PyJPNumberLong_Type);
	Py_CLEAR(st->PyJPNumberFloat_Type);
	Py_CLEAR(st->PyJPNumberBool_Type);
	Py_CLEAR(st->PyJPClassHints_Type);
	Py_CLEAR(st->PyJPPackage_Type);

	// Clear dynamic state properties
	Py_CLEAR(st->class_magic);
	Py_CLEAR(st->Py_JP_CALL);
	Py_CLEAR(st->strings_dict);
	Py_CLEAR(st->package_dict);
	return 0;
}


// Run after JVM starts
static PyObject* PyJPModule_ready(PyObject* self, PyObject* args)
{
	JP_TRACE_IN("PyJPModule_ready");

	auto* st = reinterpret_cast<PyJPModuleState*>(PyModule_GetState(self));
	if (st == nullptr || st->context == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "JPype module state or context is not available.");
		return nullptr;
	}

	JPContext* context = st->context;
	if (context == nullptr || !context->isRunning())
	{
		PyErr_SetString(PyExc_RuntimeError, "JPype engine context is not active.");
		return nullptr;
	}

	if (st == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "Failed to extract extension module state.");
		return nullptr;
	}

	PyObject* module_dict = PyModule_GetDict(self);
	if (module_dict == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "Failed to extract extension module dictionary.");
		return nullptr;
	}

	// Late-bind C++ exception converter callback
	JPPyObject exc_func = JPPyObject::use(PyDict_GetItemString(module_dict, "_pyexc_convert"));
	if (exc_func.isValid())
	{
		if (context->m_PyExcConvert != nullptr)
			Py_DECREF(context->m_PyExcConvert);
		context->m_PyExcConvert = exc_func.keep();
	}

	for (int i = 0; i < 15; ++i)
		Py_CLEAR(st->protocol_pipeline[i]);

	if (st->protocolDict != nullptr)
	{
		static const char* names[] = {
			"callable", "buffer", "sequence", "mapping", "iterable",
			"iter", "generator", "coroutine", "awaitable", "abstract_set",
			"collection", "container", "index", "number", "combinable"
		};

		for (int i = 0; i < 15; ++i)
		{
			PyObject* proto = PyDict_GetItemString(st->protocolDict, names[i]);
			Py_XINCREF(proto);
			st->protocol_pipeline[i] = proto;
		}
	}

	Py_RETURN_NONE;
	JP_TRACE_OUT;
}

PyTypeObject* PyJP_GetNumPyBaseType(PyJPModuleState* st, PyTypeObject* type)
{
	if (st == nullptr || type == nullptr)
		return nullptr;

	PyObject* mro = type->tp_mro;
	if (mro == nullptr || st->numpy_generic_type == nullptr)
		return nullptr;

	Py_ssize_t n = PyTuple_GET_SIZE(mro);

	// 1. Check the Gate using cached generic position
	// If n < 2, it's a raw object/type.
	// If the item at (n - st->numpy_genericpos) isn't generic, it's not NumPy.
	if (st->numpy_genericpos <= 0 || n < st->numpy_genericpos ||
			PyTuple_GET_ITEM(mro, n - st->numpy_genericpos) != st->numpy_generic_type)
		return nullptr;

	// 2. Resolve the concrete base using cached type position.
	// If the user subclassed it, n will be > st->numpy_typepos.
	if (st->numpy_typepos > 0 && n >= st->numpy_typepos)
		return (PyTypeObject*) PyTuple_GET_ITEM(mro, n - st->numpy_typepos);

	// 3. Fallback for types with shallower MROs, like bool_ or generic itself.
	return type;
}


#ifndef ANDROID
extern JNIEnv *Android_JNI_GetEnv();

JPContext* PyJPModule_getContext(PyObject* self)
{
	return reinterpret_cast<PyJPModuleState*>(PyModule_GetState(self))->context;
}

static PyObject* PyJPModule_startup(PyObject* self, PyObject* pyargs)
{
	JP_PY_TRY("PyJPModule_startup");

	auto* st = reinterpret_cast<PyJPModuleState*>(PyModule_GetState(self));
	if (st == nullptr || st->context == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "JPype module state or context is not available.");
		return nullptr;
	}

	JPContext* context = st->context;

	PyObject* vmOpt;
	PyObject* vmPath;
	char ignoreUnrecognized = true;
	char convertStrings = false;
	char interrupt = false;
	PyObject* tmp;

	if (!PyArg_ParseTuple(pyargs, "OO!bbbO", &vmPath, &PyTuple_Type, &vmOpt,
			&ignoreUnrecognized, &convertStrings, &interrupt, &tmp))
		return nullptr;

	if (tmp != Py_None)
	{
		if (!(JPPyString::check(tmp)))
		{
			PyErr_SetString(PyExc_TypeError, "Java jar path must be a string");
			return nullptr;
		}
		delete st->jarTmpPath;
		st->jarTmpPath = new std::string(JPPyString::asStringUTF8(tmp));
	}

	if (!(JPPyString::check(vmPath)))
	{
		PyErr_SetString(PyExc_TypeError, "Java JVM path must be a string");
		return nullptr;
	}

	string cVmPath = JPPyString::asStringUTF8(vmPath);
	JP_TRACE("vmpath", cVmPath);

	StringVector args;
	JPPySequence seq = JPPySequence::use(vmOpt);

	for (int i = 0; i < seq.size(); i++)
	{
		JPPyObject obj(seq[i]);

		if (JPPyString::check(obj.get()))
		{
			// TODO support unicode
			string v = JPPyString::asStringUTF8(obj.get());
			JP_TRACE("arg", v);
			args.emplace_back(v);
		} else
		{
			PyErr_SetString(PyExc_TypeError, "VM Arguments must be strings");
			return nullptr;
		}
	}

	// This section was moved down to make it easier to cover error cases
	if (context->isRunning())
	{
		PyErr_SetString(PyExc_OSError, "JVM is already started");
		return nullptr;
	}

	// install the gc hook
	PyJPModule_installGC(self);
	PyJPModule_loadResources(self, st);
	context->startJVM(cVmPath, args, ignoreUnrecognized != 0, convertStrings != 0, interrupt != 0);

	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static PyObject* PyJPModule_shutdown(PyObject* self, PyObject* pyargs, PyObject* kwargs)
{
	JP_PY_TRY("PyJPModule_shutdown");
	auto* st = reinterpret_cast<PyJPModuleState*>(PyModule_GetState(self));
	if (st == nullptr || st->context == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "JPype module state or context is not available.");
		return nullptr;
	}

	JPContext* context = st->context;
	char destroyJVM = true;
	char freeJVM = true;

	if (!PyArg_ParseTuple(pyargs, "bb", &destroyJVM, &freeJVM))
		return nullptr;

	context->shutdownJVM(destroyJVM, freeJVM);

#ifdef WIN32
	// Thus far this doesn't work on WINDOWS.  The issue is a bug in the JVM
	// is holding the file open and there is no apparent method to close it
	// so that this can succeed
	if (st->jarTmpPath != nullptr)
		remove(st->jarTmpPath->c_str());
#else
	if (st->jarTmpPath != nullptr)
		unlink(st->jarTmpPath->c_str());
#endif
	delete st->jarTmpPath;
	st->jarTmpPath = nullptr;

	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}
#endif

static PyObject* PyJPModule_isStarted(PyObject* module)
{
	JPContext* context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;
	return PyBool_FromLong(context->isRunning());
}

#ifndef ANDROID

static PyObject* PyJPModule_attachThread(PyObject* module)
{
	JP_PY_TRY("PyJPModule_attachThread");
	JPContext* context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;
	context->attachCurrentThread();
	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static PyObject* PyJPModule_attachThreadAsDaemon(PyObject* module)
{
	JP_PY_TRY("PyJPModule_attachThreadAsDaemon");
	JPContext* context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;
	context->attachCurrentThreadAsDaemon();
	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static PyObject* PyJPModule_detachThread(PyObject* module)
{
	JP_PY_TRY("PyJPModule_detachThread");
	JPContext* context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;
	if (context->isRunning())
		context->detachCurrentThread();
	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}
#endif

static PyObject* PyJPModule_isThreadAttached(PyObject* module)
{
	JP_PY_TRY("PyJPModule_isThreadAttached");
	JPContext* context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;
	if (!context->isRunning())
		return PyBool_FromLong(0); // GCOVR_EXCL_LINE
	return PyBool_FromLong(context->isThreadAttached());
	JP_PY_CATCH(nullptr);
}

// Cleanup hook for Py_buffer

static void releaseView(void* view)
{
	if (view != nullptr)
	{
		PyBuffer_Release((Py_buffer*) view);
		delete (Py_buffer*) view;
	}
}

static PyObject* PyJPModule_convertToDirectByteBuffer(PyObject* module, PyObject* args)
{
	JP_PY_TRY("PyJPModule_convertToDirectByteBuffer");

	JPContext* context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;

	JPJavaFrame frame = JPJavaFrame::outer(context);

	PyObject *src;
	int ro;
	if (!PyArg_ParseTuple(args, "Op", &src, &ro))
		return nullptr;

	if (PyObject_CheckBuffer(src))
	{
		JPViewWrapper vw;
		if (PyObject_GetBuffer(src, vw.view, ro ? 0 : PyBUF_WRITABLE) == -1)
			return nullptr;

		jvalue v;
		v.l = frame.NewDirectByteBuffer(vw.view->buf, vw.view->len);

		if (vw.view->readonly)
			v.l = frame.asReadOnlyBuffer(v.l);

		frame.registerRef(v.l, vw.view, &releaseView);
		vw.view = nullptr;

		JPClass *type = frame.findClassForObject(v.l);
		return type->convertToPythonObject(frame, v, false).keep();
	}

	PyErr_SetString(PyExc_TypeError, "convertToDirectByteBuffer requires buffer support");
	JP_PY_CATCH(nullptr);
}

static PyObject* PyJPModule_enableStacktraces(PyObject* module, PyObject* src)
{
	auto* st = PyJPModule_getState(module);
	if (st == nullptr)
		return nullptr;

	st->cpp_exceptions = PyObject_IsTrue(src);
	if (st->cpp_exceptions == -1)
		return nullptr;

	Py_RETURN_TRUE;
}

PyObject *PyJPModule_newArrayType(PyObject *module, PyObject *args)
{
	JP_PY_TRY("PyJPModule_newArrayType");

	JPContext* context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;

	JPJavaFrame frame = JPJavaFrame::outer(context);

	PyObject *type, *dims;
	if (!PyArg_ParseTuple(args, "OO", &type, &dims))
		return nullptr;

	if (!PyIndex_Check(dims))
	{
		PyErr_SetString(PyExc_TypeError, "dims must be an integer");
		return nullptr;
	}

	long d = PyLong_AsLong(dims);
	if (d == -1 && PyErr_Occurred())
		return nullptr;

	JPClass* cls = PyJPClass_getJPClass(type);
	if (cls == nullptr)
	{
		PyErr_SetString(PyExc_TypeError, "Java class required");
		return nullptr;
	}

	JPClass* arraycls = cls->newArrayType(frame, d);
	return PyJPClass_create(frame, arraycls).keep();
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPModule_getClass(PyObject* module, PyObject *obj)
{
	JP_PY_TRY("PyJPModule_getClass");

	JPContext* context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;

	JPJavaFrame frame = JPJavaFrame::outer(context);

	JPClass* cls;
	if (JPPyString::check(obj))
	{
		// String From Python
		cls = frame.findClassByName(JPPyString::asStringUTF8(obj));
		if (cls == nullptr)
		{
			PyErr_SetString(PyExc_ValueError, "Unable to find Java class");
			return nullptr;
		}
	}
	else
	{
		// From an existing java.lang.Class object
		JPValue *value = PyJPValue_getJavaSlot(obj);
		if (value == nullptr || value->getClass() != context->_java_lang_Class)
		{
			PyErr_Format(PyExc_TypeError,
					"JClass requires str or java.lang.Class instance, not '%s'",
					Py_TYPE(obj)->tp_name);
			return nullptr;
		}

		cls = frame.findClass((jclass) value->getJavaObject(frame));
		if (cls == nullptr)
		{
			PyErr_SetString(PyExc_ValueError, "Unable to find class");
			return nullptr;
		}
	}

	return PyJPClass_create(frame, cls).keep();
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPModule_hasClass(PyObject* self, PyObject *obj)
{
	JP_PY_TRY("PyJPModule_hasClass");
	auto* st = reinterpret_cast<PyJPModuleState*>(PyModule_GetState(self));
	if (st == nullptr || st->context == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "JPype module state or context is not available.");
		return nullptr;
	}

	JPContext* context = st->context;
	if (!context->isRunning())
		Py_RETURN_FALSE; // GCOVR_EXCL_LINE

	if (context == nullptr)
		return nullptr;
	JPJavaFrame frame = JPJavaFrame::outer(context);

	JPClass* cls;
	if (JPPyString::check(obj))
	{
		// String From Python
		cls = frame.findClassByName(JPPyString::asStringUTF8(obj));
		if (cls == nullptr)
		{
			PyErr_SetString(PyExc_ValueError, "Unable to find Java class");
			return nullptr;
		}
	} else
	{
		PyErr_Format(PyExc_TypeError, "str is required, not '%s'", Py_TYPE(obj)->tp_name);
		return nullptr;
	}

	auto *host = (PyObject*) cls->getHost();
	return PyBool_FromLong(host != nullptr);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPModule_arrayFromBuffer(PyObject *module, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPModule_arrayFromBuffer");

	PyObject *source = nullptr;
	PyObject *dtype = nullptr;
	if (!PyArg_ParseTuple(args, "OO", &source, &dtype))
		return nullptr;

	if (!PyObject_CheckBuffer(source))
	{
		PyErr_Format(PyExc_TypeError, "'%s' does not support buffers", Py_TYPE(source)->tp_name);
		return nullptr;
	}

	JPContext* context = PyJPModule_getContext(module);
	PyJPModuleState* st = context->modulestate;

	// NUMPy does a series of probes looking for the best supported format,
	// we will do the same.
	{
		JPPyBuffer buffer(source, PyBUF_FULL_RO);
		if (buffer.valid())
			return PyJPModule_convertBuffer(st, buffer, dtype);
	}
	{
		JPPyBuffer buffer(source, PyBUF_RECORDS_RO);
		if (buffer.valid())
			return PyJPModule_convertBuffer(st, buffer, dtype);
	}
	{
		JPPyBuffer buffer(source, PyBUF_ND | PyBUF_FORMAT);
		if (buffer.valid())
			return PyJPModule_convertBuffer(st, buffer, dtype);
	}

	PyErr_Format(PyExc_TypeError, "buffer protocol for '%s' not supported", Py_TYPE(source)->tp_name);
	return nullptr;
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPModule_collect(PyObject* module, PyObject *obj)
{
	JPContext* context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;

	if (!context->isRunning())
		Py_RETURN_NONE;

	PyObject *a1 = PyTuple_GetItem(obj, 0);
	if (!PyUnicode_Check(a1))
	{
		PyErr_SetString(PyExc_TypeError, "Bad callback argument");
		return nullptr;
	}

	if (PyUnicode_ReadChar(a1, 2) == 'a')
		context->m_GC->onStart();
	else
		context->m_GC->onEnd();

	Py_RETURN_NONE;
}

// GCOVR_EXCL_START

static PyObject *PyJPModule_gcStats(PyObject* module, PyObject *obj)
{
	JPContext *context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;

	JPGCStats stats;
	context->m_GC->getStats(stats);

	// 1. Secure the dictionary allocation immediately
	JPPyObject out = JPPyObject::call(PyDict_New());

	// 2. Explicitly sequence the allocations on the stack.
	//	Each container is guaranteed to stay alive until the loop/function block ends,
	//	meaning the reference count remains perfectly stable while PyDict runs.
	JPPyObject value = JPPyObject::call(PyLong_FromSsize_t((Py_ssize_t) stats.current_rss));
	PyDict_SetItemString(out.get(), "current", value.get());
	value = JPPyObject::call(PyLong_FromSsize_t((Py_ssize_t) stats.java_rss));
	PyDict_SetItemString(out.get(), "java", value.get());
	value = JPPyObject::call(PyLong_FromSsize_t((Py_ssize_t) stats.python_rss));
	PyDict_SetItemString(out.get(), "python", value.get());
	value = JPPyObject::call(PyLong_FromSsize_t((Py_ssize_t) stats.max_rss));
	PyDict_SetItemString(out.get(), "max", value.get());
	value = JPPyObject::call(PyLong_FromSsize_t((Py_ssize_t) stats.min_rss));
	PyDict_SetItemString(out.get(), "min", value.get());
	value = JPPyObject::call(PyLong_FromSsize_t((Py_ssize_t) stats.python_triggered));
	PyDict_SetItemString(out.get(), "triggered", value.get());

	return out.keep();
}

// GCOVR_EXCL_STOP

static PyObject* PyJPModule_isPackage(PyObject *module, PyObject *pkg)
{
	JP_PY_TRY("PyJPModule_isPackage");
	if (!PyUnicode_Check(pkg))
	{
		PyErr_Format(PyExc_TypeError, "isPackage required unicode");
		return nullptr;
	}

	JPContext* context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;
	JPJavaFrame frame = JPJavaFrame::outer(context);
	return PyBool_FromLong(frame.isPackage(JPPyString::asStringUTF8(pkg)));
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

/** Code to determine what interfaces are required based on the
 * dunder methods.
 */
static PyObject* module_probe(PyObject *module, PyObject *obj)
{
	JP_PY_TRY("probe");

	PyJPModuleState* st = PyJPModule_getState(module);
	if (st == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "JPype module state is not available");
		return nullptr;
	}

	PyTypeObject *type = Py_TYPE(obj);
	PyObject* result = PyJP_probe(st, type);

	// Safety check: If a nullptr comes back, ensure the exception state is set
	if (result == nullptr)
	{
		if (!PyErr_Occurred())
		{
			PyErr_Format(PyExc_RuntimeError,
				"JPype Engine Error: Probe failed for type '%s' without setting an explicit exception.",
				type->tp_name);
		}
		return nullptr;
	}

	return result;
	JP_PY_CATCH(nullptr);
}

static PyObject* module_pyobject(PyObject *module, PyObject *args_in)
{
	JP_PY_TRY("PyJPModule_pyobject");

	auto* st = PyJPModule_getState(module);
	if (st == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "JPype module state is not available");
		return nullptr;
	}

	PyObject* target_type = nullptr;
	PyObject* object_to_cast = nullptr;

	// Parse the incoming arguments: Type first, Object second
	// Note: PyArg_ParseTuple sets its own exception (TypeError) on failure,
	// so returning nullptr here is perfectly safe.
	if (!PyArg_ParseTuple(args_in, "OO", &target_type, &object_to_cast))
		return nullptr;

	// Execute the core bridge casting routine
	PyObject* result = PyJP_pyobject(st, (PyTypeObject*) target_type, object_to_cast);

	// Defensive Guard: Ensure an exception is set if a nullptr bubbles up
	if (result == nullptr)
	{
		if (!PyErr_Occurred())
		{
			PyTypeObject* target_py_type = (PyTypeObject*) target_type;
			PyTypeObject* source_py_type = Py_TYPE(object_to_cast);
			PyErr_Format(PyExc_TypeError,
				"JPype Bridge Error: Failed to cast Python object of type '%s' to target Java proxy type '%s' without setting an explicit exception.",
				source_py_type->tp_name,
				target_py_type->tp_name);
		}
		return nullptr;
	}

	return result;
	JP_PY_CATCH(nullptr);
}

#if 1
// GCOVR_EXCL_START
// This code was used in testing the Java slot memory layout.  It serves no purpose outside of debugging that issue.
PyObject* examine(PyObject *module, PyObject *other)
{
	(void) module;
	JP_PY_TRY("examine");
	int ret = 0;
	PyTypeObject *type;
	if (PyType_Check(other))
		type = (PyTypeObject*) other;
	else
		type = Py_TYPE(other);

	printf("======\n");
	int offset = 0;
	if (!PyType_Check(other))
	{
		offset = PyJPValue_getJavaSlotOffset(other);
		printf("  Object:\n");
		printf("	size: %d\n", (int) Py_SIZE(other));
		printf("	dictoffset: %d\n", (int) ((long long) _PyObject_GetDictPtr(other)-(long long) other));
		printf("	javaoffset: %d\n", offset);
	}
	printf("  Type: %p\n", type);
	printf("	name: %s\n", type->tp_name);
	printf("	typename: %s\n", Py_TYPE(type)->tp_name);
	printf("	gc: %d\n", (type->tp_flags & Py_TPFLAGS_HAVE_GC) == Py_TPFLAGS_HAVE_GC);
	printf("	basicsize: %d\n", (int) type->tp_basicsize);
	printf("	itemsize: %d\n", (int) type->tp_itemsize);
	printf("	dictoffset: %d\n", (int) type->tp_dictoffset);
	printf("	weaklistoffset: %d\n", (int) type->tp_weaklistoffset);
	printf("	hasJavaSlot: %d\n", PyJPValue_hasJavaSlot(type));
	printf("	getattro: %p\n", type->tp_getattro);
	printf("	setattro: %p\n", type->tp_setattro);
	printf("	getattr: %p\n", type->tp_getattr);
	printf("	setattr: %p\n", type->tp_setattr);
	printf("	alloc: %p\n", type->tp_alloc);
	printf("	free: %p\n", type->tp_free);
	printf("	finalize: %p\n", type->tp_finalize);
	long v = _PyObject_VAR_SIZE(type, 1)+(PyJPValue_hasJavaSlot(type)?sizeof (JPValue):0);
	printf("	size?: %ld\n",v);
	printf("======\n");

	return PyBool_FromLong(ret);
	JP_PY_CATCH(nullptr);
}
// GCOVR_EXCL_STOP
#endif

// GCOVR_EXCL_START

// This one is used only in debugging and does not need to be subinterpreter safe
int _PyJPModule_trace;
static PyObject* PyJPModule_trace(PyObject *module, PyObject *args)
{
	long value = PyLong_AsLong(args);
	if (value == -1 && PyErr_Occurred())
		return nullptr;

	long old = _PyJPModule_trace;
	_PyJPModule_trace = (int) value;
	return PyLong_FromLong(old);
}
// GCOVR_EXCL_STOP

#ifdef JP_INSTRUMENTATION
extern uint32_t _PyJPModule_fault_code;
static PyObject* PyJPModule_fault(PyObject *module, PyObject *args)
{
	if (args == Py_None)
	{
		_PyJPModule_fault_code = 0;
		Py_RETURN_NONE;
	}

	string code = JPPyString::asStringUTF8(args);
	uint32_t u = 0;
	for (size_t i = 0; i < code.size(); ++i)
		u = u * 0x1a481023 + code[i];

	_PyJPModule_fault_code = u;
	return PyLong_FromUnsignedLong((unsigned long) _PyJPModule_fault_code);
}
#endif

#ifdef ANDROID
static PyObject *PyJPModule_bootstrap(PyObject *module)
{
	// After all the internals are created we can connect the API with the internal module
	JNIEnv * env = Android_JNI_GetEnv();
	JPContext_global->attachJVM(env);
	PyJPModule_installGC(module);
	PyJPModule_loadResources(module, st);
	Py_RETURN_NONE;
}
#endif

static PyObject *PyJPModule_context(PyObject* module, PyObject *obj)
{
	JPContext *context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;
	JPJavaFrame frame = JPJavaFrame::outer(context);
	jvalue v;
	v.l = context->getJavaContext();
	return context->_java_lang_Object->convertToPythonObject(frame, v, false).keep();
}

static PyObject *PyJPModule_interpreter(PyObject* module, PyObject *obj)
{
	JPContext *context = PyJPModule_getContext(module);
	if (context == nullptr)
		return nullptr;
	JPJavaFrame frame = JPJavaFrame::outer(context);
	jvalue v;
	v.l = context->m_Interpreter;
	return context->_java_lang_Object->convertToPythonObject(frame, v, false).keep();
}


static int PyJPModule_traverse(PyObject *module, visitproc visit, void *arg)
{
	PyJPModuleState *st = reinterpret_cast<PyJPModuleState*>(PyModule_GetState(module));
	if (st == nullptr)
		return 0;

	// Traverse all the type object references
	Py_VISIT(st->PyJPClass_Type);
	Py_VISIT(st->PyJPObject_Type);
	Py_VISIT(st->PyJPException_Type);
	Py_VISIT(st->PyJPComparable_Type);
	Py_VISIT(st->PyJPArray_Type);
	Py_VISIT(st->PyJPArrayPrimitive_Type);
	Py_VISIT(st->PyJPBuffer_Type);
	Py_VISIT(st->PyJPChar_Type);
	Py_VISIT(st->PyJPField_Type);
	Py_VISIT(st->PyJPMethod_Type);
	Py_VISIT(st->PyJPMonitor_Type);
	Py_VISIT(st->PyJPProxy_Type);
	Py_VISIT(st->PyJPNumberLong_Type);
	Py_VISIT(st->PyJPNumberFloat_Type);
	Py_VISIT(st->PyJPNumberBool_Type);
	Py_VISIT(st->PyJPClassHints_Type);
	Py_VISIT(st->PyJPPackage_Type);

	// Traverse runtime helpers and internal hooks
	Py_VISIT(st->class_magic);
	Py_VISIT(st->Py_JP_CALL);
	Py_VISIT(st->strings_dict);

	Py_VISIT(st->JObject);
	Py_VISIT(st->JInterface);
	Py_VISIT(st->JArray);
	Py_VISIT(st->JChar);
	Py_VISIT(st->JException);
	Py_VISIT(st->JClassPre);
	Py_VISIT(st->JClassPost);
	Py_VISIT(st->JClassDoc);
	Py_VISIT(st->JMethodDoc);
	Py_VISIT(st->JMethodAnnotations);
	Py_VISIT(st->JMethodCode);
	Py_VISIT(st->JObjectKey);

	// Traverse internal dictionaries
	Py_VISIT(st->concreteDict);
	Py_VISIT(st->protocolDict);
	Py_VISIT(st->methodsDict);
	Py_VISIT(st->cacheDict);
	Py_VISIT(st->cacheInterfacesDict);
	Py_VISIT(st->cacheMethodsDict);
	Py_VISIT(st->package_dict);

	// Traverse collections and collections.abc wrappers
	Py_VISIT(st->abc_sequence);
	Py_VISIT(st->abc_mapping);
	Py_VISIT(st->abc_generator);
	Py_VISIT(st->abc_iterator);
	Py_VISIT(st->abc_iterable);
	Py_VISIT(st->abc_coroutine);
	Py_VISIT(st->abc_awaitable);
	Py_VISIT(st->abc_set);
	Py_VISIT(st->abc_collection);
	Py_VISIT(st->abc_container);

	// Traverse NumPy integration handles
	Py_VISIT(st->numpy_generic_type);
	Py_VISIT(st->numpy_bool_type);
	Py_VISIT(st->numpy_int8_type);
	Py_VISIT(st->numpy_int16_type);
	Py_VISIT(st->numpy_int32_type);

	// Traverse the protocol pipeline array
	for (int i = 0; i < 15; ++i)
	{
		Py_VISIT(st->protocol_pipeline[i]);
	}

	return 0;
}

static void PyJPModule_free(void *module)
{
	PyObject *mod_obj = reinterpret_cast<PyObject*>(module);
	PyJPModuleState *st = reinterpret_cast<PyJPModuleState*>(PyModule_GetState(mod_obj));
	if (st != nullptr)
	{
		// Clean up the execution loop layers
		PyJPModule_clear(mod_obj);

		// We can destroy the context here
		if (st->context != nullptr)
		{
			delete st->context;
			st->context = nullptr;
		}

		delete st->jarTmpPath;
		st->jarTmpPath = nullptr;
	}
}

static PyMethodDef moduleMethods[] = {
	// Startup and initialization
	{"isStarted", (PyCFunction) PyJPModule_isStarted, METH_NOARGS, ""},
#ifdef ANDROID
	{"bootstrap", (PyCFunction) PyJPModule_bootstrap, METH_NOARGS, ""},
#else
	{"startup", (PyCFunction) PyJPModule_startup, METH_VARARGS, ""},
	{"ready", (PyCFunction) PyJPModule_ready, METH_NOARGS, "Signal bootstrap completion" },
	{"shutdown", (PyCFunction) PyJPModule_shutdown, METH_VARARGS, ""},
#endif
	{"_getClass", (PyCFunction) PyJPModule_getClass, METH_O, ""},
	{"_hasClass", (PyCFunction) PyJPModule_hasClass, METH_O, ""},
	{"_newArrayType", (PyCFunction) PyJPModule_newArrayType, METH_VARARGS, ""},
	{"_collect", (PyCFunction) PyJPModule_collect, METH_VARARGS, ""},
	{"gcStats", (PyCFunction) PyJPModule_gcStats, METH_NOARGS, ""},
	{"context", (PyCFunction) PyJPModule_context, METH_NOARGS, ""},
	{"interpreter", (PyCFunction) PyJPModule_interpreter, METH_NOARGS, ""},

	// Threading
	{"isThreadAttachedToJVM", (PyCFunction) PyJPModule_isThreadAttached, METH_NOARGS, ""},
#ifndef ANDROID
	{"attachThreadToJVM", (PyCFunction) PyJPModule_attachThread, METH_NOARGS, ""},
	{"detachThreadFromJVM", (PyCFunction) PyJPModule_detachThread, METH_NOARGS, ""},
	{"attachThreadAsDaemon", (PyCFunction) PyJPModule_attachThreadAsDaemon, METH_NOARGS, ""},
#endif

	//{"dumpJVMStats", (PyCFunction) (&PyJPModule_dumpJVMStats), METH_NOARGS, ""},

	{"convertToDirectBuffer", (PyCFunction) PyJPModule_convertToDirectByteBuffer, METH_VARARGS, ""},
	{"arrayFromBuffer", (PyCFunction) PyJPModule_arrayFromBuffer, METH_VARARGS, ""},
	{"enableStacktraces", (PyCFunction) PyJPModule_enableStacktraces, METH_O, ""},
	{"isPackage", (PyCFunction) PyJPModule_isPackage, METH_O, ""},
	{"trace", (PyCFunction) PyJPModule_trace, METH_O, ""},
#ifdef JP_INSTRUMENTATION
	{"fault", (PyCFunction) PyJPModule_fault, METH_O, ""},
#endif
	{"examine", (PyCFunction) examine, METH_O, ""},
	{"probe", (PyCFunction) module_probe, METH_O, ""},
	{"pyobject", (PyCFunction) module_pyobject, METH_VARARGS, ""},

	// sentinel
	{nullptr}
};

static int PyJPModule_exec(PyObject* module);

static PyModuleDef_Slot jpype_slots[] = {
	{Py_mod_exec, (void*) PyJPModule_exec},
#if PY_VERSION_HEX>=0x030c0000
	// Declares eligibility for a genuinely isolated (own-GIL) subinterpreter
	// config. Requires multi-phase init (this slot array) to even be
	// offered - see plan/MultiPhaseInit.md.
	{Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#endif
#ifdef Py_GIL_DISABLED
	// Declare-as-a-slot replaces the old post-hoc PyUnstable_Module_SetGIL
	// call now that multi-phase init makes a real slot available; only
	// defined by CPython headers new enough to have both this macro and
	// Py_mod_gil (3.13+), so no separate PY_VERSION_HEX guard is needed.
	{Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
	{0, nullptr}
};

static struct PyModuleDef moduledef = {
	PyModuleDef_HEAD_INIT,
	"_jpype",
	"jpype module",
	sizeof(PyJPModuleState),
	moduleMethods,
	jpype_slots,
	PyJPModule_traverse,
	PyJPModule_clear,
	PyJPModule_free
};

PyMODINIT_FUNC PyInit__jpype()
{
	return PyModuleDef_Init(&moduledef);
}

static int PyJPModule_exec(PyObject* module)
{
	JP_PY_TRY("PyJPModule_exec");

	auto* st = PyJPModule_getState(module);
	if (st == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "Failed to allocate JPype module state");
		return -1;
	}

	// Initialize module state
	memset(st, 0, sizeof(PyJPModuleState));
	st->module_dict = PyModule_GetDict(module);
	st->interp_state = PyThreadState_Get()->interp;
#if PY_VERSION_HEX>=0x030c0000
	// Mark if this is the main interpreter for Python 3.12+
	st->is_main_interpreter = (st->interp_state == PyInterpreterState_Main());
#else
	st->is_main_interpreter = true;  // Python < 3.12 only has main interpreter
#endif
	st->count = 1;
	st->held = 1;

	// TODO: we should probably pass the version directly from a scikit-build (cmake) defined macro.
	PyModule_AddStringConstant(module, "__version__", "1.7.2.dev0");

	// Our module will be used for PyFrame object and it is a requirement that
	// we have a builtins in our dictionary.
	PyObject *builtins = PyEval_GetBuiltins();
	Py_INCREF(builtins);
	PyModule_AddObject(module, "__builtins__", builtins);

	st->strings_dict = PyDict_New();
	JP_PY_CHECK();
	// Add to module so it's visible/reachable and cleared on shutdown
	PyModule_AddObject(module, "_strings", st->strings_dict);
	Py_INCREF(st->strings_dict);

	st->Py_JP_CALL = PyUnicode_InternFromString("__call__");
	JP_PY_CHECK();

	st->class_magic = PyDict_New();
	JP_PY_CHECK();

	// Initialize each of the python extension types
	PyJPClass_initType(module, st);
	PyJPObject_initType(module, st);
	PyJPArray_initType(module, st);
	PyJPBuffer_initType(module, st);
	PyJPField_initType(module, st);
	PyJPMethod_initType(module, st);
	PyJPNumber_initType(module, st);
	PyJPMonitor_initType(module, st);
	PyJPProxy_initType(module, st);
	PyJPClassHints_initType(module, st);
	PyJPPackage_initType(module, st);
	PyJPChar_initType(module, st);

	st->context = new JPContext(st);
	return 0;
	JP_PY_CATCH(-1); // GCOVR_EXCL_LINE
}

#ifdef __cplusplus
}
#endif
