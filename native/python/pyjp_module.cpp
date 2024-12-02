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
#include "jp_arrayclass.h"
#include "jp_primitive_accessor.h"
#include "jp_gc.h"
#include "jp_stringtype.h"
#include "jp_classloader.h"
#ifdef WIN32
#include <Windows.h>
#endif

void PyJPModule_installGC(PyObject* module);

bool _jp_cpp_exceptions = false;

extern void PyJPArray_initType(PyObject* module);
extern void PyJPBuffer_initType(PyObject* module);
extern void PyJPClass_initType(PyObject* module);
extern void PyJPField_initType(PyObject* module);
extern void PyJPMethod_initType(PyObject* module);
extern void PyJPMonitor_initType(PyObject* module);
extern void PyJPProxy_initType(PyObject* module);
extern void PyJPObject_initType(PyObject* module);
extern void PyJPNumber_initType(PyObject* module);
extern void PyJPClassHints_initType(PyObject* module);
extern void PyJPPackage_initType(PyObject* module);
extern void PyJPChar_initType(PyObject* module);
extern void PyJPValue_initType(PyObject* module);

static PyObject *PyJPModule_convertBuffer(JPPyBuffer& buffer, PyObject *dtype);

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


PyObject* _JArray = nullptr;
PyObject* _JChar = nullptr;
PyObject* _JObject = nullptr;
PyObject* _JInterface = nullptr;
PyObject* _JException = nullptr;
PyObject* _JClassPre = nullptr;
PyObject* _JClassPost = nullptr;
PyObject* _JClassDoc = nullptr;
PyObject* _JMethodDoc = nullptr;
PyObject* _JMethodAnnotations = nullptr;
PyObject* _JMethodCode = nullptr;
PyObject* _JObjectKey = nullptr;
PyObject* _JVMNotRunning = nullptr;

void PyJPModule_loadResources(PyObject* module)
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
		_JChar = PyObject_GetAttrString(module, "JChar");
		JP_PY_CHECK();
		Py_INCREF(_JChar);
		_JException = PyObject_GetAttrString(module, "JException");
		JP_PY_CHECK();
		Py_INCREF(_JException);
		_JClassPre = PyObject_GetAttrString(module, "_jclassPre");
		JP_PY_CHECK();
		Py_INCREF(_JClassPre);
		_JClassPost = PyObject_GetAttrString(module, "_jclassPost");
		JP_PY_CHECK();
		Py_INCREF(_JClassPost);
		JP_PY_CHECK();
		_JClassDoc = PyObject_GetAttrString(module, "_jclassDoc");
		JP_PY_CHECK();
		Py_INCREF(_JClassDoc);
		_JMethodDoc = PyObject_GetAttrString(module, "getMethodDoc");
		Py_INCREF(_JMethodDoc);
		_JMethodAnnotations = PyObject_GetAttrString(module, "getMethodAnnotations");
		JP_PY_CHECK();
		Py_INCREF(_JMethodAnnotations);
		_JMethodCode = PyObject_GetAttrString(module, "getMethodCode");
		JP_PY_CHECK();
		Py_INCREF(_JMethodCode);

		_JObjectKey = PyCapsule_New(module, "constructor key", nullptr);

	}	catch (JPypeException&)  // GCOVR_EXCL_LINE
	{
		// GCOVR_EXCL_START
		PyJP_SetStringWithCause(PyExc_RuntimeError, "JPype resource is missing");
		JP_RAISE_PYTHON();
		// GCOVR_EXCL_STOP
	}
}

