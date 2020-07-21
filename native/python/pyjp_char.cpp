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
#include <structmember.h>
#include "jp_boxedtype.h"

#ifdef __cplusplus
extern "C"
{
#endif

PyTypeObject *PyJPChar_Type = NULL;

struct PyJPChar
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
	PyErr_SetString(PyExc_TypeError, "cast of null pointer");
	return 1;
}

PyObject *PyJPChar_Create(PyTypeObject *type, Py_UCS2 p)
{
	PyJPChar  *self = (PyJPChar*) PyJPValue_alloc(type, 0);
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
Py_UCS2 fromJPChar(PyJPChar *self)
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

static PyObject * PyJPChar_new(PyTypeObject *type, PyObject *pyargs, PyObject * kwargs)
{
	JP_PY_TRY("PyJPChar_new");
	// Get the Java class from the type.
	JPClass *cls = PyJPClass_getJPClass((PyObject*) type);
	if (cls == NULL)
	{  // GCOVR_EXCL_START
		PyErr_SetString(PyExc_TypeError, "Java class type is incorrect");
		return 0;
	}  // GCOVR_EXCL_STOP

	JPContext *context = PyJPModule_getContext();

	// Create an instance (this may fail)
	JPJavaFrame frame = JPJavaFrame::outer(context);
	if (PyTuple_Size(pyargs) != 1)
	{
		PyErr_SetString(PyExc_TypeError, "Java chars require one argument");
		return 0;
	}

	JPValue jv;
	PyObject *in = PyTuple_GetItem(pyargs, 0);
	Py_UCS4 cv = ord(in);
	if (cv != (Py_UCS4) - 1)
	{
		JPPyObject v = JPPyObject::call(PyLong_FromLong(cv));
		JPPyObject args0 = JPPyObject::call(PyTuple_Pack(1, v.get()));
		JPPyObjectVector args(args0.get());
		jv = cls->newInstance(frame, args);
	} else if (PyIndex_Check(in))
	{
		JPPyObjectVector args(pyargs);
		jv = cls->newInstance(frame, args);
	} else if (PyFloat_Check(in))
	{
		JPPyObject v = JPPyObject::call(PyNumber_Long(in));
		JPPyObject args0 = JPPyObject::call(PyTuple_Pack(1, v.get()));
		JPPyObjectVector args(args0.get());
		jv = cls->newInstance(frame, args);
	} else
	{
		// This is not strictly true as we can cast a float to a char
		PyErr_SetString(PyExc_TypeError, "Java require index or str with length 1");
		return 0;
	}

	PyObject *self = PyJPChar_Create(type, fromJPValue(jv));
	JP_PY_CHECK();
	PyJPValue_assignJavaSlot(frame, self, jv);
	return self;
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static PyObject *PyJPChar_str(PyJPChar *self)
{
	JP_PY_TRY("PyJPChar_str");
	PyJPModule_getContext(); // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (javaSlot == NULL)
	{  // GCOVR_EXCL_START
		// A slot is required
		PyErr_SetString(PyExc_TypeError, "Java slot is not set on Java char");
		return 0;
	}  // GCOVR_EXCL_STOP
	if (isNull(javaSlot))
		return JPPyString::fromStringUTF8("None").keep();
	return PyUnicode_FromOrdinal(fromJPChar(self));
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static PyObject *PyJPChar_repr(PyJPChar *self)
{
	JP_PY_TRY("PyJPChar_repr");
	PyJPModule_getContext(); // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (javaSlot == NULL)
	{  // GCOVR_EXCL_START
		// A slot is required
		PyErr_SetString(PyExc_TypeError, "Java slot is not set on Java char");
		return 0;
	}  // GCOVR_EXCL_STOP
	if (isNull(javaSlot))
		return JPPyString::fromStringUTF8("None").keep();
	return PyUnicode_Type.tp_repr((PyObject*) self);
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static PyObject *PyJPChar_index(PyJPChar *self)
{
	JP_PY_TRY("PyJPChar_index");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	return PyLong_FromLong(fromJPChar(self));
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static PyObject *PyJPChar_float(PyJPChar *self)
{
	JP_PY_TRY("PyJPChar_float");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	return PyFloat_FromDouble(fromJPChar(self));
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static PyObject *PyJPChar_abs(PyJPChar *self)
{
	JP_PY_TRY("PyJPChar_nop");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;

	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar(self)));
	return PyLong_Type.tp_as_number->nb_absolute(v.get());
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static Py_ssize_t PyJPChar_len(PyJPChar *self)
{
	JP_PY_TRY("PyJPChar_nop");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return -1;
	return 1;
	JP_PY_CATCH(-1);  // GCOVR_EXCL_LINE
}

static  PyObject *PyJPChar_and(PyJPChar *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar_and");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar(self)));
	return PyNumber_And(v.get(), other);
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static  PyObject *PyJPChar_or(PyJPChar *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar_or");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar(self)));
	return PyNumber_Or(v.get(), other);
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static  PyObject *PyJPChar_xor(PyJPChar *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar_xor");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar(self)));
	return PyNumber_Xor(v.get(), other);
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static  PyObject *PyJPChar_add(PyJPChar *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar_add");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	if (PyUnicode_Check(other))
		return PyUnicode_Type.tp_as_number->nb_add((PyObject*) self, other);

	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar(self)));
	return PyNumber_Add(v.get(), other);
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static  PyObject *PyJPChar_subtract(PyJPChar *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar_subtract");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;

	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar(self)));
	return PyNumber_Subtract(v.get(), other);
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static  PyObject *PyJPChar_mult(PyJPChar *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar_mult");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;

	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar(self)));
	return PyNumber_Multiply(v.get(), other);
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static  PyObject *PyJPChar_rshift(PyJPChar *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar_rshift");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;

	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar(self)));
	return PyNumber_Rshift(v.get(), other);
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static  PyObject *PyJPChar_lshift(PyJPChar *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar_lshift");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;

	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar(self)));
	return PyNumber_Lshift(v.get(), other);
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static  PyObject *PyJPChar_floordiv(PyObject *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar_floordiv");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot(self);
	if (javaSlot == NULL)
	{
		javaSlot = PyJPValue_getJavaSlot(other);
		if (assertNotNull(javaSlot))
			return 0;
		JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar((PyJPChar*) other)));
		return PyNumber_FloorDivide(self, v.get());
	}
	if (assertNotNull(javaSlot))
		return 0;

	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar((PyJPChar*) self)));
	return PyNumber_FloorDivide(v.get(), other);
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static  PyObject *PyJPChar_divmod(PyObject *self, PyObject *other)
{
	JP_PY_TRY("PyJPChar_divmod");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot( self);
	if (javaSlot == NULL)
	{
		javaSlot = PyJPValue_getJavaSlot(other);
		if (assertNotNull(javaSlot))
			return 0;
		JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar((PyJPChar*) other)));
		return PyNumber_Divmod(self, v.get());
	}
	if (assertNotNull(javaSlot))
		return 0;

	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar((PyJPChar*) self)));
	return PyNumber_Divmod(v.get(), other);
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static PyObject *PyJPChar_neg(PyJPChar *self)
{
	JP_PY_TRY("PyJPChar_neg");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar(self)));
	return PyNumber_Negative(v.get());
	JP_PY_CATCH(0);  // GCOVR_EXCL_LINE
}

