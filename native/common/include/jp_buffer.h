#ifndef _JPBUFFER_H_
#define _JPBUFFER_H_

#include "jp_javaframe.h"

class JPBufferType;

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPBuffer
{
public:
	JPBuffer(const JPValue& array);
	virtual~ JPBuffer();

	JPBufferType* getClass()
	{
		return m_Class;
	}

	jobject getJava()
	{
		return m_Object.get();
	}

	bool isReadOnly();

	Py_buffer& getView();

	bool isValid();

private:
	JPBufferType* m_Class;
	JPObjectRef m_Object;
	void *m_Address;
	Py_ssize_t m_Capacity;
	Py_buffer m_Buffer;
	char m_Format[3];
} ;

#endif // _JPBUFFER_H_