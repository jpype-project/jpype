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
#include <stddef.h>
#include <pyjp.h>
#include <structmember.h>

#include "jp_context.h"

#ifdef __cplusplus
extern "C"
{
#endif

PyObject *PyJPContext_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int PyJPContext_init(PyJPContext *self, PyObject *args, PyObject *kwargs);
void PyJPContext_dealloc(PyJPContext *self);
int PyJPContext_traverse(PyJPContext *self, visitproc visit, void *arg);
int PyJPContext_clear(PyJPContext *self);
PyObject *PyJPContext_str(PyJPContext *self);
PyObject *PyJPContext_startup(PyJPContext *self, PyObject *args);
PyObject *PyJPContext_shutdown(PyJPContext *self, PyObject *args);
PyObject *PyJPContext_isStarted(PyJPContext *self, PyObject *args);
PyObject *PyJPContext_attachThread(PyJPContext *self, PyObject *args);
PyObject *PyJPContext_attachThreadAsDaemon(PyJPContext *self, PyObject *args);
PyObject *PyJPContext_detachThread(PyJPContext *self, PyObject *args);
PyObject *PyJPContext_isThreadAttached(PyJPContext *self, PyObject *args);
PyObject *PyJPContext_convertToDirectByteBuffer(PyJPContext *self, PyObject *args);

const char *check_doc =
		"Checks if a thread is attached to the JVM.\n"
		"\n"
		"Python automatically attaches threads when a Java method is called.\n"
		"This creates a resource in Java for the Python thread. This method\n"
		"can be used to check if a Python thread is currently attached so that\n"
		"it can be disconnected prior to thread termination to prevent leaks.\n"
		"\n"
		"Returns:\n"
		"  True if the thread is attached to the JVM, False if the thread is\n"
		"  not attached or the JVM is not running.\n";

const char *shutdown_doc =
		"Shuts down the JVM.\n"
		"\n"
		"This method shuts down the JVM and thus disables access to existing\n"
		"Java objects. Due to limitations in the JPype, it is not possible to\n"
		"restart the JVM after being terminated.\n";

const char *attach_doc =
		"Attaches a thread to the JVM.\n"
		"\n"
		"The function manually connects a thread to the JVM to allow access to\n"
		"Java objects and methods. JPype automatically attaches when a Java\n"
		"resource is used, so a call to this is usually not needed.\n"
		"\n"
		"Raises:\n"
		"  RuntimeError: If the JVM is not running.\n";

const char *detach_doc =
		"Detaches a thread from the JVM.\n"
		"\n"
		"This function detaches the thread and frees the associated resource in\n"
		"the JVM. For codes making heavy use of threading this should be used\n"
		"to prevent resource leaks. The thread can be reattached, so there\n"
		"is no harm in detaching early or more than once. This method cannot fail\n"
		"and there is no harm in calling it when the JVM is not running.\n";

static PyMethodDef contextMethods[] = {
	// JVM control
	{"isStarted", (PyCFunction) (&PyJPContext_isStarted), METH_NOARGS, ""},
	{"_startup", (PyCFunction) (&PyJPContext_startup), METH_VARARGS, ""},
	{"shutdown", (PyCFunction) (&PyJPContext_shutdown), METH_NOARGS, shutdown_doc},

	// Threading
	{"isThreadAttached", (PyCFunction) (&PyJPContext_isThreadAttached), METH_NOARGS, check_doc},
	{"attachThread", (PyCFunction) (&PyJPContext_attachThread), METH_NOARGS, attach_doc},
	{"detachThread", (PyCFunction) (&PyJPContext_detachThread), METH_NOARGS, detach_doc},
	{"attachThreadAsDaemon", (PyCFunction) (&PyJPContext_attachThreadAsDaemon), METH_NOARGS, ""},

	// ByteBuffer
	{"_convertToDirectBuffer", (PyCFunction) (&PyJPContext_convertToDirectByteBuffer), METH_VARARGS, ""},
	{"_getClass", (PyCFunction) (&PyJPContext_getClass), METH_VARARGS, ""},

	{NULL},
};

static PyMemberDef contextMembers[] = {
	{"_classes", T_OBJECT, offsetof(PyJPContext, m_Classes), READONLY},
	{"__dictoffset__", T_PYSSIZET, offsetof(PyJPContext, m_Dict), READONLY},
	{0}
};

static PyType_Slot contextSlots[] = {
	{ Py_tp_new,        (void*) PyJPContext_new},
	{ Py_tp_init,       (void*) PyJPContext_init},
	{ Py_tp_dealloc,    (void*) PyJPContext_dealloc},
	//	{ Py_tp_traverse,   (void*) PyJPContext_traverse},
	//	{ Py_tp_clear,      (void*) PyJPContext_clear},
	{ Py_tp_str,        (void*) PyJPContext_str},
	{ Py_tp_getattro,   (void*) PyObject_GenericGetAttr},
	{ Py_tp_setattro,   (void*) PyObject_GenericSetAttr},
	{ Py_tp_doc,        (void*) "Java Virtual Machine Context"},
	{ Py_tp_methods,    (void*) contextMethods},
	{ Py_tp_members,    (void*) contextMembers},
	{0}
};

PyType_Spec PyJPContextSpec = {
	"_jpype.PyJPContext",
	sizeof (PyJPContext),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // | Py_TPFLAGS_HAVE_GC
	contextSlots
};

// Static methods

PyObject *PyJPContext_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	PyJPContext *self = (PyJPContext*) type->tp_alloc(type, 0);
	self->m_Context = NULL;
	self->m_Classes = PyDict_New();
	self->m_Context = new JPContext();
	self->m_Context->setHost((PyObject*) self);
	return (PyObject*) self;
}

