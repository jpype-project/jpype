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
#include <pyjp_context.h>
#include <structmember.h>

#include "jp_context.h"

const char* check_doc =
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

const char* shutdown_doc =
		"Shuts down the JVM.\n"
		"\n"
		"This method shuts down the JVM and thus disables access to existing\n"
		"Java objects. Due to limitations in the JPype, it is not possible to\n"
		"restart the JVM after being terminated.\n";

const char* attach_doc =
		"Attaches a thread to the JVM.\n"
		"\n"
		"The function manually connects a thread to the JVM to allow access to\n"
		"Java objects and methods. JPype automaticatlly attaches when a Java\n"
		"resource is used, so a call to this is usually not needed.\n"
		"\n"
		"Raises:\n"
		"  RuntimeError: If the JVM is not running.\n";

const char* detach_doc =
		"Detaches a thread from the JVM.\n"
		"\n"
		"This function detaches the thread and frees the associated resource in\n"
		"the JVM. For codes making heavy use of threading this should be used\n"
		"to prevent resource leaks. The thread can be reattached, so there\n"
		"is no harm in detaching early or more than once. This method cannot fail\n"
		"and there is no harm in calling it when the JVM is not running.\n";

static PyMethodDef contextMethods[] = {
	// JVM control
	{"isStarted", (PyCFunction) (&PyJPContext::isStarted), METH_NOARGS, ""},
	{"_startup", (PyCFunction) (&PyJPContext::startup), METH_VARARGS, ""},
	{"shutdown", (PyCFunction) (&PyJPContext::shutdown), METH_NOARGS, shutdown_doc},

	// Threading
	{"isThreadAttached", (PyCFunction) (&PyJPContext::isThreadAttached), METH_NOARGS, check_doc},
	{"attachThread", (PyCFunction) (&PyJPContext::attachThread), METH_NOARGS, attach_doc},
	{"detachThread", (PyCFunction) (&PyJPContext::detachThread), METH_NOARGS, detach_doc},
	{"attachThreadAsDaemon", (PyCFunction) (&PyJPContext::attachThreadAsDaemon), METH_NOARGS, ""},

	// ByteBuffer
	{"_convertToDirectBuffer", (PyCFunction) (&PyJPContext::convertToDirectByteBuffer), METH_VARARGS, ""},

	{NULL},
};

static PyMemberDef contextMembers[] = {
	{"_classes", T_OBJECT, offsetof(PyJPContext, m_Classes), READONLY},
	{0}
};

PyTypeObject PyJPContext::Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	/* tp_name           */ "_jpype.PyJPContext",
	/* tp_basicsize      */ sizeof (PyJPContext),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPContext::__dealloc__,
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
	/* tp_str            */ (reprfunc) PyJPContext::__str__,
	/* tp_getattro       */ PyObject_GenericGetAttr,
	/* tp_setattro       */ PyObject_GenericSetAttr,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
	/* tp_doc            */ "Java Virtual Machine Context",
	/* tp_traverse       */ (traverseproc) PyJPContext::traverse,
	/* tp_clear          */ (inquiry) PyJPContext::clear,
	/* tp_richcompare    */ 0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter           */ 0,
	/* tp_iternext       */ 0,
	/* tp_methods        */ contextMethods,
	/* tp_members        */ contextMembers,
	/* tp_getset         */ 0,
	/* tp_base           */ 0,
	/* tp_dict           */ 0,
	/* tp_descr_get      */ 0,
	/* tp_descr_set      */ 0,
	/* tp_dictoffset     */ offsetof(PyJPContext, m_Dict),
	/* tp_init           */ (initproc) PyJPContext::__init__,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPContext::__new__
};

// Static methods

void PyJPContext::initType(PyObject* module)
{
	PyType_Ready(&PyJPContext::Type);
	Py_INCREF(&PyJPContext::Type);
	PyModule_AddObject(module, "PyJPContext", (PyObject*) (&PyJPContext::Type));
}

bool PyJPContext::check(PyObject* o)
{
	return o != NULL && PyObject_TypeCheck(o, &PyJPContext::Type);
}

PyObject* PyJPContext::__new__(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
	PyJPContext* self = (PyJPContext*) type->tp_alloc(type, 0);
	self->m_Context = NULL;
	self->m_Classes = PyDict_New();
	self->m_Context = new JPContext();
	self->m_Context->setHost((PyObject*) self);
	return (PyObject*) self;
}

