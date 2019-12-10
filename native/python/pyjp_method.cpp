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

#ifdef __cplusplus
extern "C"
{
#endif

PyObject *PyJPMethod_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	PyJPMethod *self = (PyJPMethod*) type->tp_alloc(type, 0);
	self->m_Method = NULL;
	self->m_Instance = NULL;
	self->m_Doc = NULL;
	self->m_Annotations = NULL;
	self->m_CodeRep = NULL;
	return (PyObject*) self;
}

int PyJPMethod_clear(PyJPMethod *self);

void PyJPMethod_dealloc(PyJPMethod *self)
{
	JP_PY_TRY("PyJPMethod_dealloc", self);
	PyObject_GC_UnTrack(self);
	JP_TRACE(self->m_Instance);
	JP_TRACE((self->m_Instance != NULL ? self->m_Instance->ob_refcnt : -1));
	PyJPMethod_clear(self);
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH();
}

int PyJPMethod_traverse(PyJPMethod *self, visitproc visit, void *arg)
{
	JP_PY_TRY("PyJPMethod_traverse", self);
	Py_VISIT(self->m_Instance);
	Py_VISIT(self->m_Doc);
	Py_VISIT(self->m_Annotations);
	Py_VISIT(self->m_CodeRep);
	return 0;
	JP_PY_CATCH(-1);
}

int PyJPMethod_clear(PyJPMethod *self)
{
	JP_PY_TRY("PyJPMethod_clear", self);
	Py_CLEAR(self->m_Instance);
	Py_CLEAR(self->m_Doc);
	Py_CLEAR(self->m_Annotations);
	Py_CLEAR(self->m_CodeRep);
	return 0;
	JP_PY_CATCH(-1);
}

