/*
 * Copyright 2020 Karl Einar Nelson
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

#ifdef __cplusplus
extern "C"
{
#endif

static PyObject *PyJPObject_new(PyTypeObject *type, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPObject_new");
	// Get the Java class from the type.
	JPClass *cls = PyJPClass_getJPClass((PyObject*) type);
	if (cls == NULL)
	{
		PyErr_SetString(PyExc_TypeError, "Java class type is incorrect");
		return NULL;
	}

	// Create an instance (this may fail)
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	JPPyObjectVector args(pyargs);
	JPValue jv = cls->newInstance(frame, args);

	// If it succeeded then allocate memory
	PyObject *self = type->tp_alloc(type, 0);
	JP_PY_CHECK();

	JP_FAULT_RETURN("PyJPObject_init.null", self);
	PyJPValue_assignJavaSlot(frame, self, jv);
	return self;
	JP_PY_CATCH(NULL);
}

static PyObject *PyJPObject_compare(PyObject *self, PyObject *other, int op)
{
	JP_PY_TRY("PyJPObject_compare");
	if (op == Py_NE)
	{
		PyObject *ret = PyJPObject_compare(self, other, Py_EQ);
		if (ret == NULL)
			return NULL;
		int rc = (ret == Py_False);
		Py_DECREF(ret);
		return PyBool_FromLong(rc);
	}
	if (op != Py_EQ)
	{
		PyObject *out = Py_NotImplemented;
		Py_INCREF(out);
		return out;
	}

	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	JPValue *javaSlot0 = PyJPValue_getJavaSlot(self);
	JPValue *javaSlot1 = PyJPValue_getJavaSlot(other);

	// First slot is Null
	if (javaSlot0 == NULL || javaSlot0->getValue().l == NULL)
	{
		if (javaSlot1 == NULL)
			return PyBool_FromLong(other == Py_None);
		if (javaSlot1->getClass()->isPrimitive())
			Py_RETURN_FALSE;
		if (javaSlot1->getValue().l == NULL)
			Py_RETURN_TRUE;
		Py_RETURN_FALSE;
	}

	// Check second slot is Null
	if (other == Py_None)
		Py_RETURN_FALSE;
	if (javaSlot1 == NULL)
	{
		// This block seems like a giant waste as there are very few cases in which
		// a converted object would ever satisfy equals.  But this was the original
		// logic in JPype so we will try to match it.
		JPMatch match(&frame, other);
		javaSlot0->getClass()->findJavaConversion(match);
		if (match.type < JPMatch::_implicit)
			Py_RETURN_FALSE;
		return PyBool_FromLong(frame.equals(javaSlot0->getValue().l, match.convert().l));
	}
	if (javaSlot1->getClass()->isPrimitive())
		Py_RETURN_FALSE;
	if (javaSlot1->getValue().l == NULL)
		Py_RETURN_FALSE;

	return PyBool_FromLong(frame.equals(javaSlot0->getValue().l, javaSlot1->getValue().l));
	JP_PY_CATCH(0); // GCOVR_EXCL_LINE
}

static PyObject *PyJPComparable_compare(PyObject *self, PyObject *other, int op)
{
	JP_PY_TRY("PyJPComparable_compare");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	JPValue *javaSlot0 = PyJPValue_getJavaSlot(self);
	JPValue *javaSlot1 = PyJPValue_getJavaSlot(other);

	bool null0 = false;
	bool null1 = false;

	// First slot is Null
	if (self == Py_None || javaSlot0 == NULL ||
			(!javaSlot0->getClass()->isPrimitive() && javaSlot0->getValue().l == NULL))
		null0  = true;
	if (other == Py_None || (javaSlot1 != NULL &&
			!javaSlot1->getClass()->isPrimitive() && javaSlot1->getValue().l == NULL))
		null1  = true;

	jobject obj0 = NULL;
	jobject obj1 = NULL;

	if (!null0)
		obj0 = javaSlot0->getValue().l;

	if (!null0 && !null1 && javaSlot1 == NULL)
	{
		// Okay here is the hard part.  We need to figure out what type
		// of object to create to make them comparable.  We can't assume
		// the most derived type because some classes inherit comparable
		// and that would require the derived type.  We have to find
		// the first super class that implements Comparable.  Further,
		// because of type erasure we can't actually get.
		JPClass *cls2 = javaSlot0->getClass();
		JPMatch match(&frame, other);
		while (cls2 != NULL && !cls2->findJavaConversion(match) && !JPModifier::isComparable(cls2->getModifiers()))
			cls2 = cls2->getSuperClass();

		// This should never happen.
		if (cls2 == NULL)
		{
			PyObject *out = Py_NotImplemented;
			Py_INCREF(out);
			return out;
		}
		obj1 = match.convert().l;
	} else if (!null1 && javaSlot1 != NULL)
		obj1 = javaSlot1->getValue().l;

	switch (op)
	{
		case Py_EQ:
			if (null0 && null1)
				Py_RETURN_TRUE;
			if (null0 || null1)
				Py_RETURN_FALSE;
			return PyBool_FromLong(frame.compareTo(obj0, obj1) == 0);

		case Py_NE:
			if (null0 && null1)
				Py_RETURN_FALSE;
			if (null0 || null1)
				Py_RETURN_TRUE;
			return PyBool_FromLong(frame.compareTo(obj0, obj1) != 0);
		case Py_LT:
			if (null0 || null1)
				break;
			return PyBool_FromLong(frame.compareTo(obj0, obj1) < 0);
		case Py_LE:
			if (null0 || null1)
				break;
			return PyBool_FromLong(frame.compareTo(obj0, obj1) <= 0);
		case Py_GT:
			if (null0 || null1)
				break;
			return PyBool_FromLong(frame.compareTo(obj0, obj1) > 0);
		case Py_GE:
			if (null0 || null1)
				break;
			return PyBool_FromLong(frame.compareTo(obj0, obj1) >= 0);
	}
	PyErr_SetString(PyExc_ValueError, "can't compare null");
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static Py_hash_t PyJPObject_hash(PyObject *obj)
{
	JP_PY_TRY("PyJPObject_hash");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	JPValue *javaSlot = PyJPValue_getJavaSlot(obj);
	if (javaSlot == NULL)
		return Py_TYPE(Py_None)->tp_hash(Py_None);
	jobject o = javaSlot->getJavaObject();
	if (o == NULL)
		return Py_TYPE(Py_None)->tp_hash(Py_None);
	return frame.hashCode(o);
	JP_PY_CATCH(0);
}

static PyObject *PyJPObject_repr(PyObject *self)
{
	JP_PY_TRY("PyJPObject_repr");
	return PyUnicode_FromFormat("<java object '%s'>", Py_TYPE(self)->tp_name);
	JP_PY_CATCH(0); // GCOVR_EXCL_LINE
}

static PyType_Slot objectSlots[] = {
	{Py_tp_new,      (void*) &PyJPObject_new},
	{Py_tp_free,     (void*) &PyJPValue_free},
	{Py_tp_getattro, (void*) &PyJPValue_getattro},
	{Py_tp_setattro, (void*) &PyJPValue_setattro},
	{Py_tp_str,      (void*) &PyJPValue_str},
	{Py_tp_repr,     (void*) &PyJPObject_repr},
	{Py_tp_richcompare, (void*) &PyJPObject_compare},
	{Py_tp_hash,     (void*) &PyJPObject_hash},
	{0}
};

PyTypeObject *PyJPObject_Type = NULL;
static PyType_Spec objectSpec = {
	"_jpype._JObject",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	objectSlots
};

static PyObject *PyJPException_new(PyTypeObject *type, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPException_new");
	// Get the Java class from the type.
	JPClass *cls = PyJPClass_getJPClass((PyObject*) type);
	if (cls == NULL)
	{
		PyErr_SetString(PyExc_TypeError, "Java class type is incorrect");
		return NULL;
	}

	// Special constructor path for Exceptions
	JPPyObjectVector args(pyargs);
	if (args.size() == 2 && args[0] == _JObjectKey)
		return ((PyTypeObject*) PyExc_BaseException)->tp_new(type, args[1], kwargs);

	// Create an instance (this may fail)
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	JPValue jv = cls->newInstance(frame, args);

	// Exception must be constructed with the BaseException_new
	PyObject *self = ((PyTypeObject*) PyExc_BaseException)->tp_new(type, pyargs, kwargs);
	JP_PY_CHECK();

	JP_FAULT_RETURN("PyJPException_init.null", self);
	PyJPValue_assignJavaSlot(frame, self, jv);
	return self;
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

static int PyJPException_init(PyObject *self, PyObject *pyargs, PyObject *kwargs)
{
	JP_PY_TRY("PyJPException_init");
	JPPyObjectVector args(pyargs);
	if (args.size() == 2 && args[0] == _JObjectKey)
		return ((PyTypeObject*) PyExc_BaseException)->tp_init(self, args[1], kwargs);

	// Exception must be constructed with the BaseException_new
	return ((PyTypeObject*) PyExc_BaseException)->tp_init(self, pyargs, kwargs);
	JP_PY_CATCH(-1);  // GCOVR_EXCL_LINE
}

static PyObject* PyJPException_expandStacktrace(PyObject* self)
{
	JP_PY_TRY("PyJPModule_expandStackTrace");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	JPValue *val = PyJPValue_getJavaSlot(self);

	// These two are loop invariants and must match each time
	jthrowable th = (jthrowable) val->getValue().l;
	JPPyObject exc = JPPyObject::use(self);
	PyJPException_normalize(frame, exc, th, NULL);

	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);  // GCOVR_EXCL_LINE
}

PyObject *PyJPException_args(PyBaseExceptionObject *self)
{
	if (self->args == NULL)
		Py_RETURN_NONE;
	Py_INCREF(self->args);
	return self->args;
}

static PyMethodDef exceptionMethods[] = {
	{"_expandStacktrace", (PyCFunction) PyJPException_expandStacktrace, METH_NOARGS, ""},
	{NULL},
};

static PyGetSetDef exceptionGetSets[] = {
	{"_args", (getter) PyJPException_args, NULL, ""},
	{0}
};

PyTypeObject *PyJPException_Type = NULL;
static PyType_Slot excSlots[] = {
	{Py_tp_new,      (void*) &PyJPException_new},
	{Py_tp_init,     (void*) &PyJPException_init},
	{Py_tp_getattro, (void*) &PyJPValue_getattro},
	{Py_tp_setattro, (void*) &PyJPValue_setattro},
	{Py_tp_methods,  exceptionMethods},
	{Py_tp_getset,   exceptionGetSets},
	{0}
};

static PyType_Spec excSpec = {
	"_jpype._JException",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	excSlots
};

static PyType_Slot comparableSlots[] = {
	{Py_tp_richcompare, (void*) &PyJPComparable_compare},
	{Py_tp_hash,     (void*) &PyJPObject_hash},
	{0}
};

PyTypeObject *PyJPComparable_Type = NULL;
static PyType_Spec comparableSpec = {
	"_jpype._JComparable",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	comparableSlots
};

#ifdef __cplusplus
}
#endif

void PyJPObject_initType(PyObject* module)
{
	PyJPObject_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&objectSpec, NULL);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JObject", (PyObject*) PyJPObject_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE

	JPPyObject bases = JPPyObject::call(PyTuple_Pack(2, PyExc_Exception, PyJPObject_Type));
	PyJPException_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&excSpec, bases.get());
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JException", (PyObject*) PyJPException_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE

	bases = JPPyObject::call(PyTuple_Pack(1, PyJPObject_Type));
	PyJPComparable_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&comparableSpec, bases.get());
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JComparable", (PyObject*) PyJPComparable_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
}

/**
 * Attach stack frames and causes as required for a Python exception.
 */
void PyJPException_normalize(JPJavaFrame frame, JPPyObject exc, jthrowable th, jthrowable enclosing)
{
	JP_TRACE_IN("PyJPException_normalize");
	JPContext *context = frame.getContext();
	while (th != NULL)
	{
		// Attach the frame to first
		JPPyObject trace = PyTrace_FromJavaException(frame, th, enclosing);
		PyException_SetTraceback(exc.get(), trace.get());

		// Check for the next in the cause list
		enclosing = th;
		th = frame.getCause(th);
		if (th == NULL)
			return;
		jvalue v;
		v.l = (jobject) th;
		JPPyObject next = context->_java_lang_Object->convertToPythonObject(frame, v, false);

		// This may already be a Python exception
		JPValue *val = PyJPValue_getJavaSlot(next.get());
		if (val == NULL)
		{
			PyException_SetCause(exc.get(), next.keep());
			return;
		}
		next.incref();  // Set cause will steal our reference
		PyException_SetCause(exc.get(), next.get());
		exc = next;
	}
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}
