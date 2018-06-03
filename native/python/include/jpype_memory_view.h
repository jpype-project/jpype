#ifndef _JPYPE_PRIVATE_JPYPEMEMORYVIEW_H_
#define _JPYPE_PRIVATE_JPYPEMEMORYVIEW_H_

#if (PY_VERSION_HEX < 0x02070000)

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Memoryview is introduced to 2.x series only in 2.7, so for supporting 2.6,
 * we need to have a minimal implementation here.
 */

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

// Not supported
#define PyMemoryView_GetContiguous(X,Y,Z) NULL

void jpype_memoryview_init(PyObject* module /*PyObject **typeobject*/);

#ifdef __cplusplus
}
#endif
#endif // Python <2.7


/** Special handler for Python Memory view.
 */
class JPPyMemoryViewAccessor
{
	PyObject* memview;
	Py_buffer* py_buff;

public:
	JPPyMemoryViewAccessor(PyObject* sequence)
		: memview(0), py_buff(0)
	{
		if (!PyObject_CheckBuffer(sequence))
			return;
		memview = PyMemoryView_GetContiguous(sequence, PyBUF_READ, 'C');

		// check for TypeError, if no underlying py_buff exists.
		if (memview ==NULL  || PyErr_Occurred()) 
		{
			PyErr_Clear();
			return; 
		}
		py_buff = PyMemoryView_GET_BUFFER(memview);

	}

	~JPPyMemoryViewAccessor()
	{
		if (py_buff==0)
			return;
		Py_DECREF(py_buff);
		Py_DECREF(memview);
	}

	bool valid() { return py_buff!=0; }
	int size() { return py_buff->len; }
	void* get() { return py_buff->buf; }
};


#endif