#ifdef __cplusplus
extern "C"
{
#endif


// GCOVR_EXCL_START
// This is used exclusively during startup

void PyJP_SetStringWithCause(PyObject *exception,
		const char *str)
{
	// See _PyErr_TrySetFromCause
	PyObject *exc1, *val1, *tb1;
	PyErr_Fetch(&exc1, &val1, &tb1);
	PyErr_NormalizeException(&exc1, &val1, &tb1);
	if (tb1 != nullptr)
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
// GCOVR_EXCL_STOP

PyObject* PyJP_GetAttrDescriptor(PyTypeObject *type, PyObject *attr_name)
{
	JP_PY_TRY("Py_GetAttrDescriptor");
	if (type->tp_mro == nullptr)
		return nullptr;  // GCOVR_EXCL_LINE

	// Grab the mro
	PyObject *mro = type->tp_mro;

	// mro should be a tuple
	Py_ssize_t n = PyTuple_Size(mro);

	// Search the tuple for the attribute
	for (Py_ssize_t i = 0; i < n; ++i)
	{
		auto *type2 = (PyTypeObject*) PyTuple_GetItem(mro, i);

		// Skip objects without a functioning dictionary
		if (type2->tp_dict == NULL)
			continue;

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

	return nullptr;
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

int PyJP_IsSubClassSingle(PyTypeObject* type, PyTypeObject* obj)
{
	if (type == nullptr || obj == nullptr)
		return 0;  // GCOVR_EXCL_LINE
	PyObject* mro1 = obj->tp_mro;
	Py_ssize_t n1 = PyTuple_Size(mro1);
	Py_ssize_t n2 = PyTuple_Size(type->tp_mro);
	if (n1 < n2)
		return 0;
	return PyTuple_GetItem(mro1, n1 - n2) == (PyObject*) type;
}

int PyJP_IsInstanceSingle(PyObject* obj, PyTypeObject* type)
{
	if (type == nullptr || obj == nullptr)
		return 0; // GCOVR_EXCL_LINE
	return PyJP_IsSubClassSingle(type, Py_TYPE(obj));
}

#ifndef ANDROID
extern JNIEnv *Android_JNI_GetEnv();

static string jarTmpPath;
static PyObject* PyJPModule_startup(PyObject* module, PyObject* pyargs)
{
	JP_PY_TRY("PyJPModule_startup");

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
		jarTmpPath = JPPyString::asStringUTF8(tmp);
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
	if (JPContext_global->isRunning())
	{
		PyErr_SetString(PyExc_OSError, "JVM is already started");
		return nullptr;
	}

	// install the gc hook
	PyJPModule_installGC(module);
	PyJPModule_loadResources(module);
	JPContext_global->startJVM(cVmPath, args, ignoreUnrecognized != 0, convertStrings != 0, interrupt != 0);

	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static PyObject* PyJPModule_shutdown(PyObject* obj, PyObject* pyargs, PyObject* kwargs)
{
	JP_PY_TRY("PyJPModule_shutdown");
	char destroyJVM = true;
	char freeJVM = true;

	if (!PyArg_ParseTuple(pyargs, "bb", &destroyJVM, &freeJVM))
		return nullptr;

	JPContext_global->shutdownJVM(destroyJVM, freeJVM);

#ifdef WIN32
	// Thus far this doesn't work on WINDOWS.  The issue is a bug in the JVM
	// is holding the file open and there is no apparent method to close it
	// so that this can succeed
	if (jarTmpPath != "")
		remove(jarTmpPath.c_str());
#else
	if (jarTmpPath != "")
		unlink(jarTmpPath.c_str());
#endif

	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}
#endif

static PyObject* PyJPModule_isStarted(PyObject* obj)
{
	return PyBool_FromLong(JPContext_global->isRunning());
}

#ifndef ANDROID

static PyObject* PyJPModule_attachThread(PyObject* obj)
{
	JP_PY_TRY("PyJPModule_attachThread");
	PyJPModule_getContext()->attachCurrentThread();
	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static PyObject* PyJPModule_attachThreadAsDaemon(PyObject* obj)
{
	JP_PY_TRY("PyJPModule_attachThreadAsDaemon");
	PyJPModule_getContext()->attachCurrentThreadAsDaemon();
	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}

static PyObject* PyJPModule_detachThread(PyObject* obj)
{
	JP_PY_TRY("PyJPModule_detachThread");
	if (JPContext_global->isRunning())
		JPContext_global->detachCurrentThread();
	Py_RETURN_NONE;
	JP_PY_CATCH(nullptr);
}
#endif

static PyObject* PyJPModule_isThreadAttached(PyObject* obj)
{
	JP_PY_TRY("PyJPModule_isThreadAttached");
	if (!JPContext_global->isRunning())
		return PyBool_FromLong(0); // GCOVR_EXCL_LINE
	return PyBool_FromLong(JPContext_global->isThreadAttached());
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

static PyObject* PyJPModule_convertToDirectByteBuffer(PyObject* self, PyObject* src)
{
	JP_PY_TRY("PyJPModule_convertToDirectByteBuffer");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);

	if (PyObject_CheckBuffer(src))
	{
		JPViewWrapper vw;
		if (PyObject_GetBuffer(src, vw.view, PyBUF_WRITABLE) == -1)
			return nullptr;

		// Create a byte buffer
		jvalue v;
		v.l = frame.NewDirectByteBuffer(vw.view->buf, vw.view->len);

		// Bind lifespan of the view to the java object.
		frame.registerRef(v.l, vw.view, &releaseView);
		vw.view = nullptr;
		JPClass *type = frame.findClassForObject(v.l);
		return type->convertToPythonObject(frame, v, false).keep();
	}
	PyErr_SetString(PyExc_TypeError, "convertToDirectByteBuffer requires buffer support");
	JP_PY_CATCH(nullptr);
}

static PyObject* PyJPModule_enableStacktraces(PyObject* self, PyObject* src)
{
	_jp_cpp_exceptions = PyObject_IsTrue(src);
	Py_RETURN_TRUE;
}

PyObject *PyJPModule_newArrayType(PyObject *module, PyObject *args)
{
	JP_PY_TRY("PyJPModule_newArrayType");
	JPContext *context = PyJPModule_getContext();
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

PyObject *PyJPModule_getClass(PyObject* module, PyObject *obj)
{
	JP_PY_TRY("PyJPModule_getClass");
	JPContext *context = PyJPModule_getContext();
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
		// From an existing java.lang.Class object
		JPValue *value = PyJPValue_getJavaSlot(obj);
		if (value == nullptr || value->getClass() != context->_java_lang_Class)
		{
			PyErr_Format(PyExc_TypeError, "JClass requires str or java.lang.Class instance, not '%s'", Py_TYPE(obj)->tp_name);
			return nullptr;
		}
		cls = frame.findClass((jclass) value->getValue().l);
		if (cls == nullptr)
		{
			PyErr_SetString(PyExc_ValueError, "Unable to find class");
			return nullptr;
		}
	}

	return PyJPClass_create(frame, cls).keep();
	JP_PY_CATCH(nullptr);
}

PyObject *PyJPModule_hasClass(PyObject* module, PyObject *obj)
{
	JP_PY_TRY("PyJPModule_hasClass");
	if (!JPContext_global->isRunning())
		Py_RETURN_FALSE; // GCOVR_EXCL_LINE
	JPContext *context = PyJPModule_getContext();
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

	// NUMPy does a series of probes looking for the best supported format,
	// we will do the same.
	{
		JPPyBuffer	buffer(source, PyBUF_FULL_RO);
		if (buffer.valid())
			return PyJPModule_convertBuffer(buffer, dtype);
	}
	{
		JPPyBuffer	buffer(source, PyBUF_RECORDS_RO);
		if (buffer.valid())
			return PyJPModule_convertBuffer(buffer, dtype);
	}
	{
		JPPyBuffer	buffer(source, PyBUF_ND | PyBUF_FORMAT);
		if (buffer.valid())
			return PyJPModule_convertBuffer(buffer, dtype);
	}
	PyErr_Format(PyExc_TypeError, "buffer protocol for '%s' not supported", Py_TYPE(source)->tp_name);
	return nullptr;
	JP_PY_CATCH(nullptr);
}

PyObject *PyJPModule_collect(PyObject* module, PyObject *obj)
{
	JPContext* context = JPContext_global;
	if (!context->isRunning())
		Py_RETURN_NONE;
	PyObject *a1 = PyTuple_GetItem(obj, 0);
	if (!PyUnicode_Check(a1))
	{
		PyErr_SetString(PyExc_TypeError, "Bad callback argument");
		return nullptr;
	}
	if (PyUnicode_ReadChar(a1, 2) == 'a')
	{
		context->m_GC->onStart();
	} else
	{
		context->m_GC->onEnd();
	}
	Py_RETURN_NONE;
}

// GCOVR_EXCL_START

PyObject *PyJPModule_gcStats(PyObject* module, PyObject *obj)
{
	JPContext *context = PyJPModule_getContext();
	JPGCStats stats;
	context->m_GC->getStats(stats);
	PyObject *out = PyDict_New();
	PyObject *res;
	PyDict_SetItemString(out, "current", res = PyLong_FromSsize_t((Py_ssize_t)(stats.current_rss)));
	Py_DECREF(res);
	PyDict_SetItemString(out, "java", res = PyLong_FromSsize_t((Py_ssize_t)(stats.java_rss)));
	Py_DECREF(res);
	PyDict_SetItemString(out, "python", res = PyLong_FromSsize_t((Py_ssize_t)(stats.python_rss)));
	Py_DECREF(res);
	PyDict_SetItemString(out, "max", res = PyLong_FromSsize_t((Py_ssize_t)(stats.max_rss)));
	Py_DECREF(res);
	PyDict_SetItemString(out, "min", res = PyLong_FromSsize_t((Py_ssize_t)(stats.min_rss)));
	Py_DECREF(res);
	PyDict_SetItemString(out, "triggered", res = PyLong_FromSsize_t((Py_ssize_t)(stats.python_triggered)));
	Py_DECREF(res);
	return out;
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
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	return PyBool_FromLong(frame.isPackage(JPPyString::asStringUTF8(pkg)));
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}


#if 1
// GCOVR_EXCL_START
// This code was used in testing the Java slot memory layout.  It serves no purpose outside of debugging that issue.
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
	int offset = 0;
	if (!PyType_Check(other))
	{
		offset = PyJPValue_getJavaSlotOffset(other);
		printf("  Object:\n");
		printf("    size: %d\n", (int) Py_SIZE(other));
		printf("    dictoffset: %d\n", (int) ((long long) _PyObject_GetDictPtr(other)-(long long) other));
		printf("    javaoffset: %d\n", offset);
	}
	printf("  Type: %p\n", type);
	printf("    name: %s\n", type->tp_name);
	printf("    typename: %s\n", Py_TYPE(type)->tp_name);
	printf("    gc: %d\n", (type->tp_flags & Py_TPFLAGS_HAVE_GC) == Py_TPFLAGS_HAVE_GC);
	printf("    basicsize: %d\n", (int) type->tp_basicsize);
	printf("    itemsize: %d\n", (int) type->tp_itemsize);
	printf("    dictoffset: %d\n", (int) type->tp_dictoffset);
	printf("    weaklistoffset: %d\n", (int) type->tp_weaklistoffset);
	printf("    hasJavaSlot: %d\n", PyJPValue_hasJavaSlot(type));
	printf("    getattro: %p\n", type->tp_getattro);
	printf("    setattro: %p\n", type->tp_setattro);
	printf("    getattr: %p\n", type->tp_getattr);
	printf("    setattr: %p\n", type->tp_setattr);
	printf("    alloc: %p\n", type->tp_alloc);
	printf("    free: %p\n", type->tp_free);
	printf("    finalize: %p\n", type->tp_finalize);
	long v = _PyObject_VAR_SIZE(type, 1)+(PyJPValue_hasJavaSlot(type)?sizeof (JPValue):0);
	printf("    size?: %ld\n",v);
	printf("======\n");

	return PyBool_FromLong(ret);
	JP_PY_CATCH(nullptr);
}
// GCOVR_EXCL_STOP
#endif

// GCOVR_EXCL_START
int _PyJPModule_trace = 0;

static PyObject* PyJPModule_trace(PyObject *module, PyObject *args)
{
	bool old = _PyJPModule_trace;
	_PyJPModule_trace = PyLong_AsLong(args);
	return PyLong_FromLong(old);
}
// GCOVR_EXCL_STOP

#ifdef JP_INSTRUMENTATION
uint32_t _PyJPModule_fault_code = -1;

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
	return PyLong_FromLong(_PyJPModule_fault_code);
}
#endif

#ifdef ANDROID

static PyObject *PyJPModule_bootstrap(PyObject *module)
{
	// After all the internals are created we can connect the API with the internal module
	JNIEnv * env = Android_JNI_GetEnv();
	JPContext_global->attachJVM(env);
	PyJPModule_installGC(module);
	PyJPModule_loadResources(module);
	Py_RETURN_NONE;
}
#endif

static PyMethodDef moduleMethods[] = {
	// Startup and initialization
	{"isStarted", (PyCFunction) PyJPModule_isStarted, METH_NOARGS, ""},
#ifdef ANDROID
	{"bootstrap", (PyCFunction) PyJPModule_bootstrap, METH_NOARGS, ""},
#else
	{"startup", (PyCFunction) PyJPModule_startup, METH_VARARGS, ""},
	//	{"attach", (PyCFunction) (&PyJPModule_attach), METH_VARARGS, ""},
	{"shutdown", (PyCFunction) PyJPModule_shutdown, METH_VARARGS, ""},
#endif
	{"_getClass", (PyCFunction) PyJPModule_getClass, METH_O, ""},
	{"_hasClass", (PyCFunction) PyJPModule_hasClass, METH_O, ""},
	{"_newArrayType", (PyCFunction) PyJPModule_newArrayType, METH_VARARGS, ""},
	{"_collect", (PyCFunction) PyJPModule_collect, METH_VARARGS, ""},
	{"gcStats", (PyCFunction) PyJPModule_gcStats, METH_NOARGS, ""},

	// Threading
	{"isThreadAttachedToJVM", (PyCFunction) PyJPModule_isThreadAttached, METH_NOARGS, ""},
#ifndef ANDROID
	{"attachThreadToJVM", (PyCFunction) PyJPModule_attachThread, METH_NOARGS, ""},
	{"detachThreadFromJVM", (PyCFunction) PyJPModule_detachThread, METH_NOARGS, ""},
	{"attachThreadAsDaemon", (PyCFunction) PyJPModule_attachThreadAsDaemon, METH_NOARGS, ""},
#endif

	//{"dumpJVMStats", (PyCFunction) (&PyJPModule_dumpJVMStats), METH_NOARGS, ""},

	{"convertToDirectBuffer", (PyCFunction) PyJPModule_convertToDirectByteBuffer, METH_O, ""},
	{"arrayFromBuffer", (PyCFunction) PyJPModule_arrayFromBuffer, METH_VARARGS, ""},
	{"enableStacktraces", (PyCFunction) PyJPModule_enableStacktraces, METH_O, ""},
	{"isPackage", (PyCFunction) PyJPModule_isPackage, METH_O, ""},
	{"trace", (PyCFunction) PyJPModule_trace, METH_O, ""},
#ifdef JP_INSTRUMENTATION
	{"fault", (PyCFunction) PyJPModule_fault, METH_O, ""},
#endif
	{"examine", (PyCFunction) examine, METH_O, ""},

	// sentinel
	{nullptr}
};

static struct PyModuleDef moduledef = {
	PyModuleDef_HEAD_INIT,
	"_jpype",
	"jpype module",
	-1,
	moduleMethods,
};

PyObject *PyJPModule = nullptr;
JPContext* JPContext_global = nullptr;

PyMODINIT_FUNC PyInit__jpype()
{
	JP_PY_TRY("PyInit__jpype");
	JPContext_global = new JPContext();

	// Initialize the module (depends on python version)
	PyObject* module = PyModule_Create(&moduledef);
	// PyJPModule = module;
	Py_INCREF(module);
	PyJPModule = module;
#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(module, Py_MOD_GIL_NOT_USED);
#endif
	PyModule_AddStringConstant(module, "__version__", "1.5.2.dev0");

	// Our module will be used for PyFrame object and it is a requirement that
	// we have a builtins in our dictionary.
	PyObject *builtins = PyEval_GetBuiltins();
	Py_INCREF(builtins);
	PyModule_AddObject(module, "__builtins__", builtins);

	PyJPClassMagic = PyDict_New();
	// Initialize each of the python extension types
	PyJPValue_initType(module);
	PyJPClass_initType(module);
	PyJPObject_initType(module);

	PyJPArray_initType(module);
	PyJPBuffer_initType(module);
	PyJPField_initType(module);
	PyJPMethod_initType(module);
	PyJPNumber_initType(module);
	PyJPMonitor_initType(module);
	PyJPProxy_initType(module);
	PyJPClassHints_initType(module);
	PyJPPackage_initType(module);
	PyJPChar_initType(module);

	_PyJPModule_trace = true;

	return module;
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
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
	} catch (std::exception &ex)
	{
		PyErr_Format(PyExc_RuntimeError, "Unhandled C++ exception occurred: %s", ex.what());
		return;
	} catch (...)
	{
		PyErr_Format(PyExc_RuntimeError, "Unhandled C++ exception occurred");
		return;
	}
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

static PyObject *PyJPModule_convertBuffer(JPPyBuffer& buffer, PyObject *dtype)
{
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	Py_buffer& view = buffer.getView();

	// Okay two possibilities here.  We have a valid dtype specified,
	// or we need to figure it out from the buffer.
	JPClass *cls = nullptr;

	if (view.suboffsets != nullptr && view.suboffsets[view.ndim - 1] > 0)
	{
		PyErr_Format(PyExc_TypeError, "last dimension is not contiguous");
		return nullptr;
	}

	// First lets find out what we are unpacking
	Py_ssize_t itemsize = view.itemsize;
	char *format = view.format;
	if (format == nullptr)
		format = "B";
	// Standard size for 'l' is 4 in docs, but numpy uses format 'l' for long long
	if (itemsize == 8 && format[0] == 'l')
		format = "q";
	if (itemsize == 8 && format[0] == 'L')
		format = "Q";

	if (dtype != nullptr && dtype != Py_None )
	{
		cls = PyJPClass_getJPClass(dtype);
		if (cls == nullptr  || !cls->isPrimitive())
		{
			PyErr_Format(PyExc_TypeError, "'%s' is not a Java primitive type", Py_TYPE(dtype)->tp_name);
			return nullptr;
		}
	} else
	{
		switch (format[0])
		{
			case '?': cls = context->_boolean;
			case 'c': break;
			case 'b': cls = context->_byte;
			case 'B': break;
			case 'h': cls = context->_short;
				break;
			case 'H': break;
			case 'i':
			case 'l': cls = context->_int;
				break;
			case 'I':
			case 'L': break;
			case 'q': cls = context->_long;
				break;
			case 'Q': break;
			case 'f': cls = context->_float;
				break;
			case 'd': cls = context->_double;
				break;
			case 'n':
			case 'N':
			case 'P':
			default:
				break;
		}
		if (cls == nullptr)
		{
			PyErr_Format(PyExc_TypeError, "'%s' type code not supported without dtype specified", format);
			return nullptr;
		}
	}

	// Now we have a valid format code, so next lets get a converter for
	// the type.
	auto *pcls = dynamic_cast<JPPrimitiveType *>( cls);

	// Convert the shape
	Py_ssize_t subs = 1;
	Py_ssize_t base = 1;
	auto jdims = (jintArray) context->_int->newArrayOf(frame, view.ndim);
	if (view.shape != nullptr)
	{
		JPPrimitiveArrayAccessor<jintArray, jint*> accessor(frame, jdims,
				&JPJavaFrame::GetIntArrayElements, &JPJavaFrame::ReleaseIntArrayElements);
		jint *a = accessor.get();
		for (int i = 0; i < view.ndim; ++i)
		{
			a[i] = view.shape[i];
		}
		accessor.commit();
		for (int i = 0; i < view.ndim - 1; ++i)
		{
			subs *= view.shape[i];
		}
		base = view.shape[view.ndim - 1];
	} else
	{
		if (view.ndim > 1)
		{
			PyErr_Format(PyExc_TypeError, "buffer dims inconsistent");
			return nullptr;
		}
		base = view.len / view.itemsize;
	}
	return pcls->newMultiArray(frame, buffer, subs, base, (jobject) jdims);
}

#ifdef JP_INSTRUMENTATION

int PyJPModuleFault_check(uint32_t code)
{
	return (code == _PyJPModule_fault_code);
}

void PyJPModuleFault_throw(uint32_t code)
{
	if (code == _PyJPModule_fault_code)
	{
		_PyJPModule_fault_code = -1;
		JP_RAISE(PyExc_SystemError, "fault");
	}
}
#endif

void PyJPModule_installGC(PyObject* module)
{
	// Get the Python garbage collector
	JPPyObject gc = JPPyObject::call(PyImport_ImportModule("gc"));

	// Find the callbacks
	JPPyObject callbacks = JPPyObject::call(PyObject_GetAttrString(gc.get(), "callbacks"));

	// Hook up our callback
	JPPyObject collect = JPPyObject::call(PyObject_GetAttrString(module, "_collect"));
	PyList_Append(callbacks.get(), collect.get());
	JP_PY_CHECK();
}