PyObject *PyJPMethod_get(PyJPMethod *self, PyObject *obj, PyObject *type)
{
	JP_PY_TRY("PyJPMethod_get", self);
	PyJPModule_getContext();
	if (obj == NULL)
	{
		Py_INCREF((PyObject*) self);
		JP_TRACE_PY("method get (inc)", (PyObject*) self);
		return (PyObject*) self;
	}
	return PyJPMethod_create(self->m_Method, obj).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMethod_call(PyJPMethod *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPMethod_call", self);
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	JP_TRACE(self->m_Method->getName());
	JP_TRACE(self->m_Instance);
	if (self->m_Instance == NULL)
	{
		JPPyObjectVector vargs(args);
		return self->m_Method->invoke(vargs, false).keep();
	} else
	{
		JPPyObjectVector vargs(self->m_Instance, args);
		return self->m_Method->invoke(vargs, true).keep();
	}
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMethod_str(PyJPMethod *self)
{
	JP_PY_TRY("PyJPMethod_str", self);
	PyJPModule_getContext();
	stringstream sout;
	sout << self->m_Method->getClass()->getCanonicalName() << "." << self->m_Method->getName();
	return JPPyString::fromStringUTF8(sout.str()).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMethod_repr(PyJPMethod *self)
{
	JP_PY_TRY("PyJPMethod_repr", self);
	PyJPModule_getContext();
	stringstream ss;
	if (self->m_Instance == NULL)
		ss << "<java method `";
	else
		ss << "<java bound method `";
	ss << self->m_Method->getName() << "' of '" <<
			self->m_Method->getClass()->getCanonicalName() << "'>";
	return JPPyString::fromStringUTF8(ss.str()).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMethod_getSelf(PyJPMethod *self, void *context)
{
	JP_TRACE_IN("PyJPMethod_getSelf", self);
	PyJPModule_getContext();
	if (self->m_Instance == NULL)
		Py_RETURN_NONE;
	Py_INCREF(self->m_Instance);
	return self->m_Instance;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMethod_getNone(PyJPMethod *self, void *context)
{
	Py_RETURN_NONE;
}

PyObject *PyJPMethod_getName(PyJPMethod *self, void *context)
{
	JP_PY_TRY("PyJPMethod_getName", self);
	PyJPModule_getContext();
	JP_TRACE(self->m_Method->getName());
	return JPPyString::fromStringUTF8(self->m_Method->getName(), false).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMethod_getQualName(PyJPMethod *self, void *context)
{
	JP_PY_TRY("PyJPMethod_getQualName", self);
	PyJPModule_getContext();
	stringstream str;
	str << self->m_Method->getClass()->getCanonicalName() << '.'
			<< self->m_Method->getName();
	return JPPyString::fromStringUTF8(str.str(), false).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMethod_getDoc(PyJPMethod *self, void *context)
{
	JP_PY_TRY("PyJPMethod_getDoc", self);
	JPContext *context = PyJPModule_getContext();
	if (self->m_Doc)
	{
		Py_INCREF(self->m_Doc);
		return self->m_Doc;
	}

	// Get the resource
	JPPyObject getMethodDoc(JPPyRef::_claim,
			PyObject_GetAttrString(PyJPModule_global, "_jmethodGetDoc"));

	// Convert the overloads
	JP_TRACE("Convert overloads");
	const JPMethodList& overloads = self->m_Method->getMethodOverloads();
	JPPyTuple ov(JPPyTuple::newTuple(overloads.size()));
	int i = 0;
	JPClass *methodClass = context->_java_lang_reflect_Method;
	for (JPMethodList::const_iterator iter = overloads.begin(); iter != overloads.end(); ++iter)
	{
		JP_TRACE("Set overload", i);
		jvalue v;
		v.l = (*iter)->getJava();
		JPPyObject obj(JPPythonEnv::newJavaObject(JPValue(methodClass, v)));
		ov.setItem(i++, obj.get());
	}

	// Pack the arguments

	JP_TRACE("Pack arguments");
	jvalue v;
	v.l = (jobject) self->m_Method->getClass()->getJavaClass();
	JPPyObject obj(JPPythonEnv::newJavaObject(JPValue(context->_java_lang_Class, v)));

	JPPyTuple args(JPPyTuple::newTuple(3));
	args.setItem(0, (PyObject*) self);
	args.setItem(1, obj.get());
	args.setItem(2, ov.get());
	JP_TRACE("Call Python");
	self->m_Doc = getMethodDoc.call(args.get(), NULL).keep();

	Py_XINCREF(self->m_Doc);
	return self->m_Doc;
	JP_PY_CATCH(NULL);
}

int PyJPMethod_setDoc(PyJPMethod *self, PyObject *obj, void *context)
{
	JP_PY_TRY("PyJPMethod_getDoc");
	Py_CLEAR(self->m_Doc);
	self->m_Doc = obj;
	Py_XINCREF(self->m_Doc);
	return 0;
	JP_PY_CATCH(-1);
}

PyObject *PyJPMethod_getAnnotations(PyJPMethod *self, void *context)
{
	JP_PY_TRY("PyJPMethod_getAnnotations", self);
	JPContext *context = PyJPModule_getContext();
	if (self->m_Annotations)
	{
		Py_INCREF(self->m_Annotations);
		return self->m_Annotations;
	}

	// Get the resource
	JPPyObject getAnnotations(JPPyRef::_claim,
			PyObject_GetAttrString(PyJPModule_global, "_jmethodGetAnnotations"));

	// Convert the overloads
	JP_TRACE("Convert overloads");
	const JPMethodList& overloads = self->m_Method->getMethodOverloads();
	JPPyTuple ov(JPPyTuple::newTuple(overloads.size()));
	int i = 0;
	JPClass *methodClass = context->_java_lang_reflect_Method;
	for (JPMethodList::const_iterator iter = overloads.begin(); iter != overloads.end(); ++iter)
	{
		JP_TRACE("Set overload", i);
		jvalue v;
		v.l = (*iter)->getJava();
		JPPyObject obj(JPPythonEnv::newJavaObject(JPValue(methodClass, v)));
		ov.setItem(i++, obj.get());
	}

	// Pack the arguments

	JP_TRACE("Pack arguments");
	jvalue v;
	v.l = (jobject) self->m_Method->getClass()->getJavaClass();
	JPPyObject obj(JPPythonEnv::newJavaObject(JPValue(context->_java_lang_Class, v)));

	JPPyTuple args(JPPyTuple::newTuple(3));
	args.setItem(0, (PyObject*) self);
	args.setItem(1, obj.get());
	args.setItem(2, ov.get());
	JP_TRACE("Call Python");
	self->m_Annotations = getAnnotations.call(args.get(), NULL).keep();

	Py_XINCREF(self->m_Annotations);
	return self->m_Annotations;
	JP_PY_CATCH(NULL);
}

int PyJPMethod_setAnnotations(PyJPMethod *self, PyObject *obj, void *context)
{
	Py_CLEAR(self->m_Annotations);
	self->m_Annotations = obj;
	Py_XINCREF(self->m_Annotations);
	return 0;
}

PyObject *PyJPMethod_getCodeAttr(PyJPMethod *self, void *context, const char *attr)
{
	JP_PY_TRY("PyJPMethod_getCodeAttr", self);
	PyJPModule_getContext();
	if (self->m_CodeRep == NULL)
	{
		JPPyObject getCode(JPPyRef::_claim,
				PyObject_GetAttrString(PyJPModule_global, "_jmethodGetCode"));

		// Pack the arguments
		JP_TRACE("Pack arguments");
		JPPyTuple args(JPPyTuple::newTuple(1));
		args.setItem(0, (PyObject*) self);
		JP_TRACE("Call Python");
		self->m_CodeRep = getCode.call(args.get(), NULL).keep();

		Py_XINCREF(self->m_CodeRep);
	}
	return PyObject_GetAttrString(self->m_CodeRep, attr);
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMethod_getCode(PyJPMethod *self, void *context)
{
	return PyJPMethod_getCodeAttr(self, context, "__code__");
}

PyObject *PyJPMethod_getClosure(PyJPMethod *self, void *context)
{
	return PyJPMethod_getCodeAttr(self, context, "__closure__");
}

PyObject *PyJPMethod_getGlobals(PyJPMethod *self, void *context)
{
	return PyJPMethod_getCodeAttr(self, context, "__globals__");
}

PyObject *PyJPMethod_isBeanAccessor(PyJPMethod *self, PyObject *arg)
{
	JP_PY_TRY("PyJPMethod_isBeanAccessor", self);
	JPContext *context = PyJPModule_getContext();
	ASSERT_JVM_RUNNING(context);
	return PyBool_FromLong(self->m_Method->isBeanAccessor());
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMethod_isBeanMutator(PyJPMethod *self, PyObject *arg)
{
	JP_PY_TRY("PyJPMethod_isBeanMutator", self);
	JPContext *context = PyJPModule_getContext();
	ASSERT_JVM_RUNNING(context);
	return PyBool_FromLong(self->m_Method->isBeanMutator());
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMethod_matchReport(PyJPMethod *self, PyObject *args)
{
	JP_PY_TRY("PyJPMethod_matchReport", self);
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	JPPyObjectVector vargs(args);
	string report = self->m_Method->matchReport(vargs);
	return JPPyString::fromStringUTF8(report).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPMethod_dump(PyJPMethod *self, PyObject *args)
{
	JP_PY_TRY("PyJPMethod_dump", self);
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	string report = self->m_Method->dump();
	return JPPyString::fromStringUTF8(report).keep();
	JP_PY_CATCH(NULL);
}
static PyMethodDef methodMethods[] = {
	{"_isBeanAccessor", (PyCFunction) (&PyJPMethod_isBeanAccessor), METH_NOARGS, ""},
	{"_isBeanMutator", (PyCFunction) (&PyJPMethod_isBeanMutator), METH_NOARGS, ""},
	{"_matchReport", (PyCFunction) (&PyJPMethod_matchReport), METH_VARARGS, ""},
	{"_dump", (PyCFunction) (&PyJPMethod_dump), METH_NOARGS, ""},
	{NULL},
};

struct PyGetSetDef methodGetSet[] = {
	{"__self__", (getter) (&PyJPMethod_getSelf), NULL, NULL, NULL},
	{"__name__", (getter) (&PyJPMethod_getName), NULL, NULL, NULL},
	{"__doc__", (getter) (&PyJPMethod_getDoc), (setter) (&PyJPMethod_setDoc), NULL, NULL},
	{"__annotations__", (getter) (&PyJPMethod_getAnnotations), (setter) (&PyJPMethod_setAnnotations), NULL, NULL},
	{"__closure__", (getter) (&PyJPMethod_getClosure), NULL, NULL, NULL},
	{"__code__", (getter) (&PyJPMethod_getCode), NULL, NULL, NULL},
	{"__defaults__", (getter) (&PyJPMethod_getNone), NULL, NULL, NULL},
	{"__kwdefaults__", (getter) (&PyJPMethod_getNone), NULL, NULL, NULL},
	{"__globals__", (getter) (&PyJPMethod_getGlobals), NULL, NULL, NULL},
	{"__qualname__", (getter) (&PyJPMethod_getQualName), NULL, NULL, NULL},
	{NULL},
};

static PyType_Slot methodSlots[] = {
	{Py_tp_new,       (void*) PyJPMethod_new},
	{Py_tp_dealloc,   (void*) PyJPMethod_dealloc},
	{Py_tp_traverse,  (void*) PyJPMethod_traverse},
	{Py_tp_clear,     (void*) PyJPMethod_clear},
	{Py_tp_repr,      (void*) PyJPMethod_repr},
	{Py_tp_call,      (void*) PyJPMethod_call},
	{Py_tp_str,       (void*) PyJPMethod_str},
	{Py_tp_descr_get, (void*) PyJPMethod_get},
	{Py_tp_methods,   (void*) methodMethods},
	{Py_tp_getset,    (void*) methodGetSet},
	{0}
};

PyType_Spec PyJPMethodSpec = {
	"_jpype.PyJPMethod",
	sizeof (PyJPMethod),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	methodSlots
};

#ifdef __cplusplus
}
#endif
