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
#include "jp_methoddispatch.h"
#include "jp_method.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct PyJPMethod
{
	PyFunctionObject func;
	JPMethodDispatch* m_Method;
	PyObject* m_Instance;
	PyObject* m_Doc;
	PyObject* m_Annotations;
	PyObject* m_CodeRep;
} ;

static int PyJPMethod_traverse(PyJPMethod *self, visitproc visit, void *arg)
{
	Py_VISIT(self->m_Instance);
	Py_VISIT(self->m_Doc);
	Py_VISIT(self->m_Annotations);
	Py_VISIT(self->m_CodeRep);
	return 0;
}

static int PyJPMethod_clear(PyJPMethod *self)
{
	Py_CLEAR(self->m_Instance);
	Py_CLEAR(self->m_Doc);
	Py_CLEAR(self->m_Annotations);
	Py_CLEAR(self->m_CodeRep);
	return 0;
}

static void PyJPMethod_dealloc(PyJPMethod *self)
{
	JP_PY_TRY("PyJPMethod_dealloc");
	PyObject_GC_UnTrack(self);
	Py_TRASHCAN_BEGIN(self, PyJPMethod_dealloc)
	PyJPMethod_clear(self);
	Py_TYPE(self)->tp_free(self);
	Py_TRASHCAN_END
	JP_PY_CATCH_NONE(); // GCOVR_EXCL_LINE
}