static PyObject *PyJPChar_pos(PyJPChar *self)
{
	JP_PY_TRY("PyJPChar_pos");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar(self)));
	return PyNumber_Positive(v.get());
	JP_PY_CATCH(0);  // GCOVR_EXCL_LINE
}

static PyObject *PyJPChar_inv(PyJPChar *self)
{
	JP_PY_TRY("PyJPObject_neg");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (assertNotNull(javaSlot))
		return 0;
	// Promote to int as per Java rules
	JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar(self)));
	return PyNumber_Invert(v.get());
	JP_PY_CATCH(0);  // GCOVR_EXCL_LINE
}

static PyObject *PyJPJChar_compare(PyObject *self, PyObject *other, int op)
{
	JP_PY_TRY("PyJPJChar_compare");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot1 = PyJPValue_getJavaSlot(other);
	JPValue *javaSlot0 = PyJPValue_getJavaSlot(self);
	if (isNull(javaSlot0))
	{
		if (javaSlot1 != NULL && isNull(javaSlot1))
			other = Py_None;
		if (op == Py_EQ)
			return PyBool_FromLong(other == Py_None );
		if (op == Py_NE)
			return PyBool_FromLong(other != Py_None);
		PyObject *out = Py_NotImplemented;
		Py_INCREF(out);
		return out;
		JP_RAISE_PYTHON();
	}
	if (javaSlot1 != NULL && isNull(javaSlot1))
		return PyBool_FromLong(op == Py_NE);

	if (PyUnicode_Check(other))
	{
		// char <=> char
		// char <=> str
		return PyUnicode_Type.tp_richcompare(self, other, op);
	}
	if (PyFloat_Check(other))
	{
		JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar((PyJPChar*) self)));
		if (op < 2)
			op += 4;
		else if (op > 3)
			op -= 4;
		return PyFloat_Type.tp_richcompare(other, v.get(), op);
	}
	if (PyNumber_Check(other))
	{
		JPPyObject v = JPPyObject::call(PyLong_FromLong(fromJPChar((PyJPChar*) self)));
		return PyLong_Type.tp_richcompare(v.get(), other, op);
	}
	if (javaSlot1 != NULL)
	{
		// char  <=> object
		// object <=> char
		// object <=> object
		switch (op)
		{
			case Py_EQ:
				Py_RETURN_FALSE;
			case Py_NE:
				Py_RETURN_TRUE;
		}
		PyObject *out = Py_NotImplemented;
		Py_INCREF(out);
		return out;
	}

	PyObject *out = Py_NotImplemented;
	Py_INCREF(out);
	return out;
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static Py_hash_t PyJPChar_hash(PyObject *self)
{
	JP_PY_TRY("PyJPObject_hash");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot(self);
	if (isNull(javaSlot))
		return Py_TYPE(Py_None)->tp_hash(Py_None);
	return PyUnicode_Type.tp_hash((PyObject*) self);
	JP_PY_CATCH(0);  // GCOVR_EXCL_LINE
}

