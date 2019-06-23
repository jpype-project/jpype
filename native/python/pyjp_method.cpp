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
#include <pyjp.h>

static PyMethodDef methodMethods[] = {
	{"isBeanAccessor", (PyCFunction) (&PyJPMethod::isBeanAccessor), METH_NOARGS, ""},
	{"isBeanMutator", (PyCFunction) (&PyJPMethod::isBeanMutator), METH_NOARGS, ""},
	{"matchReport", (PyCFunction) (&PyJPMethod::matchReport), METH_VARARGS, ""},
	{"dump", (PyCFunction) (&PyJPMethod::dump), METH_NOARGS, ""},
	{NULL},
};

struct PyGetSetDef methodGetSet[] = {
	{"__self__", (getter) (&PyJPMethod::getSelf), NULL, NULL, NULL},
	{"__name__", (getter) (&PyJPMethod::getName), NULL, NULL, NULL},
	{"__doc__", (getter) (&PyJPMethod::getDoc), (setter) (&PyJPMethod::setDoc), NULL, NULL},
	{"__annotations__", (getter) (&PyJPMethod::getAnnotations), (setter) (&PyJPMethod::setAnnotations), NULL, NULL},
#if PY_MAJOR_VERSION >= 3
	{"__closure__", (getter) (&PyJPMethod::getClosure), NULL, NULL, NULL},
	{"__code__", (getter) (&PyJPMethod::getCode), NULL, NULL, NULL},
	{"__defaults__", (getter) (&PyJPMethod::getNone), NULL, NULL, NULL},
	{"__kwdefaults__", (getter) (&PyJPMethod::getNone), NULL, NULL, NULL},
	{"__globals__", (getter) (&PyJPMethod::getGlobals), NULL, NULL, NULL},
	{"__qualname__", (getter) (&PyJPMethod::getQualName), NULL, NULL, NULL},
#else
	{"func_closure", (getter) (&PyJPMethod::getClosure), NULL, NULL, NULL},
	{"func_code", (getter) (&PyJPMethod::getCode), NULL, NULL, NULL},
	{"func_defaults", (getter) (&PyJPMethod::getNone), NULL, NULL, NULL},
	{"func_doc", (getter) (&PyJPMethod::getDoc), (setter) (&PyJPMethod::setDoc), NULL, NULL},
	{"func_globals", (getter) (&PyJPMethod::getGlobals), NULL, NULL, NULL},
	{"func_name", (getter) (&PyJPMethod::getName), NULL, NULL, NULL},
#endif
	{NULL},
};


PyTypeObject PyJPMethod::Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	/* tp_name           */ "_jpype.PyJPMethod",
	/* tp_basicsize      */ sizeof (PyJPMethod),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPMethod::__dealloc__,
	/* tp_print          */ 0,
	/* tp_getattr        */ 0,
	/* tp_setattr        */ 0,
	/* tp_compare        */ 0,
	/* tp_repr           */ (reprfunc) PyJPMethod::__repr__,
	/* tp_as_number      */ 0,
	/* tp_as_sequence    */ 0,
	/* tp_as_mapping     */ 0,
	/* tp_hash           */ 0,
	/* tp_call           */ (ternaryfunc) PyJPMethod::__call__,
	/* tp_str            */ (reprfunc) PyJPMethod::__str__,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
	/* tp_doc            */ 0,
	/* tp_traverse       */ (traverseproc) PyJPMethod::traverse,
	/* tp_clear          */ (inquiry) PyJPMethod::clear,
	/* tp_richcompare    */ 0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter           */ 0,
	/* tp_iternext       */ 0,
	/* tp_methods        */ methodMethods,
	/* tp_members        */ 0,
	/* tp_getset         */ methodGetSet,
	/* tp_base           */ 0,
	/* tp_dict           */ 0,
	/* tp_descr_get      */ (descrgetfunc) PyJPMethod::__get__,
	/* tp_descr_set      */ 0,
	/* tp_dictoffset     */ 0,
	/* tp_init           */ 0,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPMethod::__new__
};

// Static methods

void PyJPMethod::initType(PyObject* module)
{
	// We inherit from PyFunction_Type just so we are an instatnce
	// for purposes of inspect and tab completion tools.  But
	// we will just ignore their memory layout as we have our own.
	PyJPMethod::Type.tp_base = &PyFunction_Type;
	PyType_Ready(&PyJPMethod::Type);
	Py_INCREF(&PyJPMethod::Type);
	PyModule_AddObject(module, "PyJPMethod", (PyObject*) (&PyJPMethod::Type));
}

