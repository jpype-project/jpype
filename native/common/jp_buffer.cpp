#include "jpype.h"
#include "jp_primitive_accessor.h"
#include "jp_buffertype.h"

JPBuffer::JPBuffer(const JPValue &value)
: m_Object(value.getClass()->getContext(), value.getValue().l)
{
	m_Class = (JPBufferType*) value.getClass();
	JPJavaFrame frame(m_Class->getContext());
	JP_TRACE_IN("JPBuffer::JPBuffer");
	m_Address = frame.GetDirectBufferAddress(m_Object.get());
	m_Capacity = (Py_ssize_t) frame.GetDirectBufferCapacity(m_Object.get());
	m_Buffer.buf = m_Address;
	m_Buffer.format = m_Format;
	m_Format[0] = frame.orderBuffer(m_Object.get()) ? '<' : '>';
	m_Format[1] = m_Class->getType()[0];
	m_Format[2] = 0;
	m_Buffer.itemsize = (Py_ssize_t) m_Class->getSize();
	m_Buffer.ndim = 1;
	m_Buffer.readonly = frame.isBufferReadOnly(m_Object.get());
	m_Buffer.shape = &m_Capacity;
	m_Buffer.strides = &m_Buffer.itemsize;
	m_Buffer.suboffsets = 0;
	JP_TRACE_OUT;
}

JPBuffer::~JPBuffer()
{
}

bool JPBuffer::isReadOnly()
{
	return m_Buffer.readonly != 0;
}

Py_buffer& JPBuffer::getView()
{
	return m_Buffer;
}

bool JPBuffer::isValid()
{
	return m_Capacity != -1;
}