int PyJPContext::__init__(PyJPContext* self, PyObject* args, PyObject* kwargs)
{
	JP_TRACE_IN_C("PyJPContext::__init__", self);
	try
	{
		return 0;
	}
	PY_STANDARD_CATCH(-1);
	JP_TRACE_OUT_C;
}

void PyJPContext::__dealloc__(PyJPContext *self)
{
	JP_TRACE_IN_C("PyJPContext::__dealloc__", self);
	if (self->m_Context->isRunning())
		self->m_Context->shutdownJVM();
	delete self->m_Context;
	self->m_Context = NULL;
	PyObject_GC_UnTrack(self);
	clear(self);
	// Free self
	Py_TYPE(self)->tp_free(self);
	JP_TRACE_OUT_C;
}

int PyJPContext::traverse(PyJPContext *self, visitproc visit, void *arg)
{
	Py_VISIT(self->m_Classes);
	return 0;
}

int PyJPContext::clear(PyJPContext *self)
{
	Py_CLEAR(self->m_Classes);
	return 0;
}

PyObject* PyJPContext::__str__(PyJPContext *self)
{
	try
	{
		JPContext *context = self->m_Context;
		stringstream sout;
		sout << "<java context " << context << ">";
		return JPPyString::fromStringUTF8(sout.str()).keep();
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject* PyJPContext::startup(PyJPContext *self, PyObject *args)
{
	JP_TRACE_IN_C("PyJPContext::startup", self);
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
	try
	{
		PyObject *vmOpt;
		PyObject *vmPath;
		char ignoreUnrecognized = true;
		char convertStrings = true;

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
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject* PyJPContext::shutdown(PyJPContext *self, PyObject *args)
{
	JP_TRACE_IN_C("PyJPContext::shutdown", self);
	try
	{
		self->m_Context->shutdownJVM();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject* PyJPContext::isStarted(PyJPContext *self, PyObject *args)
{
	return PyBool_FromLong(self->m_Context->isRunning());
}

PyObject* PyJPContext::attachThread(PyJPContext *self, PyObject *args)
{
	JP_TRACE_IN_C("PyJPContext::attachThread", self);
	try
	{
		JPContext *context = self->m_Context;
		ASSERT_JVM_RUNNING(context);
		context->attachCurrentThread();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject* PyJPContext::attachThreadAsDaemon(PyJPContext *self, PyObject *args)
{
	JP_TRACE_IN_C("PyJPContext::attachThreadAsDaemon", self);
	try
	{
		JPContext *context = self->m_Context;
		ASSERT_JVM_RUNNING(context);
		context->attachCurrentThreadAsDaemon();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject* PyJPContext::detachThread(PyJPContext *self, PyObject *args)
{
	JP_TRACE_IN_C("PyJPContext::detachThread", self);
	try
	{
		if (self->m_Context->isRunning())
			self->m_Context->detachCurrentThread();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}

PyObject* PyJPContext::isThreadAttached(PyJPContext *self, PyObject *args)
{
	try
	{
		if (!self->m_Context->isRunning())
			return PyBool_FromLong(0);
		return PyBool_FromLong(self->m_Context->isThreadAttached());
	}
	PY_STANDARD_CATCH(NULL);
}

PyObject* PyJPContext::convertToDirectByteBuffer(PyJPContext *self, PyObject *args)
{
	JP_TRACE_IN_C("PyJPContext::convertToDirectByteBuffer", self);
	try
	{
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
			jobject ref = self->m_Context->_byte->convertToDirectBuffer(src);

			// Bind lifespan of the python to the java object.
			context->getReferenceQueue()->registerRef(ref, src);

			// Convert to python object

			jvalue v;
			v.l = ref;
			JPClass *type = context->getTypeManager()->findClassForObject(ref);
			res = type->convertToPythonObject(v).keep();
		}

		if (res != NULL)
		{
			return res;
		}

		JP_RAISE_RUNTIME_ERROR("Do not know how to convert to direct byte buffer, only memory view supported");
	}
	PY_STANDARD_CATCH(NULL);
	JP_TRACE_OUT_C;
}
