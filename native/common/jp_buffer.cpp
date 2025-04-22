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
#include "jp_buffer.h"
#include "jp_primitive_accessor.h"
#include "jp_buffertype.h"

JPBuffer::JPBuffer(const JPValue &value)
: m_Object(value.getValue().l)
{
	m_Class = dynamic_cast<JPBufferType*>( value.getClass());
	JPJavaFrame frame = JPJavaFrame::outer();
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
	m_Buffer.len = m_Buffer.itemsize * m_Capacity;
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}

JPBuffer::~JPBuffer()
= default;

bool JPBuffer::isReadOnly() const
{
	return m_Buffer.readonly != 0;
}

Py_buffer& JPBuffer::getView()
{
	return m_Buffer;
}

bool JPBuffer::isValid() const
{
	return m_Capacity != -1;
}