static PyObject *PyJPMethod_get(PyJPMethod *self, PyObject *obj, PyObject *type)
{
	JP_PY_TRY("PyJPMethod_get");
	PyJPModule_getContext();
	if (obj == nullptr)
	{
		Py_INCREF((PyObject*) self);
		JP_TRACE_PY("method get (inc)", (PyObject*) self);
		return (PyObject*) self;
	}
	PyJPMethod *out = (PyJPMethod*) PyJPMethod_create(self->m_Method, obj).keep();
	if (self->m_Doc != nullptr)
	{
		out->m_Doc = self->m_Doc;
		Py_INCREF(out->m_Doc);
	}
	if (self->m_Annotations != nullptr)
	{
		out->m_Annotations = self->m_Annotations;
		Py_INCREF(out->m_Annotations);
	}
	return (PyObject*) out;
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static PyObject *PyJPMethod_call(PyJPMethod *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPMethod_call");
	JPJavaFrame frame = JPJavaFrame::outer();
	JP_TRACE(self->m_Method->getName());
	// Clear any pending interrupts if we are on the main thread
	if (hasInterrupt())
		frame.clearInterrupt(false);
	PyObject *out = nullptr;
	if (self->m_Instance == nullptr)
	{
		JPPyObjectVector vargs(args);
		out = self->m_Method->invoke(frame, vargs, false).keep();
	} else
	{
		JPPyObjectVector vargs(self->m_Instance, args);
		out = self->m_Method->invoke(frame, vargs, true).keep();
	}
	return out;
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static PyObject *PyJPMethod_matches(PyJPMethod *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPMethod_matches");
	JPJavaFrame frame = JPJavaFrame::outer();
	JP_TRACE(self->m_Method->getName());
	if (self->m_Instance == nullptr)
	{
		JPPyObjectVector vargs(args);
		return PyBool_FromLong(self->m_Method->matches(frame, vargs, false));
	} else
	{
		JPPyObjectVector vargs(self->m_Instance, args);
		return PyBool_FromLong(self->m_Method->matches(frame, vargs, true));
	}
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static PyObject *PyJPMethod_str(PyJPMethod *self)
{
	JP_PY_TRY("PyJPMethod_str");
	JPJavaFrame frame = JPJavaFrame::outer();
	return PyUnicode_FromFormat("%s.%s",
			self->m_Method->getClass()->getCanonicalName().c_str(),
			self->m_Method->getName().c_str());
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static PyObject *PyJPMethod_repr(PyJPMethod *self)
{
	JP_PY_TRY("PyJPMethod_repr");
	PyJPModule_getContext();
	return PyUnicode_FromFormat("<java %smethod '%s' of '%s'>",
			(self->m_Instance != nullptr) ? "bound " : "",
			self->m_Method->getName().c_str(),
			self->m_Method->getClass()->getCanonicalName().c_str());
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static PyObject *PyJPMethod_getSelf(PyJPMethod *self, void *ctxt)
{
	JP_PY_TRY("PyJPMethod_getSelf");
	PyJPModule_getContext();
	if (self->m_Instance == nullptr)
		Py_RETURN_NONE;
	Py_INCREF(self->m_Instance);
	return self->m_Instance;
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static PyObject *PyJPMethod_getNone(PyJPMethod *self, void *ctxt)
{
	Py_RETURN_NONE;
}

static PyObject *PyJPMethod_getName(PyJPMethod *self, void *ctxt)
{
	JP_PY_TRY("PyJPMethod_getName");
	PyJPModule_getContext();
	return JPPyString::fromStringUTF8(self->m_Method->getName()).keep();
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static PyObject *PyJPMethod_getQualName(PyJPMethod *self, void *ctxt)
{
	JP_PY_TRY("PyJPMethod_getQualName");
	PyJPModule_getContext();
	return PyUnicode_FromFormat("%s.%s",
			self->m_Method->getClass()->getCanonicalName().c_str(),
			self->m_Method->getName().c_str());
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

static PyObject *PyJPMethod_getDoc(PyJPMethod *self, void *ctxt)
{
	JP_PY_TRY("PyJPMethod_getDoc");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer();
	if (self->m_Doc)
	{
		Py_INCREF(self->m_Doc);
		return self->m_Doc;
	}

	// Convert the overloads
	JP_TRACE("Convert overloads");
	const JPMethodList& overloads = self->m_Method->getMethodOverloads();
	JPPyObject ov = JPPyObject::call(PyTuple_New(overloads.size()));
	int i = 0;
	JPClass* methodClass = frame.findClassByName("java.lang.reflect.Method");
	for (auto iter = overloads.begin(); iter != overloads.end(); ++iter)
	{
		JP_TRACE("Set overload", i);
		jvalue v;
		v.l = (*iter)->getJava();
		JPPyObject obj(methodClass->convertToPythonObject(frame, v, true));
		PyTuple_SetItem(ov.get(), i++, obj.keep());
	}

	// Pack the arguments
	{
		JP_TRACE("Pack arguments");
		jvalue v;
		v.l = (jobject) self->m_Method->getClass()->getJavaClass();
		JPPyObject obj(context->_java_lang_Class->convertToPythonObject(frame, v, true));
		JPPyObject args = JPPyTuple_Pack(self, obj.get(), ov.get());
		JP_TRACE("Call Python");
		self->m_Doc = PyObject_Call(_JMethodDoc, args.get(), nullptr);
		Py_XINCREF(self->m_Doc);
		return self->m_Doc;
	}
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

int PyJPMethod_setDoc(PyJPMethod *self, PyObject *obj, void *ctxt)
{
	JP_PY_TRY("PyJPMethod_setDoc");
	Py_CLEAR(self->m_Doc);
	self->m_Doc = obj;
	Py_XINCREF(self->m_Doc);
	return 0;
	JP_PY_CATCH(-1); // GCOVR_EXCL_LINE
}

PyObject *PyJPMethod_getAnnotations(PyJPMethod *self, void *ctxt)
{
	JP_PY_TRY("PyJPMethod_getAnnotations");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer();
	if (self->m_Annotations)
	{
		Py_INCREF(self->m_Annotations);
		return self->m_Annotations;
	}

	// Convert the overloads
	JP_TRACE("Convert overloads");
	const JPMethodList& overloads = self->m_Method->getMethodOverloads();
	JPPyObject ov = JPPyObject::call(PyTuple_New(overloads.size()));
	int i = 0;
	JPClass* methodClass = frame.findClassByName("java.lang.reflect.Method");
	for (auto iter = overloads.begin(); iter != overloads.end(); ++iter)
	{
		JP_TRACE("Set overload", i);
		jvalue v;
		v.l = (*iter)->getJava();
		JPPyObject obj(methodClass->convertToPythonObject(frame, v, true));
		PyTuple_SetItem(ov.get(), i++, obj.keep());
	}

	// Pack the arguments
	{
		JP_TRACE("Pack arguments");
		jvalue v;
		v.l = (jobject) self->m_Method->getClass()->getJavaClass();
		JPPyObject obj(context->_java_lang_Class->convertToPythonObject(frame, v, true));
		JPPyObject args = JPPyTuple_Pack(self, obj.get(), ov.get());
		JP_TRACE("Call Python");
		self->m_Annotations = PyObject_Call(_JMethodAnnotations, args.get(), nullptr);
	}

	Py_XINCREF(self->m_Annotations);
	return self->m_Annotations;
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

int PyJPMethod_setAnnotations(PyJPMethod *self, PyObject* obj, void *ctx)
{
	Py_CLEAR(self->m_Annotations);
	self->m_Annotations = obj;
	Py_XINCREF(self->m_Annotations);
	return 0;
}

PyObject *PyJPMethod_getCodeAttr(PyJPMethod *self, void *ctx, const char *attr)
{
	JP_PY_TRY("PyJPMethod_getCodeAttr");
	PyJPModule_getContext();
	if (self->m_CodeRep == nullptr)
	{
		JPPyObject args = JPPyTuple_Pack(self);
		JP_TRACE("Call Python");
		self->m_CodeRep = PyObject_Call(_JMethodCode, args.get(), nullptr);
	}
	return PyObject_GetAttrString(self->m_CodeRep, attr);
	JP_PY_CATCH(nullptr);
}

PyObject *PyJPMethod_getCode(PyJPMethod *self, void *ctxt)
{
	return PyJPMethod_getCodeAttr(self, ctxt, "__code__");
}

PyObject *PyJPMethod_getClosure(PyJPMethod *self, void *ctxt)
{
	return PyJPMethod_getCodeAttr(self, ctxt, "__closure__");
}

PyObject *PyJPMethod_getGlobals(PyJPMethod *self, void *ctxt)
{
	return PyJPMethod_getCodeAttr(self, ctxt, "__globals__");
}

PyObject *PyJPMethod_isBeanAccessor(PyJPMethod *self, PyObject *arg)
{
	JP_PY_TRY("PyJPMethod_isBeanAccessor");
	PyJPModule_getContext();
	return PyBool_FromLong(self->m_Method->isBeanAccessor());
	JP_PY_CATCH(nullptr);
}

PyObject *PyJPMethod_isBeanMutator(PyJPMethod *self, PyObject *arg)
{
	JP_PY_TRY("PyJPMethod_isBeanMutator");
	PyJPModule_getContext();
	return PyBool_FromLong(self->m_Method->isBeanMutator());
	JP_PY_CATCH(nullptr);
}

PyObject *PyJPMethod_matchReport(PyJPMethod *self, PyObject *args)
{
	JP_PY_TRY("PyJPMethod_matchReport");
	PyJPModule_getContext();
	JPPyObjectVector vargs(args);
	string report = self->m_Method->matchReport(vargs);
	return JPPyString::fromStringUTF8(report).keep();
	JP_PY_CATCH(nullptr);
}

static PyMethodDef methodMethods[] = {
	{"_isBeanAccessor", (PyCFunction) (&PyJPMethod_isBeanAccessor), METH_NOARGS, ""},
	{"_isBeanMutator", (PyCFunction) (&PyJPMethod_isBeanMutator), METH_NOARGS, ""},
	{"matchReport", (PyCFunction) (&PyJPMethod_matchReport), METH_VARARGS, ""},
	// This is  currently private but may be promoted
	{"_matches", (PyCFunction) (&PyJPMethod_matches), METH_VARARGS, ""},
	{nullptr},
};

struct PyGetSetDef methodGetSet[] = {
	{"__self__", (getter) (&PyJPMethod_getSelf), nullptr, nullptr, nullptr},
	{"__name__", (getter) (&PyJPMethod_getName), nullptr, nullptr, nullptr},
	{"__doc__", (getter) (&PyJPMethod_getDoc), (setter) (&PyJPMethod_setDoc), nullptr, nullptr},
	{"__annotations__", (getter) (&PyJPMethod_getAnnotations), (setter) (&PyJPMethod_setAnnotations), nullptr, nullptr},
	{"__closure__", (getter) (&PyJPMethod_getClosure), nullptr, nullptr, nullptr},
	{"__code__", (getter) (&PyJPMethod_getCode), nullptr, nullptr, nullptr},
	{"__defaults__", (getter) (&PyJPMethod_getNone), nullptr, nullptr, nullptr},
	{"__kwdefaults__", (getter) (&PyJPMethod_getNone), nullptr, nullptr, nullptr},
	{"__globals__", (getter) (&PyJPMethod_getGlobals), nullptr, nullptr, nullptr},
	{"__qualname__", (getter) (&PyJPMethod_getQualName), nullptr, nullptr, nullptr},
	{nullptr},
};

static PyType_Slot methodSlots[] = {
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

PyTypeObject *PyJPMethod_Type = nullptr;
static PyType_Spec methodSpec = {
	"_jpype._JMethod",
	sizeof (PyJPMethod),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	methodSlots
};

#ifdef __cplusplus
}
#endif

void PyJPMethod_initType(PyObject* module)
{
	// We inherit from PyFunction_Type just so we are an instance
	// for purposes of inspect and tab completion tools.  But
	// we will just ignore their memory layout as we have our own.
	JPPyObject tuple = JPPyTuple_Pack(&PyFunction_Type);
	unsigned long flags = PyFunction_Type.tp_flags;
	PyFunction_Type.tp_flags |= Py_TPFLAGS_BASETYPE;
	PyJPMethod_Type = (PyTypeObject*) PyType_FromSpecWithBases(&methodSpec, tuple.get());
	PyFunction_Type.tp_flags = flags;
	JP_PY_CHECK();

	PyModule_AddObject(module, "_JMethod", (PyObject*) PyJPMethod_Type);
	JP_PY_CHECK();
}

JPPyObject PyJPMethod_create(JPMethodDispatch *m, PyObject *instance)
{
	JP_TRACE_IN("PyJPMethod_create");
	auto* self = (PyJPMethod*) PyJPMethod_Type->tp_alloc(PyJPMethod_Type, 0);
	JP_PY_CHECK();
	self->m_Method = m;
	self->m_Instance = instance;
	self->m_Doc = nullptr;
	self->m_Annotations = nullptr;
	self->m_CodeRep = nullptr;
	Py_XINCREF(self->m_Instance);
	return JPPyObject::claim((PyObject*) self);
	JP_TRACE_OUT; /// GCOVR_EXCL_LINE
}
