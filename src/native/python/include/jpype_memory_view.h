#ifdef __cplusplus
extern "C" {
#endif

#ifndef _JPYPE_PRIVATE_JPYPEMEMORYVIEW_H_
#define _JPYPE_PRIVATE_JPYPEMEMORYVIEW_H_

/*
 * Memoryview is introduced to 2.x series only in 2.7, so for supporting 2.6,
 * we need to have a minimal implementation here.
 */
#if (PY_VERSION_HEX < 0x02070000)

typedef struct {
	PyObject_HEAD
	PyObject *base;
	Py_buffer view;
} PyMemorySimpleViewObject;


extern PyTypeObject PyMemorySimpleView_Type;

#define PyMemorySimpleView_CheckExact(op) (((PyObject*)(op))->ob_type == &PyMemorySimpleView_Type)

#define PyMemorySimpleView_GET_BUFFER(op) (&((PyMemorySimpleViewObject *)(op))->view)

#define PyMemoryView_FromObject PyMemorySimpleView_FromObject
#define PyMemoryView_GET_BUFFER PyMemorySimpleView_GET_BUFFER
#define PyMemoryView_Check PyMemorySimpleView_CheckExact

#endif

void jpype_memoryview_init(PyObject* module /*PyObject **typeobject*/);

#endif

#ifdef __cplusplus
}
#endif
