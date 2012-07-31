/*****************************************************************************
   Copyright 2004 Steve Ménard

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
#ifndef _JPFIELD_H_
#define _JPFIELD_H_

/**
 * Field object
 */
class JPField
{
public :
	/**
	 * default constructor
	 */
	JPField();
	
	/**
	 * Create a new field based on class and java.lang.Field object
	 */
	JPField(JPClass* clazz, jobject fld);
	
	JPField(const JPField&);
	
	/**
	 * destructor
	 */
	virtual ~JPField();
	
public :
	bool isStatic();
	
	string getName();
	JPTypeName getType()
	{
		return m_Type;
	}
	
	HostRef* getStaticAttribute();
	void     setStaticAttribute(HostRef* val);
	
	HostRef* getAttribute(jobject inst);
	void     setAttribute(jobject inst, HostRef* val);

	bool isFinal()
	{
		return m_IsFinal;
	}

private :
	string                 m_Name;
	JPClass*			   m_Class;
	bool                   m_IsStatic;
	bool                   m_IsFinal;
	jobject                m_Field;
	jfieldID               m_FieldID;
	JPTypeName             m_Type;
};

#endif // _JPFIELD_H_

