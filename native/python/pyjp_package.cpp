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
#include "jp_stringtype.h"
#include <structmember.h>

#ifdef __cplusplus
extern "C"
{
#endif
PyTypeObject *PyJPPackage_Type = nullptr;
static PyObject *PyJPPackage_Dict = nullptr;

static PyObject *PyJPPackage_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPPackage_new");
	PyObject *name = nullptr;
	if (!PyArg_Parse(args, "(U)", &name))
		return nullptr;

	// Check the cache
	PyObject *obj = PyDict_GetItem(PyJPPackage_Dict, name);
	if (obj != nullptr)
	{
		Py_INCREF(obj);
		return obj;
	}

	// Otherwise create a new object
	PyObject *self = PyModule_Type.tp_new(PyJPPackage_Type, args, nullptr);
	int rc = PyModule_Type.tp_init(self, args, nullptr);
	if (rc != 0)
	{
		// If we fail clean up the mess.
		Py_DECREF(self);
		return nullptr;
	}

	// Place in cache
	PyDict_SetItem(PyJPPackage_Dict, name, self);
	return self;
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static void dtor(PyObject *self)
{
	// No need for exception if JVM is not running.
	JPContext *context = JPContext_global;
	if (context == nullptr || !context->isRunning())
		return;
	auto jo = (jobject) PyCapsule_GetPointer(self, nullptr);
	if (jo == nullptr)
		return;
	JPJavaFrame frame = JPJavaFrame::outer();
	frame.DeleteGlobalRef(jo);
}

static jobject getPackage(JPJavaFrame &frame, PyObject *self)
{
	PyObject *dict = PyModule_GetDict(self); // borrowed
	PyObject *capsule = PyDict_GetItemString(dict, "_jpackage"); // borrowed
	jobject jo;
	if (capsule != nullptr)
	{
		jo = (jobject) PyCapsule_GetPointer(capsule, nullptr);
		return jo;
	}

	const char *name = PyModule_GetName(self);
	// Attempt to load the object.
	jo =	frame.getPackage(name);

	// Found it, use it.
	if (jo != nullptr)
	{
		jo = frame.NewGlobalRef(jo);
		capsule = PyCapsule_New(jo, nullptr, dtor);
		PyDict_SetItemString(dict, "_jpackage", capsule); // no steal
		//		Py_DECREF(capsule);
		return jo;
	}

	// Otherwise, this is a bad package.
	PyErr_Format(PyExc_AttributeError, "Java package '%s' is not valid", name);
	return nullptr;
}

/**
 * Get an attribute from the package.
 *
 * This will auto load packages and classes when encounter,
 * but first checks the cache.  This acts like an standard Python
 * module otherwise.
 *
 * @param self
 * @param attr
 * @return
 */
static PyObject *PyJPPackage_getattro(PyObject *self, PyObject *attr)
{
	JP_PY_TRY("PyJPPackage_getattro");
	if (!PyUnicode_Check(attr))
	{
		PyErr_Format(PyExc_TypeError, "attribute name must be string, not '%s'", Py_TYPE(attr)->tp_name);
		return nullptr;
	}

	PyObject *dict = PyModule_GetDict(self);
	if (dict != nullptr)
	{
		// Check the cache
		PyObject *out = PyDict_GetItem(PyModule_GetDict(self), attr);
		if (out != nullptr)
		{
			Py_INCREF(out);
			return out;
		}
	}

	string attrName = JPPyString::asStringUTF8(attr).c_str();
	// Check for private attribute
	if (attrName.compare(0, 2, "__") == 0)
		return PyObject_GenericGetAttr((PyObject*) self, attr);

	// Check for JVM running first
	JPContext* context = JPContext_global;
	if (!context->isRunning())
	{
		PyErr_Format(PyExc_RuntimeError,
				"Unable to import '%s.%U' without JVM",
				PyModule_GetName(self), attr);
		return nullptr;
	}
	JPJavaFrame frame = JPJavaFrame::outer();
	jobject pkg = getPackage(frame, self);
	if (pkg == nullptr)
		return nullptr;

	JPPyObject out;
	jobject obj;
	try
	{
		obj = frame.getPackageObject(pkg, attrName);
	}		catch (JPypeException& ex)
	{
		JPPyObject h = JPPyObject::accept(PyObject_GetAttrString(self, "_handler"));
		// If something fails, we need to go to a handler
		if (!h.isNull())
		{
			ex.toPython();
			JPPyErrFrame err;
			err.normalize();
			err.clear();
			JPPyObject tuple0 = JPPyTuple_Pack(self, attr, err.m_ExceptionValue.get());
			PyObject *rc = PyObject_Call(h.get(), tuple0.get(), nullptr);
			if (rc == nullptr)
				return nullptr;
			Py_DECREF(rc); // GCOVR_EXCL_LINE
		}
		throw; // GCOVR_EXCL_LINE
	}
	if (obj == nullptr)
	{
		PyErr_Format(PyExc_AttributeError, "Java package '%s' has no attribute '%U'",
				PyModule_GetName(self), attr);
		return nullptr;
	} else if (frame.IsInstanceOf(obj, context->_java_lang_Class->getJavaClass()))
		out = PyJPClass_create(frame, frame.findClass((jclass) obj));
	else if (frame.IsInstanceOf(obj, context->_java_lang_String->getJavaClass()))
	{
		JPPyObject u = JPPyObject::call(PyUnicode_FromFormat("%s.%U",
				PyModule_GetName(self), attr));
		JPPyObject args = JPPyTuple_Pack(u.get());
		out = JPPyObject::call(PyObject_Call((PyObject*) PyJPPackage_Type, args.get(), nullptr));
	} else
	{
		// We should be able to handle Python classes, datafiles, etc,
		// but that will take time to implement.  In principle, things
		// that are not packages or classes should appear as Buffers or
		// some other resource type.
		PyErr_Format(PyExc_AttributeError, "'%U' is unknown object type in Java package", attr);
		return nullptr;
	}
	// Cache the item for now
	PyDict_SetItem(dict, attr, out.get()); // no steal
	return out.keep();
	JP_PY_CATCH(nullptr);  // GCOVR_EXCL_LINE
}

/**
 *  This next method is required, I have no clue why.  Seems
 * likely that the default PyObject traverse does not agree
 * with modules.
 */
static int PyJPPackage_traverse(PyObject *m, visitproc visit, void *arg)
{
	return PyModule_Type.tp_traverse(m, visit, arg);
}

static int PyJPPackage_clear(PyObject *m)
{
	return PyModule_Type.tp_clear(m);
}

static PyObject *PyJPPackage_str(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPPackage_str");
	return PyModule_GetNameObject(self);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPPackage_repr(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPPackage_repr");
	return PyUnicode_FromFormat("<java package '%s'>", PyModule_GetName(self));
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPPackage_call(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPPackage_call");
	PyErr_Format(PyExc_TypeError, "Package `%s` is not callable.", PyModule_GetName(self));
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPPackage_package(PyObject *self)
{
	return PyUnicode_FromFormat("java");
}

static PyObject *PyJPPackage_path(PyObject *self)
{
	return PyList_New(0);
}

static PyObject *PyJPPackage_dir(PyObject *self)
{
	JP_PY_TRY("PyJPPackage_dir");
	JPJavaFrame frame = JPJavaFrame::outer();
	jobject pkg = getPackage(frame, self);
	if (pkg == nullptr)
		return nullptr;

	jarray o = frame.getPackageContents(pkg);
	Py_ssize_t len = frame.GetArrayLength(o);
	JPPyObject out = JPPyObject::call(PyList_New(len));
	for (Py_ssize_t i = 0;  i < len; ++i)
	{
		string str = frame.toStringUTF8((jstring)
				frame.GetObjectArrayElement((jobjectArray) o, (jsize) i));
		PyList_SetItem(out.get(), i, PyUnicode_FromFormat("%s", str.c_str()));
	}
	return out.keep();
	JP_PY_CATCH(nullptr);
}

/**
 * Add redirect for matmul in package modules.
 *
 * This will be used to support "java@obj" which will be used
 * to force cast a Python object into Java.
 *
 * @param self
 * @param other
 * @return
 */
static PyObject *PyJPPackage_cast(PyObject *self, PyObject *other)
{
	JP_PY_TRY("PyJPPackage_cast");
	PyObject *dict = PyModule_GetDict(self);
	PyObject* matmul = PyDict_GetItemString(dict, "__matmul__");
	if (matmul == nullptr)
		Py_RETURN_NOTIMPLEMENTED;
	JPPyObject args = JPPyTuple_Pack(self, other);
	return PyObject_Call(matmul, args.get(), nullptr);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPPackage_castEq(PyObject *self, PyObject *other)
{
	PyErr_Format(PyExc_TypeError, "Matmul equals not support for Java packages");
	return nullptr;
}

static PyMethodDef packageMethods[] = {
	{"__dir__", (PyCFunction) PyJPPackage_dir, METH_NOARGS},
	{nullptr},
};

static PyGetSetDef packageGetSets[] = {
	{"__all__", (getter) PyJPPackage_dir, nullptr, ""},
	{"__name__", (getter) PyJPPackage_str, nullptr, ""},
	{"__package__", (getter) PyJPPackage_package, nullptr, ""},
	{"__path__", (getter) PyJPPackage_path, nullptr, ""},
	{nullptr}
};

static PyType_Slot packageSlots[] = {
	{Py_tp_new,      (void*) PyJPPackage_new},
	{Py_tp_traverse, (void*) PyJPPackage_traverse},
	{Py_tp_clear,    (void*) PyJPPackage_clear},
	{Py_tp_getattro, (void*) PyJPPackage_getattro},
	{Py_tp_str,      (void*) PyJPPackage_str},
	{Py_tp_repr,     (void*) PyJPPackage_repr},
	{Py_tp_call,     (void*) PyJPPackage_call},
	{Py_nb_matrix_multiply, (void*) PyJPPackage_cast},
	{Py_nb_inplace_matrix_multiply, (void*) PyJPPackage_castEq},
	{Py_tp_methods,  (void*) packageMethods},
	{Py_tp_getset,   (void*) packageGetSets},
	{0}
};

static PyType_Spec packageSpec = {
	"_jpype._JPackage",
	-1,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	packageSlots
};

#ifdef __cplusplus
}
#endif

void PyJPPackage_initType(PyObject* module)
{
	// Inherit from module.
	JPPyObject bases = JPPyTuple_Pack(&PyModule_Type);
	packageSpec.basicsize = PyModule_Type.tp_basicsize;
	PyJPPackage_Type = (PyTypeObject*) PyType_FromSpecWithBases(&packageSpec, bases.get());
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JPackage", (PyObject*) PyJPPackage_Type);
	JP_PY_CHECK();

	// Set up a dictionary so we can reuse packages
	PyJPPackage_Dict = PyDict_New();
	PyModule_AddObject(module, "_packages", PyJPPackage_Dict);
}
