/*
 * Copyright 2020 nelson85.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "jpype.h"
#include "pyjp.h"
#include <structmember.h>
#include "jp_boxedtype.h"

#ifdef __cplusplus
extern "C"
{
#endif

PyTypeObject *PyJPChar2_Type = NULL;

struct PyJPChar2
{
	PyCompactUnicodeObject m_Obj;
	char m_Data[4];
} ;

#define _PyUnicode_LENGTH(op) (((PyASCIIObject *)(op))->length)
#define _PyUnicode_STATE(op) (((PyASCIIObject *)(op))->state)
#define _PyUnicode_HASH(op) (((PyASCIIObject *)(op))->hash)
#define _PyUnicode_WSTR(op) (((PyASCIIObject*)(op))->wstr)
#define _PyUnicode_WSTR_LENGTH(op)  (((PyCompactUnicodeObject*)(op))->wstr_length)

static Py_UCS4 ord(PyObject *c)
{
	if (PyUnicode_Check(c))
	{
		if (PyUnicode_READY(c) == -1)
			return -1;
		if (PyUnicode_GET_LENGTH(c) == 1)
			return PyUnicode_READ_CHAR(c, 0);
	} else if (PyBytes_Check(c) && PyBytes_GET_SIZE(c) == 1)
		return (Py_UCS4) ((unsigned char) *PyBytes_AS_STRING(c));
	else if (PyByteArray_Check(c) &&  PyByteArray_GET_SIZE(c))
		return (Py_UCS4) ((unsigned char) *PyByteArray_AS_STRING(c));
	return (Py_UCS4) - 1;
}

PyObject *PyJPChar2_Create(PyTypeObject *type, Py_UCS2 p)
{
	PyJPChar2  *self = (PyJPChar2*) PyJPValue_alloc(type, 0);
	if (self == 0)
		return 0;
	self->m_Data[0] = 0;
	self->m_Data[1] = 0;
	self->m_Data[2] = 0;
	self->m_Data[3] = 0;

	_PyUnicode_LENGTH(self) = 1;
	_PyUnicode_HASH(self) = -1;
	_PyUnicode_STATE(self).kind = PyUnicode_1BYTE_KIND;

	_PyUnicode_STATE(self).ascii = 0;
	_PyUnicode_STATE(self).ready = 1;
	_PyUnicode_STATE(self).interned = 0;
	_PyUnicode_STATE(self).compact = 1;

	if (p < 128)
	{
		_PyUnicode_STATE(self).ascii = 1;
		char *data = (char*) (((PyASCIIObject*) self) + 1);
		data[0] = p;
		data[1] = 0;
	} else
		if (p < 256)
	{
		char *data = (char*) ( ((PyCompactUnicodeObject*) self) + 1);
		data[0] = p;
		data[1] = 0;
		_PyUnicode_WSTR_LENGTH(self) = 0;
		_PyUnicode_WSTR(self) = NULL;
		self->m_Obj.utf8 = NULL;
		self->m_Obj.utf8_length = 0;
	} else
	{

		Py_UCS2 *data = (Py_UCS2*) ( ((PyCompactUnicodeObject*) self) + 1);
		data[0] = p;
		data[1] = 0;
		_PyUnicode_STATE(self).kind = PyUnicode_2BYTE_KIND;
		if (sizeof (wchar_t) == 2)
		{
			_PyUnicode_WSTR_LENGTH(self) = 1;
			_PyUnicode_WSTR(self) = (wchar_t *) data;
		} else
		{
			_PyUnicode_WSTR_LENGTH(self) = 0;
			_PyUnicode_WSTR(self) = NULL;
		}
		self->m_Obj.utf8 = NULL;
		self->m_Obj.utf8_length = 0;
	}
	return (PyObject*) self;
}

/** This one is just used for initializing so the local copy matches.
 */
Py_UCS2 fromJPValue(const JPValue & value)
{
	JPClass* cls = value.getClass();
	if (cls->isPrimitive())
		return (Py_UCS2) (value.getValue().c);
	JPPrimitiveType* pcls = ((JPBoxedType*) cls)->getPrimitive();
	if (value.getValue().l == 0)
		return (Py_UCS2) - 1;
	else
		return (Py_UCS2) (pcls->getValueFromObject(value).getValue().c);
}

