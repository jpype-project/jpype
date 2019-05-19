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
#include <pyjp.h>

static PyMethodDef classMethods[] = {
	{"toString", (PyCFunction) (&PyJPValue::toString), METH_NOARGS, ""},
	{"toUnicode", (PyCFunction) (&PyJPValue::toUnicode), METH_NOARGS, ""},
	{NULL},
};

PyTypeObject PyJPValue::Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	/* tp_name           */ "_jpype.PyJPValue",
	/* tp_basicsize      */ sizeof (PyJPValue),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPValue::__dealloc__,
	/* tp_print          */ 0,
	/* tp_getattr        */ 0,
	/* tp_setattr        */ 0,
	/* tp_compare        */ 0,
	/* tp_repr           */ 0,
	/* tp_as_number      */ 0,
	/* tp_as_sequence    */ 0,
	/* tp_as_mapping     */ 0,
	/* tp_hash           */ 0,
	/* tp_call           */ 0,
	/* tp_str            */ (reprfunc) PyJPValue::__str__,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT,
	/* tp_doc            */
	"Wrapper of a java value which holds a class and instance of an object \n"
	"or a primitive.  This object is always stored as the attributed \n"
	"__javavalue__.  Anything with this type with that attribute will be\n"
	"considered a java object wrapper.",
	/* tp_traverse       */ 0,
	/* tp_clear          */ 0,
	/* tp_richcompare    */ 0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter           */ 0,
	/* tp_iternext       */ 0,
	/* tp_methods        */ classMethods,
	/* tp_members        */ 0,
	/* tp_getset         */ 0,
	/* tp_base           */ 0,
	/* tp_dict           */ 0,
	/* tp_descr_get      */ 0,
	/* tp_descr_set      */ 0,
	/* tp_dictoffset     */ 0,
	/* tp_init           */ (initproc) PyJPValue::__init__,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPValue::__new__
};

// Static methods

void PyJPValue::initType(PyObject* module)
{
	PyType_Ready(&PyJPValue::Type);
	Py_INCREF(&PyJPValue::Type);
	PyModule_AddObject(module, "PyJPValue", (PyObject*) (&PyJPValue::Type));
}

bool PyJPValue::check(PyObject* o)
{
	return o->ob_type == &PyJPValue::Type;
}

// These are from the internal methods when we alreayd have the jvalue

JPPyObject PyJPValue::alloc(const JPValue& value)
{
	return alloc(value.getClass(), value.getValue());
}

JPPyObject PyJPValue::alloc(JPClass* cls, jvalue value)
{
	JPJavaFrame frame;
	JP_TRACE_IN("PyJPValue::alloc");
	PyJPValue* self = PyObject_New(PyJPValue, &PyJPValue::Type);
	JP_PY_CHECK();

	// If it is not a primitive we need to reference it
	if (dynamic_cast<JPPrimitiveType*> (cls) != cls)
		value.l = frame.NewGlobalRef(value.l);

	// New value instance
	self->m_Value = JPValue(cls, value);
	self->m_Cache = NULL;
	JP_TRACE("Value", self->m_Value.getClass(), &(self->m_Value.getValue()));
	return JPPyObject(JPPyRef::_claim, (PyObject*) self);
	JP_TRACE_OUT;
}

PyObject* PyJPValue::__new__(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
	PyJPValue* self = (PyJPValue*) type->tp_alloc(type, 0);
	jvalue v;
	self->m_Value = JPValue(NULL, v);
	self->m_Cache = NULL;
	return (PyObject*) self;
}

// Replacement for convertToJava.

int PyJPValue::__init__(PyJPValue* self, PyObject* args, PyObject* kwargs)
{
	JP_TRACE_IN("PyJPValue::__init__");
	try
	{
		ASSERT_JVM_RUNNING("PyJPValue::__init__");
		JPJavaFrame frame;
		self->m_Cache = NULL;

		PyObject* claz;
		PyObject* value;

		if (!PyArg_ParseTuple(args, "O!O", &PyJPClass::Type, &claz, &value))
		{
			return -1;
		}

		JPClass* type = ((PyJPClass*) claz)->m_Class;
		ASSERT_NOT_NULL(value);
		ASSERT_NOT_NULL(type);

		// If it is already a Java object, then let Java decide
		// if the cast is possible
		JPValue* jval = JPPythonEnv::getJavaValue(value);
		if (jval != NULL && type->isInstance(*jval))
		{
			jvalue v = jval->getValue();
			v.l = frame.NewGlobalRef(v.l);
			self->m_Value = JPValue(type, v);
			return 0;
		}

		// Otherwise, see if we can convert it
		if (type->canConvertToJava(value) == JPMatch::_none)
		{
			stringstream ss;
			ss << "Unable to convert " << value->ob_type->tp_name << " to java type " << type->toString();
			PyErr_SetString(PyExc_TypeError, ss.str().c_str());
			return -1;
		}

		jvalue v = type->convertToJava(value);
		if (dynamic_cast<JPPrimitiveType*> (type) != type)
			v.l = frame.NewGlobalRef(v.l);
		self->m_Value = JPValue(type, v);
		return 0;
	}
	PY_STANDARD_CATCH;
	return -1;
	JP_TRACE_OUT;
}

