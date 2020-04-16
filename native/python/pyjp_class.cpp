/*****************************************************************************
   Copyright 2004-2008 Steve Ménard

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
#include <algorithm>
#include <Python.h>
#include <frameobject.h>
#include <structmember.h>
#include "jpype.h"
#include "pyjp.h"
#include "jp_arrayclass.h"
#include "jp_boxedtype.h"
#include "jp_field.h"
#include "jp_method.h"
#include "jp_methoddispatch.h"
#include "jp_primitive_accessor.h"

struct PyJPClass
{
	PyHeapTypeObject ht_type;
	JPClass* m_Class;
} ;

#ifdef __cplusplus
extern "C"
{
#endif

int PyJPClass_Check(PyObject* obj)
{
	return Py_IsInstanceSingle(PyJPClass_Type, obj);
}

PyObject *PyJPClass_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClass_new");
	if (PyTuple_Size(args) != 3)
		JP_RAISE(PyExc_TypeError, "Java class meta required 3 arguments");

	PyTypeObject *typenew = (PyTypeObject*) PyType_Type.tp_new(type, args, kwargs);
	if (typenew == 0)
		return NULL;
	if (typenew->tp_finalize != NULL && typenew->tp_finalize != (destructor) PyJPValue_finalize)
	{
		Py_DECREF(typenew);
		PyErr_SetString(PyExc_TypeError, "finalizer conflict");
		return NULL;
	}

	// GCOVR_EXCL_START
	// This sanity check is trigger if the user attempts to build their own
	// type wrapper with a __del__ method defined.  It is hard to trigger.
	if (typenew->tp_alloc != (allocfunc) PyJPValue_alloc
			&& typenew->tp_alloc != PyBaseObject_Type.tp_alloc)
	{
		Py_DECREF(typenew);
		PyErr_SetString(PyExc_TypeError, "alloc conflict");
		return NULL;
	}
	// GCOVR_EXCL_STOP

	typenew->tp_alloc = (allocfunc) PyJPValue_alloc;
	typenew->tp_finalize = (destructor) PyJPValue_finalize;

	return (PyObject*) typenew;
	JP_PY_CATCH(NULL);
}

PyObject* examine(PyObject *module, PyObject *other);

PyObject* PyJPClass_FromSpecWithBases(PyType_Spec *spec, PyObject *bases)
{
	JP_PY_TRY("PyJPClass_FromSpecWithBases");
	// Python lacks a FromSpecWithMeta so we are going to have to fake it here.
	PyTypeObject* type = (PyTypeObject*) PyJPClass_Type->tp_alloc(PyJPClass_Type, 0);
	PyHeapTypeObject* heap = (PyHeapTypeObject*) type;
	type->tp_flags = spec->flags | Py_TPFLAGS_HEAPTYPE | Py_TPFLAGS_HAVE_GC;
	type->tp_name = spec->name;
	const char *s = strrchr(spec->name, '.');
	if (s == NULL)
		s = spec->name;
	else
		s++;
	heap->ht_qualname = PyUnicode_FromString(s);
	heap->ht_name = heap->ht_qualname;
	Py_INCREF(heap->ht_name);
	if (bases == NULL)
		type->tp_bases = PyTuple_Pack(1, (PyObject*) & PyBaseObject_Type);
	else
	{
		type->tp_bases = bases;
		Py_INCREF(bases);
	}
	type->tp_base = (PyTypeObject*) PyTuple_GetItem(type->tp_bases, 0);
	Py_INCREF(type->tp_base);
	type->tp_as_async = &heap->as_async;
	type->tp_as_buffer = &heap->as_buffer;
	type->tp_as_mapping = &heap->as_mapping;
	type->tp_as_number = &heap->as_number;
	type->tp_as_sequence = &heap->as_sequence;
	type->tp_basicsize = spec->basicsize;
	if (spec->basicsize == 0)
		type->tp_basicsize = type->tp_base->tp_basicsize;
	type->tp_itemsize = spec->itemsize;
	if (spec->itemsize == 0)
		type->tp_itemsize = type->tp_base->tp_itemsize;
	type->tp_alloc = PyJPValue_alloc;
	type->tp_free = PyJPValue_free;
	type->tp_finalize = (destructor) PyJPValue_finalize;
	for (PyType_Slot* slot = spec->slots; slot->slot; slot++)
	{
		switch (slot->slot)
		{
			case Py_tp_free:
				type->tp_free = (freefunc) slot->pfunc;
				break;
			case Py_tp_new:
				type->tp_new = (newfunc) slot->pfunc;
				break;
			case Py_tp_init:
				type->tp_init = (initproc) slot->pfunc;
				break;
			case Py_tp_getattro:
				type->tp_getattro = (getattrofunc) slot->pfunc;
				break;
			case Py_tp_setattro:
				type->tp_setattro = (setattrofunc) slot->pfunc;
				break;
			case Py_tp_dealloc:
				type->tp_dealloc = (destructor) slot->pfunc;
				break;
			case Py_tp_str:
				type->tp_str = (reprfunc) slot->pfunc;
				break;
			case Py_tp_repr:
				type->tp_repr = (reprfunc) slot->pfunc;
				break;
			case Py_tp_methods:
				type->tp_methods = (PyMethodDef*) slot->pfunc;
				break;
			case Py_sq_item:
				heap->as_sequence.sq_item = (ssizeargfunc) slot->pfunc;
				break;
			case Py_sq_length:
				heap->as_sequence.sq_length = (lenfunc) slot->pfunc;
				break;
			case Py_mp_ass_subscript:
				heap->as_mapping.mp_ass_subscript = (objobjargproc) slot->pfunc;
				break;
			case Py_tp_hash:
				type->tp_hash = (hashfunc) slot->pfunc;
				break;
			case Py_nb_int:
				heap->as_number.nb_int = (unaryfunc) slot->pfunc;
				break;
			case Py_nb_float:
				heap->as_number.nb_float = (unaryfunc) slot->pfunc;
				break;
			case Py_tp_richcompare:
				type->tp_richcompare = (richcmpfunc) slot->pfunc;
				break;
			case Py_mp_subscript:
				heap->as_mapping.mp_subscript = (binaryfunc) slot->pfunc;
				break;
				// GCOVR_EXCL_START
			default:
				PyErr_Format(PyExc_TypeError, "slot %d not implemented", slot->slot);
				JP_RAISE_PYTHON();
				// GCOVR_EXCL_STOP
		}
	}
	PyType_Ready(type);
	//heap->ht_cached_keys = _PyDict_NewKeysForClass();
	PyDict_SetItemString(type->tp_dict, "__module__", PyUnicode_FromString("_jpype"));
	return (PyObject*) type;
	JP_PY_CATCH(NULL); // GCOVR_EXCL_LINE
}

int PyJPClass_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPClass_init");
	if (PyTuple_Size(args) == 1)
		return 0;

	// Set the host object
	PyObject *name = NULL;
	PyObject *bases = NULL;
	PyObject *members = NULL;
	if (!PyArg_ParseTuple(args, "OOO", &name, &bases, &members))
		return -1;

	//	 Check that all types are Java types
	if (!PyTuple_Check(bases))
		JP_RAISE(PyExc_TypeError, "Bases must be a tuple");
	for (int i = 0; i < PyTuple_Size(bases); ++i)
	{
		if (!PyJPClass_Check(PyTuple_GetItem(bases, i)))
			JP_RAISE(PyExc_TypeError, "All bases must be Java types");
	}

	// Call the type init
	int rc = PyType_Type.tp_init(self, args, kwargs);
	if (rc == -1)
		return rc; // GCOVR_EXCL_LINE no clue how to trigger this one

	return rc;
	JP_PY_CATCH(-1);
}

PyObject* PyJPClass_mro(PyTypeObject *self)
{
	Py_ssize_t sz = PySequence_Size(self->tp_bases);
	std::list<PyObject*> bases;
	bases.push_back((PyObject*) self);

	// Merge together all bases
	std::list<PyObject*> out;
	for (std::list<PyObject*>::iterator iter = bases.begin();
			iter != bases.end(); ++iter)
	{
		PyObject *l = ((PyTypeObject*) * iter)->tp_bases;
		sz = PySequence_Size(l);
		for (Py_ssize_t i = 0;  i < sz; ++i)
		{
			PyObject *obj = PyTuple_GetItem(l, i);
			bool found = (std::find(bases.begin(), bases.end(), obj) != bases.end());
			if (!found)
			{
				bases.push_back(obj);
			}
		}
	}

	while (!bases.empty())
	{
		PyObject* front = bases.front();
		bases.pop_front();
		for (std::list<PyObject*>::iterator iter = bases.begin();
				iter != bases.end(); ++iter)
		{
			if (PySequence_Contains(((PyTypeObject*) * iter)->tp_bases, front))
			{
				bases.push_back(front);
				front = NULL;
				break;
			}
		}
		if (front != NULL)
		{
			out.push_back(front);
			PyObject* next = (PyObject*) ((PyTypeObject*) front)->tp_base;
			if (next)
			{
				bases.remove(next);
				bases.push_front(next);
			}
		}
	}

	PyObject *obj = PyTuple_New(out.size());
	int j = 0;
	for (std::list<PyObject*>::iterator iter = out.begin();
			iter != out.end(); ++iter)
	{
		Py_INCREF(*iter);
		PyTuple_SetItem(obj, j++, *iter);
	}
	return obj;
}

PyObject *PyJPClass_getattro(PyObject *obj, PyObject *name)
{
	JP_PY_TRY("PyJPClass_getattro");
	if (!PyUnicode_Check(name))
	{
		PyErr_Format(PyExc_TypeError,
				"attribute name must be string, not '%.200s'",
				Py_TYPE(name)->tp_name);
		return NULL;
	}

	// Private members are accessed directly
	PyObject* pyattr = PyType_Type.tp_getattro(obj, name);
	if (pyattr == NULL)
		return NULL;
	JPPyObject attr(JPPyRef::_claim, pyattr);

	// Private members go regardless
	if (PyUnicode_GetLength(name) && PyUnicode_ReadChar(name, 0) == '_')
		return attr.keep();

	// Methods
	if (Py_TYPE(attr.get()) == PyJPMethod_Type)
		return attr.keep();

	// Don't allow properties to be rewritten
	if (!PyObject_IsInstance(attr.get(), (PyObject*) & PyProperty_Type))
		return attr.keep();

	const char *name_str = PyUnicode_AsUTF8(name);
	PyErr_Format(PyExc_AttributeError, "Field '%s' is static", name_str);
	return NULL;
	JP_PY_CATCH(NULL);
}

int PyJPClass_setattro(PyObject *self, PyObject *attr_name, PyObject *v)
{
	JP_PY_TRY("PyJPClass_setattro");
	PyJPModule_getContext();
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

	JPPyObject f = JPPyObject(JPPyRef::_accept, Py_GetAttrDescriptor((PyTypeObject*) self, attr_name));
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
			name_str, ((PyTypeObject*) self)->tp_name);
	return -1;
	JP_PY_CATCH(-1);
}

PyObject* PyJPClass_subclasscheck(PyTypeObject *type, PyTypeObject *test)
{
	JP_PY_TRY("PyJPClass_subclasscheck");
	if (test == type)
		Py_RETURN_TRUE;

	// GCOVR_EXCL_START
	// This is triggered only if the user asks for isInstance when the
	// JVM is shutdown. It should not happen in normal operations.
	if (!JPContext_global->isRunning())
	{
		if ((PyObject*) type == _JObject)
			return PyBool_FromLong(Py_IsSubClassSingle(PyJPObject_Type, test));
		return PyBool_FromLong(Py_IsSubClassSingle(type, test));
	}
	// GCOVR_EXCL_STOP

	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);

	// Check for class inheritance first
	JPClass *testClass = PyJPClass_getJPClass((PyObject*) test);
	JPClass *typeClass = PyJPClass_getJPClass((PyObject*) type);
	if (testClass == NULL)
		Py_RETURN_FALSE;
	if (testClass == typeClass)
		Py_RETURN_TRUE;
	if (typeClass != NULL)
	{
		if (typeClass->isPrimitive())
			Py_RETURN_FALSE;
		bool b = frame.IsAssignableFrom(testClass->getJavaClass(), typeClass->getJavaClass()) != 0;
		return PyBool_FromLong(b);
	}

	// Otherwise check for special cases
	if ((PyObject*) type == _JInterface)
		return PyBool_FromLong(testClass->isInterface());
	if ((PyObject*) type == _JObject)
		return PyBool_FromLong(!testClass->isPrimitive());
	if ((PyObject*) type == _JArray)
		return PyBool_FromLong(testClass->isArray());
	if ((PyObject*) type == _JException)
		return PyBool_FromLong(testClass->isThrowable());

	PyObject* mro1 = test->tp_mro;
	Py_ssize_t n1 = PyTuple_Size(mro1);
	for (int i = 0; i < n1; ++i)
	{
		if (PyTuple_GetItem(mro1, i) == (PyObject*) type)
			Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPClass_class(PyObject *self, PyObject *closure)
{
	JP_PY_TRY("PyJPClass_class");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	JPValue* javaSlot = PyJPValue_getJavaSlot(self);
	if (javaSlot == NULL)
		JP_RAISE(PyExc_AttributeError, "Java slot is null");
	return javaSlot->getClass()->convertToPythonObject(frame, javaSlot->getValue(), false).keep();
	JP_PY_CATCH(NULL);
}

static int PyJPClass_setClass(PyObject *self, PyObject *type, PyObject *closure)
{
	JP_PY_TRY("PyJPClass_setClass", self);
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	JPValue* javaSlot = PyJPValue_getJavaSlot(type);
	if (javaSlot == NULL || javaSlot->getClass() != context->_java_lang_Class)
		JP_RAISE(PyExc_TypeError, "Java class instance is required");
	if (PyJPValue_isSetJavaSlot(self))
		JP_RAISE(PyExc_AttributeError, "Java class can't be set");
	PyJPValue_assignJavaSlot(frame, self, *javaSlot);

	JPClass* cls = frame.findClass((jclass) javaSlot->getJavaObject());
	JP_TRACE("Set host", cls, javaSlot->getClass()->getCanonicalName().c_str());
	if (cls->getHost() == NULL)
		cls->setHost(self);
	((PyJPClass*) self)->m_Class = cls;
	return 0;
	JP_PY_CATCH(-1);
}

static PyObject *PyJPClass_hints(PyObject *self, PyObject *closure)
{
	JP_PY_TRY("PyJPClass_hints");
	PyJPModule_getContext();
	PyJPClass *cls = (PyJPClass*) self;
	PyObject *hints = cls->m_Class->getHints();
	if (hints == NULL)
		Py_RETURN_NONE; // GCOVR_EXCL_LINE only triggered if JClassPost failed
	Py_INCREF(hints);
	return hints;
	JP_PY_CATCH(NULL);
}

static int PyJPClass_setHints(PyObject *self, PyObject *value, PyObject *closure)
{
	JP_PY_TRY("PyJPClass_setHints", self);
	PyJPModule_getContext();
	PyJPClass *cls = (PyJPClass*) self;
	PyObject *hints = cls->m_Class->getHints();
	if (hints != NULL)
		JP_RAISE(PyExc_AttributeError, "_hints can't be set");
	cls->m_Class->setHints(value);
	return 0;
	JP_PY_CATCH(-1);
}

PyObject* PyJPClass_instancecheck(PyTypeObject *self, PyObject *test)
{
	// JInterface is a meta
	if ((PyObject*) self == _JInterface)
	{
		JPContext *context = PyJPModule_getContext();
		JPJavaFrame frame(context);
		JPClass *testClass = PyJPClass_getJPClass((PyObject*) test);
		return PyBool_FromLong(testClass != NULL && testClass->isInterface());
	}
	if ((PyObject*) self == _JException)
	{
		JPContext *context = PyJPModule_getContext();
		JPJavaFrame frame(context);
		JPClass *testClass = PyJPClass_getJPClass((PyObject*) test);
		if (testClass)
			return PyBool_FromLong(testClass->isThrowable());
	}
	return PyJPClass_subclasscheck(self, Py_TYPE(test));
}

// Added for auditing

static PyObject *PyJPClass_canConvertToJava(PyJPClass *self, PyObject *other)
{
	JP_PY_TRY("PyJPClass_canConvertToJava");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);

	JPClass *cls = self->m_Class;

	// Test the conversion
	JPMatch match(&frame, other);
	cls->findJavaConversion(match);

	// Report to user
	if (match.type == JPMatch::_none)
		return JPPyString::fromStringUTF8("none").keep();
	if (match.type == JPMatch::_explicit)
		return JPPyString::fromStringUTF8("explicit").keep();
	if (match.type == JPMatch::_implicit)
		return JPPyString::fromStringUTF8("implicit").keep();
	if (match.type == JPMatch::_exact)
		return JPPyString::fromStringUTF8("exact").keep();

	// Not sure how this could happen
	Py_RETURN_NONE; // GCOVR_EXCL_NONE
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPClass_cast(PyJPClass *self, PyObject *other)
{
	JP_PY_TRY("PyJPClass_cast");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	JPClass *type =  self->m_Class;
	JPValue *val = PyJPValue_getJavaSlot(other);

	// Cast on non-Java
	if ( val == NULL || val->getClass()->isPrimitive())
	{
		JPMatch match(&frame, other);
		type->findJavaConversion(match);
		// Otherwise, see if we can convert it
		if (match.type == JPMatch::_none)
		{
			PyErr_Format(PyExc_TypeError,
					"Unable to cast '%s' to java type '%s'",
					Py_TYPE(other)->tp_name,
					type->getCanonicalName().c_str()
					);
			return NULL;
		}
		jvalue v = match.convert();
		return type->convertToPythonObject(frame, v, true).keep();
	}

	// Cast on java object
	//	if (!type->isSubTypeOf(val->getClass()))
	jobject obj = val->getJavaObject();
	if (obj == NULL)
	{
		jvalue v;
		v.l = NULL;
		return type->convertToPythonObject(frame, v, true).keep();
	}
	JPClass *otherClass = frame.findClassForObject(obj);
	if (otherClass == NULL)
	{
		return type->convertToPythonObject(frame, val->getValue(), true).keep();
	}

	if (!otherClass->isAssignableFrom(frame, type))
	{
		PyErr_Format(PyExc_TypeError,
				"Unable to cast '%s' to java type '%s'",
				otherClass->getCanonicalName().c_str(),
				type->getCanonicalName().c_str()
				);
		return NULL;
	}

	// Special case.  If the otherClass is an array and the array is
	// a slice then we need to copy it here.
	if (PyObject_IsInstance(other, (PyObject*) PyJPArray_Type))
	{
		PyJPArray *array = (PyJPArray*) other;
		if (array->m_Array->isSlice())
		{
			JPJavaFrame frame(context);
			jvalue v;
			v.l = frame.keep(array->m_Array->clone(frame, other));
			return type->convertToPythonObject(frame, v, true).keep();
		}
	}

	return type->convertToPythonObject(frame, val->getValue(), true).keep();

	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

// Added for auditing

static PyObject *PyJPClass_convertToJava(PyJPClass *self, PyObject *other)
{
	JP_PY_TRY("PyJPClass_convertToJava");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);

	JPClass *cls = self->m_Class;

	// Test the conversion
	JPMatch match(&frame, other);
	cls->findJavaConversion(match);

	// If there is no conversion report a failure
	if (match.type == JPMatch::_none)
	{
		PyErr_SetString(PyExc_TypeError, "Unable to create an instance.");
		return 0;
	}

	// Otherwise give back a PyJPValue
	jvalue v = match.convert();
	return cls->convertToPythonObject(frame, v, true).keep();
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPClass_repr(PyJPClass *self)
{
	std::stringstream ss;
	string name;
	if (self->m_Class == NULL)
		name = ((PyTypeObject*) self)->tp_name;
	else
		name = self->m_Class->getCanonicalName();
	ss << "<java class '" << name << "'>";
	return PyUnicode_FromString(ss.str().c_str());
}

static PyMethodDef classMethods[] = {
	{"__instancecheck__", (PyCFunction) & PyJPClass_instancecheck, METH_O, ""},
	{"__subclasscheck__", (PyCFunction) & PyJPClass_subclasscheck, METH_O, ""},
	{"mro",               (PyCFunction) & PyJPClass_mro, METH_NOARGS, ""},
	{"_canConvertToJava", (PyCFunction) (&PyJPClass_canConvertToJava), METH_O, ""},
	{"_convertToJava",    (PyCFunction) (&PyJPClass_convertToJava), METH_O, ""},
	{"_cast",             (PyCFunction) (&PyJPClass_cast), METH_O, ""},
	{NULL},
};

static PyGetSetDef classGetSets[] = {
	{"class_", (getter) PyJPClass_class, (setter) PyJPClass_setClass, ""},
	{"_hints", (getter) PyJPClass_hints, (setter) PyJPClass_setHints, ""},
	{0}
};

static PyType_Slot classSlots[] = {
	{ Py_tp_alloc,    (void*) PyJPValue_alloc},
	{ Py_tp_finalize, (void*) PyJPValue_finalize},
	{ Py_tp_new,      (void*) PyJPClass_new},
	{ Py_tp_init,     (void*) PyJPClass_init},
	{ Py_tp_repr,     (void*) PyJPClass_repr},
	{ Py_tp_getattro, (void*) PyJPClass_getattro},
	{ Py_tp_setattro, (void*) PyJPClass_setattro},
	{ Py_tp_methods,  (void*) classMethods},
	{ Py_tp_getset,   (void*) &classGetSets},
	{0}
};

PyTypeObject* PyJPClass_Type = NULL;
static PyType_Spec classSpec = {
	"_jpype._JClass",
	sizeof (PyJPClass),
	0,
	Py_TPFLAGS_DEFAULT  | Py_TPFLAGS_BASETYPE,
	classSlots
};

#ifdef __cplusplus
}
#endif

void PyJPClass_initType(PyObject* module)
{
	PyObject *bases = PyTuple_Pack(1, &PyType_Type);
	PyJPClass_Type = (PyTypeObject*) PyType_FromSpecWithBases(&classSpec, bases);
	Py_DECREF(bases);
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JClass", (PyObject*) PyJPClass_Type);
	JP_PY_CHECK();
}

JPClass* PyJPClass_getJPClass(PyObject* obj)
{
	try
	{
		if (obj == NULL)
			return NULL;
		if (PyJPClass_Check(obj))
			return ((PyJPClass*) obj)->m_Class;
		JPValue* javaSlot = PyJPValue_getJavaSlot(obj);
		if (javaSlot == NULL)
			return NULL;
		JPClass *cls = javaSlot->getClass();
		if (cls != cls->getContext()->_java_lang_Class)
			return NULL;
		JPJavaFrame frame(cls->getContext());
		return frame.findClass((jclass) javaSlot->getJavaObject());
	}	catch (...) // GCOVR_EXCL_LINE
	{
		return NULL; // GCOVR_EXCL_LINE
	}
}

static JPPyObject PyJPClass_getBases(JPJavaFrame &frame, JPClass* cls)
{
	JP_TRACE_IN("PyJPClass_bases");

	cls->ensureMembers(frame);

	// Decide the base for this object
	JPPyObject baseType;
	JPContext *context = PyJPModule_getContext();
	JPClass *super = cls->getSuperClass();
	if (dynamic_cast<JPBoxedType*> (cls) == cls)
	{
		if (cls == context->_java_lang_Boolean)
		{
			baseType = JPPyObject(JPPyRef::_use, (PyObject*) PyJPNumberBool_Type);
		} else if (cls == context->_java_lang_Character)
		{
			baseType = JPPyObject(JPPyRef::_use, (PyObject*) PyJPNumberChar_Type);
		} else if (cls == context->_java_lang_Boolean
				|| cls == context->_java_lang_Byte
				|| cls == context->_java_lang_Short
				|| cls == context->_java_lang_Integer
				|| cls == context->_java_lang_Long
				)
		{
			baseType = JPPyObject(JPPyRef::_use, (PyObject*) PyJPNumberLong_Type);
		} else if ( cls == context->_java_lang_Float
				|| cls == context->_java_lang_Double
				)
		{
			baseType = JPPyObject(JPPyRef::_use, (PyObject*) PyJPNumberFloat_Type);
		}
	} else if (JPModifier::isBuffer(cls->getModifiers()))
	{
		baseType = JPPyObject(JPPyRef::_use, (PyObject*) PyJPBuffer_Type);
	} else if (cls == context->_java_lang_Throwable)
	{
		baseType = JPPyObject(JPPyRef::_use, (PyObject*) PyJPException_Type);
	} else if (cls->isArray())
	{
		JPArrayClass* acls = (JPArrayClass*) cls;
		if (acls->getComponentType()->isPrimitive())
			baseType = JPPyObject(JPPyRef::_use, (PyObject*) PyJPArrayPrimitive_Type);
		else
			baseType = JPPyObject(JPPyRef::_use, (PyObject*) PyJPArray_Type);
	} else if (cls->getCanonicalName() == "java.lang.Comparable")
	{
		baseType = JPPyObject(JPPyRef::_use, (PyObject*) PyJPComparable_Type);
	} else if (super == NULL)
	{
		baseType = JPPyObject(JPPyRef::_use, (PyObject*) PyJPObject_Type);
	}

	const JPClassList& baseItf = cls->getInterfaces();
	size_t count = baseItf.size() + (!baseType.isNull() ? 1 : 0) + (super != NULL ? 1 : 0);

	// Pack into a tuple
	JPPyObject result = JPPyObject(JPPyRef::_call, PyList_New(count));
	unsigned int i = 0;
	for (; i < baseItf.size(); i++)
	{
		PyList_SetItem(result.get(), i, PyJPClass_create(frame, baseItf[i]).keep());
	}
	if (super != NULL)
	{
		PyList_SetItem(result.get(), i++, PyJPClass_create(frame, super).keep());
	}
	if (!baseType.isNull())
	{
		PyList_SetItem(result.get(), i++, baseType.keep());
	}
	return result;
	JP_TRACE_OUT;
}

/**
 * Internal method for wrapping a returned Java class instance.
 *
 * This checks the cache for existing wrappers and then
 * transfers control to JClassFactory.  This is required because all of
 * the post load stuff needs to be in Python.
 *
 * @param cls
 * @return
 */