/** Get the value of the char.  Does not touch Java.
 */
Py_UCS2 fromJPChar(PyJPChar2 *self)
{
	if (_PyUnicode_STATE(self).ascii == 1)
	{
		Py_UCS1 *data = (Py_UCS1*) (((PyASCIIObject*) self) + 1);
		return data[0];
	}
	if (_PyUnicode_STATE(self).kind == PyUnicode_1BYTE_KIND)
	{
		Py_UCS1 *data = (Py_UCS1*) ( ((PyCompactUnicodeObject*) self) + 1);
		return data[0];
	}
	Py_UCS2 *data = (Py_UCS2*) ( ((PyCompactUnicodeObject*) self) + 1);
	return data[0];
}

static PyObject * PyJPChar2_new(PyTypeObject *type, PyObject *pyargs, PyObject * kwargs)
{
	JP_PY_TRY("PyJPObject_new");
	// Get the Java class from the type.
	JPClass *cls = PyJPClass_getJPClass((PyObject*) type);
	if (cls == NULL)
		JP_RAISE(PyExc_TypeError, "Java class type is incorrect");

	JPContext *context = PyJPModule_getContext();

	// Create an instance (this may fail)
	JPJavaFrame frame(context);
	if (PyTuple_Size(pyargs) != 1)
	{
		PyErr_Format(PyExc_TypeError, "Java chars require one argument");
		return 0;
	}

	JPValue jv;
	PyObject *in = PyTuple_GetItem(pyargs, 0);
	Py_UCS4 cv = ord(in);
	if (cv != (Py_UCS4) - 1)
	{
		JPPyObject v(JPPyRef::_call, PyLong_FromLong(cv));
		JPPyTuple args0 = JPPyTuple::newTuple(1);
		args0.setItem(0, v.get());
		JPPyObjectVector args(args0.get());
		jv = cls->newInstance(frame, args);
	} else if (PyIndex_Check(in))
	{
		JPPyObjectVector args(pyargs);
		jv = cls->newInstance(frame, args);
	} else if (PyFloat_Check(in))
	{
		JPPyObject v(JPPyRef::_call, PyNumber_Long(in));
		JPPyTuple args0 = JPPyTuple::newTuple(1);
		args0.setItem(0, v.get());
		JPPyObjectVector args(args0.get());
		jv = cls->newInstance(frame, args);
	} else
	{
		// This is not strictly true as we can cast a float to a char
		PyErr_Format(PyExc_TypeError, "Java require index or str with length 1");
		return 0;
	}

	PyObject *self = PyJPChar2_Create(type, fromJPValue(jv));
	JP_PY_CHECK();
	PyJPValue_assignJavaSlot(frame, self, jv);
	return self;
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPChar2_str(PyJPChar2 *self)
{
	JP_PY_TRY("PyJPChar2_str");
	PyJPModule_getContext(); // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (javaSlot == NULL)
	{
		// A slot is required
		PyErr_Format(PyExc_TypeError, "Bad object");
		return 0;
	}
	JPClass *cls = javaSlot->getClass();
	if (!cls->isPrimitive() && javaSlot->getValue().l == 0)
		return JPPyString::fromStringUTF8("null").keep();
	return PyUnicode_FromOrdinal(fromJPChar(self));
	JP_PY_CATCH(NULL);
}

static int isNull(JPValue *javaSlot)
{
	if (javaSlot != NULL )
	{
		JPClass *cls = javaSlot->getClass();
		if (cls->isPrimitive() || javaSlot->getValue().l != NULL)
			return 0;
	}
	return 1;
}

static int assertNotNull(JPValue *javaSlot)
{
	if (!isNull(javaSlot))
		return 0;
	PyErr_Format(PyExc_TypeError, "cast of null pointer");
	return 1;
}

static PyObject *PyJPChar2_index(PyJPChar2 *self)
{
	JP_PY_TRY("PyJPChar2_index");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	return PyLong_FromLong(fromJPChar(self));
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPChar2_float(PyJPChar2 *self)
{
	JP_PY_TRY("PyJPChar2_float");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	return PyFloat_FromDouble(fromJPChar(self));
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPChar2_nop(PyJPChar2 *self)
{
	Py_INCREF(self);
	return (PyObject*) self;
}

static PyObject* PyJPChar_FromLong(long value)
{
	JP_PY_TRY("PyJPChar2_float");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	PyObject *self = PyJPChar2_Create(PyJPChar2_Type, value);
	jvalue jv;
	jv.c = value;
	PyJPValue_assignJavaSlot(frame, self, JPValue(context->_char, jv));
	return self;
	JP_PY_CATCH(NULL);
}

static  PyObject *PyJPChar2_and(PyJPChar2 *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar2_and");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	long v = PyLong_AsLong(other);
	if (v == -1  && PyErr_Occurred())
		return 0;
	return PyJPChar_FromLong(fromJPChar(self) & v);
	JP_PY_CATCH(NULL);
}

static  PyObject *PyJPChar2_or(PyJPChar2 *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar2_or");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	long v = PyLong_AsLong(other);
	if (v == -1  && PyErr_Occurred())
		return 0;
	return PyJPChar_FromLong(fromJPChar(self) | v);
	JP_PY_CATCH(NULL);
}

static  PyObject *PyJPChar2_xor(PyJPChar2 *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar2_xor");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	long v = PyLong_AsLong(other);
	if (v == -1  && PyErr_Occurred())
		return 0;
	return PyJPChar_FromLong(fromJPChar(self) ^ v);
	JP_PY_CATCH(NULL);
}

static  PyObject *PyJPChar2_add(PyJPChar2 *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar2_add");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	long v = PyLong_AsLong(other);
	if (v == -1  && PyErr_Occurred())
		return 0;
	return PyJPChar_FromLong(fromJPChar(self) + v);
	JP_PY_CATCH(NULL);
}

static  PyObject *PyJPChar2_subtract(PyJPChar2 *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar2_subtract");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	long v = PyLong_AsLong(other);
	if (v == -1  && PyErr_Occurred())
		return 0;
	return PyJPChar_FromLong(fromJPChar(self) - v);
	JP_PY_CATCH(NULL);
}

static  PyObject *PyJPChar2_mult(PyJPChar2 *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar2_mult");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	long v = PyLong_AsLong(other);
	if (v == -1  && PyErr_Occurred())
		return 0;
	return PyJPChar_FromLong(fromJPChar(self) * v);
	JP_PY_CATCH(NULL);
}

static  PyObject *PyJPChar2_rshift(PyJPChar2 *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar2_rshift");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	long v = PyLong_AsLong(other);
	if (v == -1  && PyErr_Occurred())
		return 0;
	return PyJPChar_FromLong(fromJPChar(self) >> v);
	JP_PY_CATCH(NULL);
}

static  PyObject *PyJPChar2_lshift(PyJPChar2 *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar2_lshift");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	long v = PyLong_AsLong(other);
	if (v == -1  && PyErr_Occurred())
		return 0;
	return PyJPChar_FromLong(fromJPChar(self) << v);
	JP_PY_CATCH(NULL);
}

static const char* op_names[] = {
	"<", "<=", "==", "!=", ">", ">="
};

static PyObject *PyJPJChar_compare(PyObject *self, PyObject *other, int op)
{
	JP_PY_TRY("PyJPJChar_compare");
	PyJPModule_getContext();  // Check that JVM is running
	//	JPJavaFrame frame(context);
	JPValue *javaSlot1 = PyJPValue_getJavaSlot(other);
	JPValue *javaSlot0 = PyJPValue_getJavaSlot(self);
	if (isNull(javaSlot0))
	{
		if (isNull(javaSlot1))
			other = Py_None;
		if (op == Py_EQ)
			return PyBool_FromLong(other == Py_None );
		if (op == Py_NE)
			return PyBool_FromLong(other != Py_None);
		PyErr_Format(PyExc_TypeError, "'%s' not supported with null pointer", op_names[op]);
		JP_RAISE_PYTHON();
	}
	Py_UCS2 v0 = fromJPChar((PyJPChar2 *) self);
	if (javaSlot1 != NULL && isNull(javaSlot1))
		return PyBool_FromLong(op == Py_NE);
	Py_UCS4 v1 = ord(other);
	if (v1 == (Py_UCS4) - 1 && PyIndex_Check(other))
		v1 = (Py_UCS4) PyLong_AsLong(other);

	switch (op)
	{
		case Py_EQ:
			return PyBool_FromLong(v0 == v1);
		case Py_NE:
			return PyBool_FromLong(v0 != v1);
		case Py_LT:
			return PyBool_FromLong(v0 < v1);
		case Py_LE:
			return PyBool_FromLong(v0 <= v1);
		case Py_GT:
			return PyBool_FromLong(v0 > v1);
		case Py_GE:
			return PyBool_FromLong(v0 >= v1);
		default:  // GCOVR_EXCL_LINE
			Py_RETURN_FALSE; // GCOVR_EXCL_LINE
	}
	JP_PY_CATCH(NULL);
}

static Py_hash_t PyJPChar_hash(PyObject *self)
{
	JP_PY_TRY("PyJPObject_hash");
	PyJPModule_getContext();  // Check that JVM is running
	//	JPJavaFrame frame(context);
	JPValue *javaSlot = PyJPValue_getJavaSlot(self);
	if (isNull(javaSlot))
		return Py_TYPE(Py_None)->tp_hash(Py_None);
	return PyUnicode_Type.tp_hash((PyObject*) self);
	JP_PY_CATCH(0);
}

static int PyJPChar_bool(PyJPChar2 *self)
{
	JP_PY_TRY("PyJPObject_bool");
	PyJPModule_getContext();  // Check that JVM is running
	//	JPJavaFrame frame(context);
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (isNull(javaSlot))
		return 0;
	return fromJPChar(self) != 0;
	JP_PY_CATCH(0);
}

static PyObject *PyJPChar_neg(PyJPChar2 *self)
{
	JP_PY_TRY("PyJPObject_neg");
	PyJPModule_getContext();  // Check that JVM is running
	//	JPJavaFrame frame(context);
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	return PyJPChar_FromLong(-fromJPChar(self));
	JP_PY_CATCH(0);
}

static PyMethodDef charMethods[] = {
	//	{"thing", (PyCFunction) PyJPMethod_matchReport, METH_VARARGS, ""},
	{NULL},
};

struct PyGetSetDef charGetSet[] = {
	//	{"thing", (getter) PyJPMethod_getSelf, NULL, NULL, NULL},
	{NULL},
};

static PyType_Slot charSlots[] = {
	{Py_tp_new,       (void*) PyJPChar2_new},
	{Py_tp_methods,   (void*) charMethods},
	{Py_tp_getset,    (void*) charGetSet},
	{Py_tp_str,       (void*) PyJPChar2_str},
	{Py_nb_index,     (void*) PyJPChar2_index},
	{Py_nb_float,     (void*) PyJPChar2_float},
	{Py_nb_absolute,  (void*) PyJPChar2_nop}, // Always positive
	{Py_nb_and,       (void*) PyJPChar2_and},
	{Py_nb_or,        (void*) PyJPChar2_or},
	{Py_nb_xor,       (void*) PyJPChar2_xor},
	{Py_nb_add,       (void*) PyJPChar2_add},
	{Py_nb_subtract,  (void*) PyJPChar2_subtract},
	{Py_nb_multiply,  (void*) PyJPChar2_mult},
	{Py_nb_rshift,    (void*) PyJPChar2_rshift},
	{Py_nb_lshift,    (void*) PyJPChar2_lshift},
	{Py_tp_richcompare, (void*) PyJPJChar_compare},
	{Py_tp_hash,      (void*) PyJPChar_hash},
	{Py_nb_bool,      (void*) PyJPChar_bool},
	{Py_nb_negative,  (void*) PyJPChar_neg},
	{Py_tp_getattro,  (void*) &PyJPValue_getattro},
	{Py_tp_setattro,  (void*) &PyJPValue_setattro},
	{0}
};

static PyType_Spec charSpec = {
	"_jpype._JChar2",
	sizeof (PyJPChar2),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // | Py_TPFLAGS_HAVE_GC,
	charSlots
};

#ifdef __cplusplus
}
#endif

void PyJPChar2_initType(PyObject* module)
{
	// We will inherit from str and JObject
	PyObject *bases = PyTuple_Pack(2, &PyUnicode_Type, PyJPObject_Type);
	PyJPChar2_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&charSpec, bases);
	Py_DECREF(bases);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JChar2", (PyObject*) PyJPChar2_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
}

