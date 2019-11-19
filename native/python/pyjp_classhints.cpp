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
#include <pyjp_classhints.h>

	static PyMethodDef classMethods[] = {
	{NULL},
};

PyTypeObject PyJPClassHints::Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	/* tp_name           */ "PyJPClassHints",
	/* tp_basicsize      */ sizeof (PyJPClassHints),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPClassHints::__dealloc__,
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
	/* tp_str            */ (reprfunc) PyJPClassHints::__str__,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT, // | Py_TPFLAGS_HAVE_GC,
	/* tp_doc            */ "Java Class Hints",
	/* tp_traverse       */ 0,  // (traverseproc) PyJPClassHints::traverse,
	/* tp_clear          */ 0,  //(inquiry) PyJPClassHints::clear,
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
	/* tp_init           */ (initproc) PyJPClassHints::__init__,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyJPClassHints::__new__
};

// Static methods

void PyJPClassHints::initType(PyObject *module)
{
	PyType_Ready(&PyJPClassHints::Type);
	Py_INCREF(&PyJPClassHints::Type);
	PyModule_AddObject(module, "PyJPClassHints", (PyObject*) (&PyJPClassHints::Type));
}

PyObject *PyJPClassHints::__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_TRACE_IN("PyJPProxy::new");
	PyJPClassHints *self = (PyJPClassHints*) type->tp_alloc(type, 0);
	self->m_Hints = NULL;
	return (PyObject*) self;
	JP_TRACE_OUT;
}

int PyJPClassHints::__init__(PyJPClassHints *self, PyObject *args, PyObject *kwargs)
{
	JP_TRACE_IN_C("PyJPClassHints::init", self);
	try
	{
		// Parse arguments
		PyObject *target;
		PyObject *pyintf;
		if (!PyArg_ParseTuple(args, "OO", &target, &pyintf))
		{
			return -1;
		}

		return 0;
	}
	PY_STANDARD_CATCH(-1);
	JP_TRACE_OUT_C;
}

void PyJPClassHints::__dealloc__(PyJPClassHints *self)
{
	JP_TRACE_IN_C("PyJPClassHints::dealloc", self);
	delete self->m_Hints;

//	PyObject_GC_UnTrack(self);
//	clear(self);
	// Free self
	Py_TYPE(self)->tp_free(self);
	JP_TRACE_OUT_C;
}

//int PyJPClassHints::traverse(PyJPClassHints *self, visitproc visit, void *arg)
//{
//	JP_TRACE_IN_C("PyJPClassHints::traverse", self);
//	return 0;
//	JP_TRACE_OUT_C;
//}
//
//int PyJPClassHints::clear(PyJPClassHints *self)
//{
//	JP_TRACE_IN_C("PyJPClassHints::clear", self);
//	return 0;
//	JP_TRACE_OUT_C;
//}

PyObject *PyJPClassHints::__str__(PyJPClassHints *self)
{
	try
	{
//		JPContext *context = self->m_Context->m_Context;
//		ASSERT_JVM_RUNNING(context);
//		JPJavaFrame frame(context);
		stringstream sout;
		sout << "<java class hints>";
		return JPPyString::fromStringUTF8(sout.str()).keep();
	}
	PY_STANDARD_CATCH(NULL);
}

bool PyJPClassHints::check(PyObject *o)
{
	return Py_TYPE(o) == &PyJPClassHints::Type;
}
