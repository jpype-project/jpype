#include "jpype.h"
#include "pyjp.h"
#include "jp_context.h"
#include "jp_buffertype.h"

JPBufferType::JPBufferType(JPJavaFrame& frame,
		jclass cls,
		const string& name,
		JPClass* superClass,
		const JPClassList& interfaces,
		jint modifiers)
: JPClass(frame, cls, name, superClass, interfaces, modifiers)
{
	// Use name to get the type
	if (name == "java.nio.Buffer")
	{
		m_Type = "b";
		m_Size = 1;
	} else if (name == "java.nio.ByteBuffer")
	{
		m_Type = "b";
		m_Size = 1;
	} else if (name == "java.nio.CharBuffer")
	{
		m_Type = "H";
		m_Size = 2;
	} else if (name == "java.nio.ShortBuffer")
	{
		m_Type = "h";
		m_Size = 2;
	} else if (name == "java.nio.IntBuffer")
	{
		m_Type = "i";
		m_Size = 4;
	} else if (name == "java.nio.LongBuffer")
	{
		m_Type = "q";
		m_Size = 8;
	} else if (name == "java.nio.FloatBuffer")
	{
		m_Type = "f";
		m_Size = 4;
	} else if (name == "java.nio.DoubleBuffer")
	{
		m_Type = "d";
		m_Size = 8;
	} else
	{
		JPBufferType* super = dynamic_cast<JPBufferType*> (m_SuperClass);
		if (super == NULL)
			JP_RAISE(PyExc_TypeError, "Unsupported buffer type");
		m_Type = super->m_Type;
		m_Size = super->m_Size;
	}
}

JPBufferType::~JPBufferType()
{
}

JPPyObject JPBufferType::convertToPythonObject(JPJavaFrame& frame, jvalue value, bool cast)
{
	JP_TRACE_IN("JPBufferClass::convertToPythonObject");
	if (!cast && value.l == NULL)
		return JPPyObject::getNone();
	JPPyObject wrapper = PyJPClass_create(frame, this);
	JPPyObject obj = PyJPBuffer_create(frame, (PyTypeObject*) wrapper.get(), JPValue(this, value));
	return obj;
	JP_TRACE_OUT;
}