PyObject* PyJPValue::__str__(PyJPValue* self)
{
	try
	{
		ASSERT_JVM_RUNNING("PyJPValue::__str__");
		JPJavaFrame frame;
		stringstream sout;
		sout << "<java value " << self->m_Value.getClass()->toString();

		// FIXME Remove these extra diagnostic values
		if (dynamic_cast<JPPrimitiveType*> (self->m_Value.getClass()) != NULL)
			sout << endl << "  value = primitive";
		else
		{
			jobject jo = self->m_Value.getJavaObject();
			sout << "  value = " << jo << " " << JPJni::toString(jo);
		}

		sout << ">";
		return JPPyString::fromStringUTF8(sout.str()).keep();
	}
	PY_STANDARD_CATCH;
	return 0;
}

void PyJPValue::__dealloc__(PyJPValue* self)
{
	JP_TRACE_IN("PyJPValue::__dealloc__");
	JPValue& value = self->m_Value;
	JPClass* cls = value.getClass();
	JP_TRACE("Value", cls, &(value.getValue()));
	if (self->m_Cache != NULL)
	{
		Py_DECREF(self->m_Cache);
		self->m_Cache = NULL;
	}
	// This one can't check for initialized because we may need to delete a stale
	// resource after shutdown.
	if (cls != NULL && JPEnv::isInitialized() && dynamic_cast<JPPrimitiveType*> (cls) != cls)
	{
		// If the JVM has shutdown then we don't need to free the resource
		// FIXME there is a problem with initializing the sytem twice.
		// Once we shut down the cls type goes away so this will fail.  If
		// we then reinitialize we will access this bad resource.  Not sure
		// of an easy solution.
		JP_TRACE("Dereference object");
		JPJavaFrame::ReleaseGlobalRef(value.getValue().l);
	}
	JP_TRACE("free", Py_TYPE(self)->tp_free);
	Py_TYPE(self)->tp_free(self);
	JP_TRACE_OUT;
}

void ensureCache(PyJPValue* self)
{
	if (self->m_Cache != NULL)
		return;
	self->m_Cache = PyDict_New();
}

/** This is the way to convert an object into a python string. */
PyObject* PyJPValue::toString(PyJPValue* self)
{
	JP_TRACE_IN("PyJPValue::toString");
	try
	{
		ASSERT_JVM_RUNNING("PyJPValue::toString");
		JPJavaFrame frame;
		JPClass* cls = self->m_Value.getClass();
		if (cls == JPTypeManager::_java_lang_String)
		{
			// Java strings are immutable so we will cache them.
			ensureCache(self);
			PyObject* out;
			out = PyDict_GetItemString(self->m_Cache, "str"); // Borrowed reference
			if (out == NULL)
			{
				jstring str = (jstring) self->m_Value.getValue().l;
				if (str == NULL)
					JP_RAISE_VALUE_ERROR("null string");
				PyDict_SetItemString(self->m_Cache, "str", out = JPPyString::fromStringUTF8(JPJni::toStringUTF8(str)).keep());
			}
			Py_INCREF(out);
			return out;

		}
		if (dynamic_cast<JPPrimitiveType*> (cls) != 0)
			JP_RAISE_VALUE_ERROR("toString requires a java object");
		if (cls == NULL)
			JP_RAISE_VALUE_ERROR("toString called with null class");

		// In general toString is not immutable, so we won't cache it.
		return JPPyString::fromStringUTF8(JPJni::toString(self->m_Value.getValue().l)).keep();
	}
	PY_STANDARD_CATCH;
	return 0;
	JP_TRACE_OUT;
}

/** This is the way to convert an object into a python string. */
PyObject* PyJPValue::toUnicode(PyJPValue* self)
{
	JP_TRACE_IN("PyJPValue::toUnicode");
	try
	{
		ASSERT_JVM_RUNNING("PyJPValue::toUnicode");
		JPJavaFrame frame;
		JPClass* cls = self->m_Value.getClass();
		if (cls == JPTypeManager::_java_lang_String)
		{
			// Java strings are immutable so we will cache them.
			ensureCache(self);
			PyObject* out;
			out = PyDict_GetItemString(self->m_Cache, "unicode"); // Borrowed reference
			if (out == NULL)
			{
				jstring str = (jstring) self->m_Value.getValue().l;
				if (str == NULL)
					JP_RAISE_VALUE_ERROR("null string");
				PyDict_SetItemString(self->m_Cache, "unicode", out = JPPyString::fromStringUTF8(JPJni::toStringUTF8(str), true).keep());
			}
			Py_INCREF(out);
			return out;

		}
		if (dynamic_cast<JPPrimitiveType*> (cls) != 0)
			JP_RAISE_VALUE_ERROR("toUnicode requires a java object");
		if (cls == NULL)
			JP_RAISE_VALUE_ERROR("toUnicode called with null class");

		// In general toString is not immutable, so we won't cache it.
		return JPPyString::fromStringUTF8(JPJni::toString(self->m_Value.getValue().l), true).keep();
	}
	PY_STANDARD_CATCH;
	return 0;
	JP_TRACE_OUT;
}


