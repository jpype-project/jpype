//This is based on the simple memory view found in numpy

/* 
 * Simple PyMemoryView'ish object for Python 2.6 compatibility.
 *
 * On Python >= 2.7, we can use the actual PyMemoryView objects.
 *
 * Some code copied from the CPython implementation.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#include "jpype_memory_view.h"


#if (PY_VERSION_HEX < 0x02070000)

static PyObject * PyMemorySimpleView_FromObject(PyObject *base);

/*
 * Memory allocation
 */

static int
memorysimpleview_traverse(PyMemorySimpleViewObject *self,
						  visitproc visit, void *arg)
{
	if (self->base != NULL)
		Py_VISIT(self->base);
	if (self->view.obj != NULL)
		Py_VISIT(self->view.obj);
	return 0;
}

static int
memorysimpleview_clear(PyMemorySimpleViewObject *self)
{
	Py_CLEAR(self->base);
	PyBuffer_Release(&self->view);
	self->view.obj = NULL;
	return 0;
}

static void
memorysimpleview_dealloc(PyMemorySimpleViewObject *self)
{
	PyObject_GC_UnTrack(self);
	Py_CLEAR(self->base);
	if (self->view.obj != NULL) {
		PyBuffer_Release(&self->view);
		self->view.obj = NULL;
	}
	PyObject_GC_Del(self);
}

static PyObject *
memorysimpleview_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)
{
	PyObject *obj;
	static char *kwlist[] = {"object", 0};
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:memoryview", kwlist,
									 &obj)) {
		return NULL;
	}
	return PyMemorySimpleView_FromObject(obj);
}


/*
 * Buffer interface
 */

static int
memorysimpleview_getbuffer(PyMemorySimpleViewObject *self,
						   Py_buffer *view, int flags)
{
	return PyObject_GetBuffer(self->base, view, flags);
}

static void
memorysimpleview_releasebuffer(PyMemorySimpleViewObject *self,
							   Py_buffer *view)
{
	PyBuffer_Release(view);
}

static PyBufferProcs memorysimpleview_as_buffer = {
	(readbufferproc)0,	   /*bf_getreadbuffer*/
	(writebufferproc)0,	 /*bf_getwritebuffer*/
	(segcountproc)0,		/*bf_getsegcount*/
	(charbufferproc)0,	   /*bf_getcharbuffer*/
	(getbufferproc)memorysimpleview_getbuffer, /* bf_getbuffer */
	(releasebufferproc)memorysimpleview_releasebuffer, /* bf_releasebuffer */
};


/*
 * Getters
 */

