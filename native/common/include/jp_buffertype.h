#ifndef _JPBUFFERTYPE_H_
#define _JPBUFFERTYPE_H_

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPBufferType : public JPClass
{
public:
	JPBufferType(JPJavaFrame& frame, jclass cls, const string& name, JPClass* superClass, const JPClassList& interfaces, jint modifiers);
	virtual~ JPBufferType();

	char* getType()
	{
		return const_cast<char*> (m_Type);
	}

	int getSize()
	{
		return m_Size;
	}

	JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue value, bool cast) override;

private:
	const char* m_Type;
	int m_Size;
} ;

#endif // _JPBUFFERCLASS_H_