JPPyObject PyJPMethod::alloc(JPMethodDispatch* m, PyObject* instance)
{
	JP_TRACE_IN_C("PyJPMethod::alloc");
	PyJPMethod *self = (PyJPMethod*) PyJPMethod::Type.tp_alloc(&PyJPMethod::Type, 0);
	JP_PY_CHECK();
	self->m_Method = m;
	self->m_Instance = instance;
	if (instance != NULL)
	{
		JP_TRACE_PY("method alloc (inc)", instance);
		Py_INCREF(instance);
	}
	self->m_Doc = NULL;
	self->m_Annotations = NULL;
	self->m_CodeRep = NULL;
	self->m_Context = (PyJPContext*) (m->getContext()->getHost());
	Py_INCREF(self->m_Context);
	JP_TRACE("self", self);
	return JPPyObject(JPPyRef::_claim, (PyObject*) self);
	JP_TRACE_OUT_C;
}

PyObject* PyJPMethod::__new__(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
	PyJPMethod* self = (PyJPMethod*) type->tp_alloc(type, 0);
	self->m_Method = NULL;
	self->m_Instance = NULL;
	self->m_Context = NULL;
	self->m_Doc = NULL;
	self->m_Annotations = NULL;
	self->m_CodeRep = NULL;
	return (PyObject*) self;
}

void PyJPMethod::__dealloc__(PyJPMethod* self)
{
	JP_TRACE_IN_C("PyJPMethod::__dealloc__", self);
	PyObject_GC_UnTrack(self);
	clear(self);
	Py_TYPE(self)->tp_free(self);
	JP_TRACE_OUT_C;
}

int PyJPMethod::traverse(PyJPMethod *self, visitproc visit, void *arg)
{
	JP_TRACE_IN_C("PyJPMethod::traverse", self);
	Py_VISIT(self->m_Instance);
	Py_VISIT(self->m_Context);
	Py_VISIT(self->m_Doc);
	Py_VISIT(self->m_Annotations);
	Py_VISIT(self->m_CodeRep);
	return 0;
	JP_TRACE_OUT_C;
}

int PyJPMethod::clear(PyJPMethod *self)
{
	JP_TRACE_IN_C("PyJPMethod::clear", self);
	Py_CLEAR(self->m_Instance);
	Py_CLEAR(self->m_Context);
	Py_CLEAR(self->m_Doc);
	Py_CLEAR(self->m_Annotations);
	Py_CLEAR(self->m_CodeRep);
	return 0;
	JP_TRACE_OUT_C;
}

