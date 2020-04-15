#include "jpype.h"
#include "pyjp.h"
#include "jp_buffer.h"
#include "jp_buffertype.h"

#ifdef __cplusplus
extern "C"
{
#endif

static void PyJPBuffer_dealloc(PyJPBuffer *self)
{
	JP_PY_TRY("PyJPBuffer_dealloc");
	delete self->m_Buffer;
	Py_TYPE(self)->tp_free(self);
	JP_PY_CATCH();
}

static PyObject *PyJPBuffer_repr(PyJPBuffer *self)
{
	JP_PY_TRY("PyJPBuffer_repr");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	if (self->m_Buffer == NULL)
		JP_RAISE(PyExc_ValueError, "Null array");
	stringstream sout;

	sout << "<java beffer " << self->m_Buffer->getClass()->toString() << ">";
	return JPPyString::fromStringUTF8(sout.str()).keep();
	JP_PY_CATCH(0);
}

static void PyJPBuffer_releaseBuffer(PyJPBuffer *self, Py_buffer *view)
{
	JP_PY_TRY("PyJPBufferPrimitive_releaseBuffer");
	JP_PY_CATCH();
}

int PyJPBuffer_getBuffer(PyJPBuffer *self, Py_buffer *view, int flags)
{
	JP_PY_TRY("PyJPBufferPrimitive_getBuffer");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame(context);
	if (self->m_Buffer == NULL)
		JP_RAISE(PyExc_ValueError, "Null buffer");
	try
	{
		JPBuffer *buffer = self->m_Buffer;

		if (!buffer->isValid())
		{
			PyErr_SetString(PyExc_BufferError, "Java buffer is not direct");
			return -1;
		}

		if ((flags & PyBUF_WRITEABLE) == PyBUF_WRITEABLE && buffer->isReadOnly())
		{
			PyErr_SetString(PyExc_BufferError, "Java buffer is not writable");
			return -1;
		}

		*view = buffer->getView();

		// If strides are not requested and this is a slice then fail
		if ((flags & PyBUF_STRIDES) != PyBUF_STRIDES)
		{
			if (view->strides[0] != view->itemsize)
				JP_RAISE(PyExc_BufferError, "slices required strides");
			view->strides = NULL;
		}

		// If shape is not requested
		if ((flags & PyBUF_ND) != PyBUF_ND)
		{
			view->shape = NULL;
		}

		// If format is not requested
		if ((flags & PyBUF_FORMAT) != PyBUF_FORMAT)
			view->format = NULL;

		// Okay all successful so reference the parent object
		view->obj = (PyObject*) self;
		Py_INCREF(view->obj);
		return 0;
	} catch (JPypeException &ex)
	{
		PyJPBuffer_releaseBuffer(self, view);

		// We are only allowed to raise BufferError
		PyErr_SetString(PyExc_BufferError, "Java buffer view failed");
		return -1;
	}
	JP_PY_CATCH(-1);
}

static PyType_Slot bufferSlots[] = {
	{ Py_tp_dealloc,  (void*) PyJPBuffer_dealloc},
	{ Py_tp_repr,     (void*) PyJPBuffer_repr},
	{0}
};

static PyBufferProcs directBuffer = {
	(getbufferproc) & PyJPBuffer_getBuffer,
	(releasebufferproc) & PyJPBuffer_releaseBuffer
};

PyTypeObject *PyJPBuffer_Type = NULL;
static PyType_Spec bufferSpec = {
	"_jpype._JBuffer",
	sizeof (PyJPBuffer),
	0,
	Py_TPFLAGS_DEFAULT  | Py_TPFLAGS_BASETYPE,
	bufferSlots
};

#ifdef __cplusplus
}
#endif

void PyJPBuffer_initType(PyObject * module)
{
	JPPyTuple tuple = JPPyTuple::newTuple(1);
	tuple.setItem(0, (PyObject*) PyJPObject_Type);
	PyJPBuffer_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&bufferSpec, tuple.get());
	PyJPBuffer_Type->tp_as_buffer = &directBuffer;
	JP_PY_CHECK();
	PyModule_AddObject(module, "_JBuffer", (PyObject*) PyJPBuffer_Type);
	JP_PY_CHECK();
}

JPPyObject PyJPBuffer_create(JPJavaFrame &frame, PyTypeObject *type, const JPValue& value)
{
	JPPyObject obj(JPPyRef::_call, type->tp_alloc(type, 0));
	((PyJPBuffer*) obj.get())->m_Buffer = new JPBuffer(value);
	PyJPValue_assignJavaSlot(frame, obj.get(), value);
	return obj;
}