static PyObject *
_IntTupleFromSsizet(int len, Py_ssize_t *vals)
{
	int i;
	PyObject *o;
	PyObject *intTuple;

	if (vals == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	intTuple = PyTuple_New(len);
	if (!intTuple) return NULL;
	for(i=0; i<len; i++) {
		o = PyInt_FromSsize_t(vals[i]);
		if (!o) {
			Py_DECREF(intTuple);
			return NULL;
		}
		PyTuple_SET_ITEM(intTuple, i, o);
	}
	return intTuple;
}

static PyObject *
memorysimpleview_format_get(PyMemorySimpleViewObject *self)
{
	return PyString_FromString(self->view.format);
}

static PyObject *
memorysimpleview_itemsize_get(PyMemorySimpleViewObject *self)
{
	return PyLong_FromSsize_t(self->view.itemsize);
}

static PyObject *
memorysimpleview_shape_get(PyMemorySimpleViewObject *self)
{
	return _IntTupleFromSsizet(self->view.ndim, self->view.shape);
}

static PyObject *
memorysimpleview_strides_get(PyMemorySimpleViewObject *self)
{
	return _IntTupleFromSsizet(self->view.ndim, self->view.strides);
}

static PyObject *
memorysimpleview_suboffsets_get(PyMemorySimpleViewObject *self)
{
	return _IntTupleFromSsizet(self->view.ndim, self->view.suboffsets);
}

static PyObject *
memorysimpleview_readonly_get(PyMemorySimpleViewObject *self)
{
	return PyBool_FromLong(self->view.readonly);
}

static PyObject *
memorysimpleview_ndim_get(PyMemorySimpleViewObject *self)
{
	return PyLong_FromLong(self->view.ndim);
}


static PyGetSetDef memorysimpleview_getsets[] =
{
	{"format", (getter)memorysimpleview_format_get, NULL, NULL, NULL},
	{"itemsize", (getter)memorysimpleview_itemsize_get, NULL, NULL, NULL},
	{"shape", (getter)memorysimpleview_shape_get, NULL, NULL, NULL},
	{"strides", (getter)memorysimpleview_strides_get, NULL, NULL, NULL},
	{"suboffsets", (getter)memorysimpleview_suboffsets_get, NULL, NULL, NULL},
	{"readonly", (getter)memorysimpleview_readonly_get, NULL, NULL, NULL},
	{"ndim", (getter)memorysimpleview_ndim_get, NULL, NULL, NULL},
	{NULL, NULL, NULL, NULL}
};

PyTypeObject PyMemorySimpleView_Type = {
	PyObject_HEAD_INIT(NULL)
	0,										  /* ob_size */
	"_jpype.memoryview",
	sizeof(PyMemorySimpleViewObject),
	0,										  /* tp_itemsize */
	/* methods */
	(destructor)memorysimpleview_dealloc,	   /* tp_dealloc */
	0,										  /* tp_print */
	0,										  /* tp_getattr */
	0,										  /* tp_setattr */
	(cmpfunc)0,								 /* tp_compare */
	(reprfunc)0,								/* tp_repr */
	0,										  /* tp_as_number */
	0,										  /* tp_as_sequence */
	0,										  /* tp_as_mapping */
	0,										  /* tp_hash */
	0,										  /* tp_call */
	(reprfunc)0,								/* tp_str */
	0,										  /* tp_getattro */
	0,										  /* tp_setattro */
	&memorysimpleview_as_buffer,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC
	| Py_TPFLAGS_HAVE_NEWBUFFER,				/* tp_flags */
	0,										  /* tp_doc */
	(traverseproc)memorysimpleview_traverse,	/* tp_traverse */
	(inquiry)memorysimpleview_clear,			/* tp_clear */
	0,										  /* tp_richcompare */
	0,										  /* tp_weaklistoffset */
	0,										  /* tp_iter */
	0,										  /* tp_iternext */
	0,										  /* tp_methods */
	0,										  /* tp_members */
	memorysimpleview_getsets,				   /* tp_getset */
	0,										  /* tp_base */
	0,										  /* tp_dict */
	0,										  /* tp_descr_get */
	0,										  /* tp_descr_set */
	0,										  /* tp_dictoffset */
	0,										  /* tp_init */
	0,										  /* tp_alloc */
	memorysimpleview_new,					   /* tp_new */
	0,										  /* tp_free */
	0,										  /* tp_is_gc */
	0,										  /* tp_bases */
	0,										  /* tp_mro */
	0,										  /* tp_cache */
	0,										  /* tp_subclasses */
	0,										  /* tp_weaklist */
	0,										  /* tp_del */
#if PY_VERSION_HEX >= 0x02060000
	0,										  /* tp_version_tag */
#endif
};


/*
 * Factory
 */
static PyObject *
PyMemorySimpleView_FromObject(PyObject *base)
{
	PyMemorySimpleViewObject *mview = NULL;
	Py_buffer view;

	if (Py_TYPE(base)->tp_as_buffer == NULL ||
		Py_TYPE(base)->tp_as_buffer->bf_getbuffer == NULL) {

		PyErr_SetString(PyExc_TypeError,
			"cannot make memory view because object does "
			"not have the buffer interface");
		return NULL;
	}

	memset(&view, 0, sizeof(Py_buffer));
	if (PyObject_GetBuffer(base, &view, PyBUF_FULL_RO) < 0)
		return NULL;

	mview = (PyMemorySimpleViewObject *)
		PyObject_GC_New(PyMemorySimpleViewObject, &PyMemorySimpleView_Type);
	if (mview == NULL) {
		PyBuffer_Release(&view);
		return NULL;
	}
	memcpy(&mview->view, &view, sizeof(Py_buffer));
	mview->base = base;
	Py_INCREF(base);

	PyObject_GC_Track(mview);
	return (PyObject *)mview;
}


/*
 * Module initialization
 */

void
jpype_memoryview_init(PyObject* module /*PyObject **typeobject*/)
{
	PyType_Ready(&PyMemorySimpleView_Type);
//	*typeobject = (PyObject*)&PyMemorySimpleView_Type;
//	return 0;
	PyModule_AddObject(module, "memoryview", (PyObject*)&PyMemorySimpleView_Type);
}

#endif