static int PyJPChar_bool(PyJPChar *self)
{
	JP_PY_TRY("PyJPObject_bool");
	PyJPModule_getContext();  // Check that JVM is running
	JPValue *javaSlot = PyJPValue_getJavaSlot((PyObject*) self);
	if (isNull(javaSlot))
		return 0;
	return fromJPChar(self) != 0;
	JP_PY_CATCH(0);  // GCOVR_EXCL_LINE
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
	{Py_tp_new,       (void*) PyJPChar_new},
	{Py_tp_methods,   (void*) charMethods},
	{Py_tp_getset,    (void*) charGetSet},
	{Py_tp_str,       (void*) PyJPChar_str},
	{Py_tp_repr,      (void*) PyJPChar_repr},
	{Py_nb_index,     (void*) PyJPChar_index},
#if PY_VERSION_HEX<0x03080000
	{Py_nb_int,     (void*) PyJPChar_index},
#endif
	{Py_nb_float,     (void*) PyJPChar_float},
	{Py_nb_absolute,  (void*) PyJPChar_abs},
	{Py_nb_and,       (void*) PyJPChar_and},
	{Py_nb_or,        (void*) PyJPChar_or},
	{Py_nb_xor,       (void*) PyJPChar_xor},
	{Py_nb_add,       (void*) PyJPChar_add},
	{Py_nb_subtract,  (void*) PyJPChar_subtract},
	{Py_nb_multiply,  (void*) PyJPChar_mult},
	{Py_nb_rshift,    (void*) PyJPChar_rshift},
	{Py_nb_lshift,    (void*) PyJPChar_lshift},
	{Py_tp_richcompare, (void*) PyJPJChar_compare},
	{Py_tp_hash,      (void*) PyJPChar_hash},
	{Py_nb_bool,      (void*) PyJPChar_bool},
	{Py_nb_negative,  (void*) PyJPChar_neg},
	{Py_nb_positive,  (void*) PyJPChar_pos},
	{Py_nb_invert,    (void*) PyJPChar_inv},
	{Py_nb_floor_divide, (void*) PyJPChar_floordiv},
	{Py_nb_divmod, (void*) PyJPChar_divmod},
	{Py_tp_getattro,  (void*) PyJPValue_getattro},
	{Py_tp_setattro,  (void*) PyJPValue_setattro},
	{ Py_sq_length,   (void*) PyJPChar_len},
	{0}
};

static PyType_Spec charSpec = {
	"_jpype._JChar",
	sizeof (PyJPChar),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // | Py_TPFLAGS_HAVE_GC,
	charSlots
};

#ifdef __cplusplus
}
#endif

void PyJPChar_initType(PyObject* module)
{
	// We will inherit from str and JObject
	PyObject *bases = PyTuple_Pack(2, &PyUnicode_Type, PyJPObject_Type);
	PyJPChar_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&charSpec, bases);
	Py_DECREF(bases);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JChar", (PyObject*) PyJPChar_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
}
