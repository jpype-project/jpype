/*****************************************************************************
   Copyright 2019 Karl Einar Nelson

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
#include <structmember.h>

#ifdef __cplusplus
extern "C"
{
#endif

PyObject *PyJPClassMeta_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClassMeta_new");
	// Call the Factory
	if (PyTuple_Size(args) == 1)
	{
		// We have to handle string, PyJPValue instances and point to a class
		PyObject *factory = PyObject_GetAttrString(PyJPModule_global, "ClassFactory");
		if (factory == NULL)
			return NULL;
		return PyObject_Call(factory, args, kwargs);
	}
	return PyType_Type.tp_new(type, args, kwargs);
	JP_PY_CATCH(NULL);
}

int PyJPClassMeta_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClassMeta_init", self);
	if (PyTuple_Size(args) == 1)
	{
		return 0;
	}
	return PyType_Type.tp_init(self, args, kwargs);
	JP_PY_CATCH(-1);
}

void PyJPClassMeta_dealloc(PyObject *self)
{
	JP_PY_TRY("PyJPClassMeta_dealloc");
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH();
}

PyObject *PyJPClassMeta_getattro(PyObject *obj, PyObject *name)
{
	JP_PY_TRY("PyJPClassMeta_getattro");
	PyJPModuleState *state = PyJPModuleState_global;
	if (!PyUnicode_Check(name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				Py_TYPE(name)->tp_name);
		return NULL;
	}
	Py_ssize_t sz;
	JP_TRACE(PyUnicode_AsUTF8AndSize(name, &sz));

	// Private members are accessed directly
	PyObject* pyattr = PyType_Type.tp_getattro(obj, name);
	if (pyattr == NULL)
		return NULL;
	JPPyObject attr(JPPyRef::_accept, pyattr);

	// Private members go regardless
	if (PyUnicode_GetLength(name) && PyUnicode_ReadChar(name, 0) == '_')
		return attr.keep();

	// Methods
	if (Py_TYPE(attr.get()) == (PyTypeObject*) state->PyJPMethod_Type)
		return attr.keep();

	// Don't allow properties to be rewritten
	if (!PyObject_IsInstance(attr.get(), (PyObject*) & PyProperty_Type))
		return attr.keep();

	const char *name_str = PyUnicode_AsUTF8(name);
	PyErr_Format(PyExc_AttributeError, "Field '%s' is static", name_str);
	return NULL;
	JP_PY_CATCH(NULL);
}

int PyJPClassMeta_setattro(PyObject *self, PyObject *attr_name, PyObject *v)
{
	JP_PY_TRY("PyJPClassMeta_setattro");
	if (!PyUnicode_Check(attr_name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				attr_name->ob_type->tp_name);
		return -1;
	}

	// Private members are accessed directly
	if (PyUnicode_GetLength(attr_name) && PyUnicode_ReadChar(attr_name, 0) == '_')
		return PyType_Type.tp_setattro(self, attr_name, v);

	JPPyObject f = JPPyObject(JPPyRef::_accept, PyType_Lookup((PyTypeObject*) self, attr_name));
	if (f.isNull())
	{
		const char *name_str = PyUnicode_AsUTF8(attr_name);
		PyErr_Format(PyExc_AttributeError, "Field '%s' is not found", name_str);
		return -1;
	}

	descrsetfunc desc = Py_TYPE(f.get())->tp_descr_set;
	if (desc != NULL)
		return desc(f.get(), self, v);

	// Not a descriptor
	const char *name_str = PyUnicode_AsUTF8(attr_name);
	PyErr_Format(PyExc_AttributeError,
			"Static field '%s' is not settable on Java '%s' object",
			name_str, Py_TYPE(self)->tp_name);
	return -1;
	JP_PY_CATCH(-1);
}

PyObject* PyJPClassMeta_check(PyObject *self, PyObject *args, PyObject *kwargs)
{
	PyJPModuleState *state = PyJPModuleState_global;

	int ret = 0;

	PyObject *other;
	if (!PyArg_ParseTuple(args, "O", &other))
		return NULL;

	PyObject *mro = Py_TYPE(other)->tp_mro;
	Py_ssize_t n = PyTuple_Size(mro);
	if (n >= 3)
	{
		ret = (PyTuple_GetItem(mro, n - 3) == state->PyJPClassMeta_Type);
	}

	return PyBool_FromLong(ret);
}

static struct PyMethodDef metaMethods[] = {
	{"_isinstance", (PyCFunction) & PyJPClassMeta_check, METH_VARARGS | METH_CLASS, ""},
	//	{"__instancecheck__", XXXX, METH_VARARGS, ""},
	//	{"__subclasscheck__", XXXX, METH_VARARGS, ""},
	{0}
};

static PyType_Slot metaSlots[] = {
	{Py_tp_new,      (void*) &PyJPClassMeta_new},
	{Py_tp_init,     (void*) &PyJPClassMeta_init},
	{Py_tp_dealloc,  (void*) &PyJPClassMeta_dealloc},
	{Py_tp_getattro, (void*) &PyJPClassMeta_getattro},
	{Py_tp_setattro, (void*) &PyJPClassMeta_setattro},
	{Py_tp_methods,  (void*) &metaMethods},
	{0}
};

PyType_Spec PyJPClassMetaSpec = {
	"_jpype.PyJPClassMeta",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	metaSlots
};

#ifdef __cplusplus
}
#endif