int PyJPContext_init(PyJPContext *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPContext_init", self)
	return 0;
	JP_PY_CATCH(-1);
}

void PyJPContext_dealloc(PyJPContext *self)
{
	JP_PY_TRY("PyJPContext_dealloc", self)
	if (self->m_Context->isRunning())
		self->m_Context->shutdownJVM();
	delete self->m_Context;
	self->m_Context = NULL;
	PyObject_GC_UnTrack(self);
	PyJPContext_clear(self);
	// Free self
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH();
}

int PyJPContext_traverse(PyJPContext *self, visitproc visit, void *arg)
{
	Py_VISIT(self->m_Classes);
	return 0;
}

int PyJPContext_clear(PyJPContext *self)
{
	Py_CLEAR(self->m_Classes);
	return 0;
}

PyObject *PyJPContext_str(PyJPContext *self)
{
	JP_PY_TRY("PyJPContext_str", self)
	JPContext *context = self->m_Context;
	stringstream sout;
	sout << "<java context " << context << ">";
	return JPPyString::fromStringUTF8(sout.str()).keep();
	JP_PY_CATCH(NULL);
}

PyObject *PyJPContext_startup(PyJPContext *self, PyObject *args)
{
	JP_PY_TRY("PyJPContext_startup", self)
	if (self->m_Context->isRunning())
	{
		PyErr_SetString(PyExc_OSError, "JVM is already started");
		return NULL;
	}
	if (self->m_Context->isShutdown())
	{
		PyErr_SetString(PyExc_OSError, "JVM cannot be restarted");
		return NULL;
	}
	PyObject *vmOpt;
	PyObject *vmPath;
	bool ignoreUnrecognized = true;
	bool convertStrings = true;

	if (!PyArg_ParseTuple(args, "OO!bb", &vmPath, &PyTuple_Type, &vmOpt,
			&ignoreUnrecognized, &convertStrings))
	{
		return NULL;
	}

	if (!(JPPyString::check(vmPath)))
	{
		JP_RAISE_RUNTIME_ERROR("Java JVM path must be a string");
	}

	string cVmPath = JPPyString::asStringUTF8(vmPath);
	JP_TRACE("vmpath", cVmPath);

	StringVector args;
	JPPySequence seq(JPPyRef::_use, vmOpt);

	for (int i = 0; i < seq.size(); i++)
	{
		JPPyObject obj(seq[i]);

		if (JPPyString::check(obj.get()))
		{
			// TODO support unicode
			string v = JPPyString::asStringUTF8(obj.get());
			JP_TRACE("arg", v);
			args.push_back(v);
		} else
		{
			JP_RAISE_RUNTIME_ERROR("VM Arguments must be strings");
		}
	}

	self->m_Context->startJVM(cVmPath, args, ignoreUnrecognized, convertStrings);
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPContext_shutdown(PyJPContext *self, PyObject *args)
{
	JP_PY_TRY("PyJPContext_shutdown", self)
	// Stop the JVM
	self->m_Context->shutdownJVM();

	// Disconnect the classes
	PyDict_Clear(self->m_Classes);
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPContext_isStarted(PyJPContext *self, PyObject *args)
{
	return PyBool_FromLong(self->m_Context->isRunning());
}

PyObject *PyJPContext_attachThread(PyJPContext *self, PyObject *args)
{
	JP_PY_TRY("PyJPContext_attachThread", self)
	JPContext *context = self->m_Context;
	ASSERT_JVM_RUNNING(context);
	context->attachCurrentThread();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPContext_attachThreadAsDaemon(PyJPContext *self, PyObject *args)
{
	JP_PY_TRY("PyJPContext_attachThreadAsDaemon", self)
	JPContext *context = self->m_Context;
	ASSERT_JVM_RUNNING(context);
	context->attachCurrentThreadAsDaemon();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPContext_detachThread(PyJPContext *self, PyObject *args)
{
	JP_PY_TRY("PyJPContext_detachThread", self)
	if (self->m_Context->isRunning())
		self->m_Context->detachCurrentThread();
	Py_RETURN_NONE;
	JP_PY_CATCH(NULL);
}

PyObject *PyJPContext_isThreadAttached(PyJPContext *self, PyObject *args)
{
	JP_PY_TRY("PyJPContext_isThreadAttached", self)
	if (!self->m_Context->isRunning())
		return PyBool_FromLong(0);
	return PyBool_FromLong(self->m_Context->isThreadAttached());
	JP_PY_CATCH(NULL);
}

PyObject *PyJPContext_convertToDirectByteBuffer(PyJPContext *self, PyObject *args)
{
	JP_PY_TRY("PyJPContext_convertToDirectByteBuffer", self)
	JPContext *context = self->m_Context;
	ASSERT_JVM_RUNNING(context);
	JPJavaFrame frame(context);

	// Use special method defined on the TypeConverter interface ...
	PyObject *src;

	PyArg_ParseTuple(args, "O", &src);
	JP_PY_CHECK();

	PyObject *res = NULL;
	if (JPPyMemoryView::check(src))
	{
		JP_TRACE("Converting");
		jobject ref = self->m_Context->_byte->convertToDirectBuffer(frame, src);

		// Bind lifespan of the python to the java object.
		context->getReferenceQueue()->registerRef(ref, src);

		// Convert to python object

		jvalue v;
		v.l = ref;
		JPClass *type = context->getTypeManager()->findClassForObject(ref);
		res = type->convertToPythonObject(frame, v).keep();
	}

	if (res != NULL)
	{
		return res;
	}

	JP_RAISE_RUNTIME_ERROR("Do not know how to convert to direct byte buffer, only memory view supported");
	JP_PY_CATCH(NULL);
}

// Call from Python

PyObject *PyJPContext_getClass(PyJPContext *self, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPContext_getClass", self)
	PyObject *module = PyJPModule_global;
	PyJPModuleState *state = PyJPModuleState(module);

	PyJPClass *cls = NULL;
	if (!PyArg_ParseTuple(args, "O!", state->PyJPClass_Type, &cls))
		return NULL;

	JPClass *javaClass = cls->m_Class;
	if (javaClass->getHost() != NULL)
	{
		PyObject* out = javaClass->getHost();
		Py_INCREF(out);
		return out;
	}

	// Get the type factory
	JPPyObject factory(JPPyRef::_accept,
			PyObject_GetAttrString(module, "_JClassFactory"));

	//	PyObject* factory = PyDict_GetItemString(Py_TYPE(self)->tp_dict, "_JClassFactory");
	if (factory.isNull())
		JP_RAISE_RUNTIME_ERROR("Factory not set");

	// Call the factory
	JPPyObject out = factory.call(args, kwargs);

	// Store caches
	javaClass ->setHost(out.get());
	return out.keep();
	JP_PY_CATCH(NULL);
}

#ifdef __cplusplus
}
#endif

JPPyObject JPPythonEnv::newJavaClass(JPClass *javaClass)
{
	JP_TRACE_IN("JPPythonEnv::newJavaClass")
	PyJPModuleState *state = PyJPModuleState_global;

	ASSERT_NOT_NULL(javaClass);

	// Check the cache
	if (javaClass->getHost() != NULL)
	{
		return JPPyObject(JPPyRef::_use, javaClass->getHost());
	}

	PyJPContext *context = (PyJPContext*) (javaClass->getContext()->getHost());

	// Get the type factory
	JPPyObject factory(JPPyRef::_claim,
			PyObject_GetAttrString(PyJPModule_global, "_JClassFactory"));

	// Pack the args
	JPPyTuple args(JPPyTuple::newTuple(1));
	args.setItem(0, PyJPClass_create((PyTypeObject*) state->PyJPClass_Type,
			context->m_Context, javaClass).get());

	// Call the factory in Python
	JPPyObject out = JPPyObject(JPPyRef::_call, PyObject_Call(factory.get(), args.get(), NULL));
	javaClass->setHost(out.get());
	return out;
	JP_TRACE_OUT;
}