PyObject *PyJPMethod::__get__(PyJPMethod *self, PyObject *obj, PyObject *type)
{
	JP_TRACE_IN_C("PyJPMethod::__get__", self);
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		if (obj == NULL)
		{
			Py_INCREF((PyObject*) self);
			JP_TRACE_PY("method get (inc)", (PyObject*) self);
			return (PyObject*) self;
		}
		return PyJPMethod::alloc(self->m_Method, obj).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject* PyJPMethod::__call__(PyJPMethod *self, PyObject *args, PyObject *kwargs)
{
	JP_TRACE_IN_C("PyJPMethod::__call__", self);
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		JP_TRACE(self->m_Method->getName());
		if (self->m_Instance == NULL)
		{
			JPPyObjectVector vargs(args);
			return self->m_Method->invoke(vargs, false).keep();
		}
		else
		{
			JPPyObjectVector vargs(self->m_Instance, args);
			return self->m_Method->invoke(vargs, true).keep();
		}
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject* PyJPMethod::__str__(PyJPMethod* self)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		stringstream sout;
		sout << self->m_Method->getClass()->getCanonicalName() << "." << self->m_Method->getName();
		return JPPyString::fromStringUTF8(sout.str()).keep();
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject* PyJPMethod::__repr__(PyJPMethod* self)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		stringstream ss;
		if (self->m_Instance == NULL)
			ss << "<java method `";
		else
			ss << "<java bound method `";
		ss << self->m_Method->getName() << "' of '" <<
				self->m_Method->getClass()->getCanonicalName() << "'>";
		return JPPyString::fromStringUTF8(ss.str()).keep();
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject *PyJPMethod::getSelf(PyJPMethod *self, void *context)
{
	JP_TRACE_IN("PyJPMethod::getSelf");
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		if (self->m_Instance == NULL)
			Py_RETURN_NONE;
		Py_INCREF(self->m_Instance);
		return self->m_Instance;
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT;
}


PyObject *PyJPMethod::getNone(PyJPMethod *self, void *context)
{
	Py_RETURN_NONE;
}

PyObject *PyJPMethod::getName(PyJPMethod *self, void *context)
{
	JP_TRACE_IN_C("PyJPMethod::getName", self);
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return JPPyString::fromStringUTF8(self->m_Method->getName(), false).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPMethod::getQualName(PyJPMethod *self, void *context)
{
	JP_TRACE_IN_C("PyJPMethod::getQualName", self);
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		stringstream str;
		str << self->m_Method->getClass()->getCanonicalName() << '.'
				<< self->m_Method->getName();
		return JPPyString::fromStringUTF8(str.str(), false).keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject *PyJPMethod::getDoc(PyJPMethod *self, void *context)
{
	JP_TRACE_IN_C("PyJPMethod::getDoc", self);
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		if (self->m_Doc)
		{
			Py_INCREF(self->m_Doc);
			return self->m_Doc;
		}
		JPPyObject out(JPPythonEnv::getMethodDoc(self));
		self->m_Doc = out.get();
		Py_XINCREF(self->m_Doc);
		return out.keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT;
}

int PyJPMethod::setDoc(PyJPMethod *self, PyObject* obj, void *context)
{
	JP_TRACE_IN("PyJPMethod::getDoc");
	Py_CLEAR(self->m_Doc);
	self->m_Doc = obj;
	Py_XINCREF(self->m_Doc);
	return 0;
	JP_TRACE_OUT;
}

PyObject *PyJPMethod::getAnnotations(PyJPMethod *self, void *context)
{
	JP_TRACE_IN_C("PyJPMethod::getAnnotations", self);
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		if (self->m_Annotations)
		{
			Py_INCREF(self->m_Annotations);
			return self->m_Annotations;
		}
		JPPyObject out(JPPythonEnv::getMethodAnnotations(self));
		self->m_Annotations = out.get();
		Py_XINCREF(self->m_Annotations);
		return out.keep();
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT;
}

int PyJPMethod::setAnnotations(PyJPMethod *self, PyObject* obj, void *context)
{
	JP_TRACE_IN_C("PyJPMethod::getAnnotations", self);
	Py_CLEAR(self->m_Annotations);
	self->m_Annotations = obj;
	Py_XINCREF(self->m_Annotations);
	return 0;
	JP_TRACE_OUT_C;
}

PyObject *PyJPMethod::getCodeAttr(PyJPMethod *self, void *context, const char* attr)
{
	JP_TRACE_IN_C("PyJPMethod::getCode", self);
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		if (self->m_CodeRep == NULL)
		{
			JPPyObject out(JPPythonEnv::getMethodCode(self));
			self->m_CodeRep = out.get();
			Py_XINCREF(self->m_CodeRep);
		}
		return PyObject_GetAttrString(self->m_CodeRep, attr);
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT;
}

PyObject *PyJPMethod::getCode(PyJPMethod *self, void *context)
{
#if PY_MAJOR_VERSION >= 3
	return getCodeAttr(self, context, "__code__");
#else
	return getCodeAttr(self, context, "func_code");
#endif
}

PyObject *PyJPMethod::getClosure(PyJPMethod *self, void *context)
{
#if PY_MAJOR_VERSION >= 3
	return getCodeAttr(self, context, "__closure__");
#else
	return getCodeAttr(self, context, "func_closure");
#endif
}

PyObject *PyJPMethod::getGlobals(PyJPMethod *self, void *context)
{
#if PY_MAJOR_VERSION >= 3
	return getCodeAttr(self, context, "__globals__");
#else
	return getCodeAttr(self, context, "func_globals");
#endif
}

PyObject* PyJPMethod::isBeanAccessor(PyJPMethod *self, PyObject *arg)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(self->m_Method->isBeanAccessor());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject* PyJPMethod::isBeanMutator(PyJPMethod *self, PyObject *arg)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		return PyBool_FromLong(self->m_Method->isBeanMutator());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject* PyJPMethod::matchReport(PyJPMethod *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		JPPyObjectVector vargs(args);
		string report = self->m_Method->matchReport(vargs);
		return JPPyString::fromStringUTF8(report).keep();
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject* PyJPMethod::dump(PyJPMethod *self, PyObject *args)
{
	try
	{
		JPContext *context = self->m_Context->m_Context;
		ASSERT_JVM_RUNNING(context);
		JPJavaFrame frame(context);
		string report = self->m_Method->dump();
		return JPPyString::fromStringUTF8(report).keep();
	}
	PY_STANDARD_CATCH(NULL);
}
