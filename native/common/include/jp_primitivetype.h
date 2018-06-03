/*****************************************************************************
   Copyright 2004 Steve M�nard

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
#ifndef _JPPRIMITIVETYPE_H_
#define _JPPRIMITIVETYPE_H_

class JPPrimitiveType : public JPType
{
protected :
	JPPrimitiveType(JPTypeName::ETypes type, bool isObject, const JPTypeName& objectType) :
		m_Type(JPTypeName::fromType(type)),
		m_IsObject(isObject),
		m_ObjectTypeName(objectType)
	{
	}
	
	virtual ~JPPrimitiveType()
	{
	}
	
private :
	JPTypeName m_Type;
	bool       m_IsObject;
	JPTypeName m_ObjectTypeName;

public :
	virtual bool       isObjectType() const
	{ 
		return m_IsObject; 
	}
	
	virtual const JPTypeName& getName() const
	{
		return m_Type;
	}
	
	virtual const JPTypeName& getObjectType() const
	{
		return m_ObjectTypeName;
	}

	virtual jobject	   convertToJavaObject(HostRef* obj);

	virtual PyObject* getArrayRangeToSequence(JPJavaFrame& frame, jarray, int start, int length) = 0;

	virtual void setArrayRange(JPJavaFrame& frame, jarray, int, int, PyObject*) = 0;
};

#endif 