JPPyObject PyJPClass_create(JPJavaFrame &frame, JPClass* cls)
{
	JP_TRACE_IN("PyJPClass_create", cls);
	// Check the cache for speed

	PyObject *host = (PyObject*) cls->getHost();
	if (host != NULL)
	{
		return JPPyObject(JPPyRef::_use, host);
	}
	JPContext *context = cls->getContext();
	//	cls->postLoad();

	JPPyTuple args = JPPyTuple::newTuple(3);
	PyTuple_SetItem(args.get(), 0, JPPyString::fromStringUTF8(cls->getCanonicalName()).keep());
	PyTuple_SetItem(args.get(), 1, PyJPClass_getBases(frame, cls).keep());

	PyObject *members = PyDict_New();
	PyTuple_SetItem(args.get(), 2, members);

	// Catch creation loop,  the process of creating our parent
	host = (PyObject*) cls->getHost();
	if (host != NULL)
	{
		return JPPyObject(JPPyRef::_use, host);
	}


	const JPFieldList& instFields = cls->getFields();
	for (JPFieldList::const_iterator iter = instFields.begin(); iter != instFields.end(); iter++)
	{
		JPPyObject fieldName(JPPyString::fromStringUTF8((*iter)->getName()));
		PyDict_SetItem(members, fieldName.keep(), PyJPField_create(*iter).keep());
	}
	const JPMethodDispatchList& m_Methods = cls->getMethods();
	for (JPMethodDispatchList::const_iterator iter = m_Methods.begin(); iter != m_Methods.end(); iter++)
	{
		JPPyObject methodName(JPPyString::fromStringUTF8((*iter)->getName()));
		PyDict_SetItem(members, methodName.keep(),
				PyJPMethod_create(*iter, NULL).keep());
	}

	if (cls->isInterface())
	{
		const JPMethodDispatchList& m_Methods = context->_java_lang_Object->getMethods();
		for (JPMethodDispatchList::const_iterator iter = m_Methods.begin(); iter != m_Methods.end(); iter++)
		{
			JPPyObject methodName(JPPyString::fromStringUTF8((*iter)->getName()));
			PyDict_SetItem(members, methodName.keep(),
					PyJPMethod_create(*iter, NULL).keep());
		}
	}

	// Call the customizer to make any required changes to the tables.
	JP_TRACE("call pre");
	JPPyObject rc(JPPyRef::_call, PyObject_Call(_JClassPre, args.get(), NULL));

	// Create the type using the meta class magic
	JPPyObject vself(JPPyRef::_call, PyJPClass_Type->tp_new(PyJPClass_Type, rc.get(), NULL));
	PyJPClass *self = (PyJPClass*) vself.get();

	// Attach the javaSlot
	self->m_Class = cls;
	//	self->m_Class->postLoad();
	PyJPValue_assignJavaSlot(frame, (PyObject*) self, JPValue(context->_java_lang_Class,
			(jobject) self->m_Class->getJavaClass()));

	// Attach the cache  (adds reference, thus wrapper lives to end of JVM)
	JP_TRACE("set host");
	cls->setHost((PyObject*) self);

	// Call the post load routine to attach inner classes
	JP_TRACE("call post");
	args = JPPyTuple::newTuple(1);
	args.setItem(0, (PyObject*) self);
	JPPyObject rc2(JPPyRef::_call, PyObject_Call(_JClassPost, args.get(), NULL));
	return JPPyObject(JPPyRef::_use, (PyObject*) self);
	JP_TRACE_OUT;
}
