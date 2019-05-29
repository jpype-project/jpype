/*****************************************************************************
   Copyright 2004 Steve Mï¿½nard

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

#include "jp_modifier.h"
#include "jp_class.h"

/**
 * Field object
 */
class JPField
{
public:
	/**
	 * Create a new field based on class and java.lang.Field object
	 */
	JPField(JPClass* cls,
			const string& name,
			jobject field,
			JPClass* fieldType,
			jint modifiers);

	/**
	 * destructor
	 */
	virtual ~JPField();

public:
	JPContext* getContext()
	{
		return m_Class->getContext();
	}

	string toString() const;

	const string& getName() const
	{
		return m_Name;
	}


	JPPyObject getStaticField();
	void     setStaticField(PyObject* val);

	JPPyObject getField(jobject inst);
	void     setField(jobject inst, PyObject* val);

	bool isFinal() const
	{
		return JPModifier::isFinal(m_Modifiers);
	}

	bool isStatic() const
	{
		return JPModifier::isStatic(m_Modifiers);
	}

private:
	JPField(const JPField&);
	JPField& operator=(const JPField&) ;

	void ensureTypeCache();

private:
	string           m_Name;
	JPClass*         m_Class;
	JPObjectRef      m_Field;
	jfieldID         m_FieldID;
	JPClass*         m_Type;
	jint             m_Modifiers;
} ;

typedef vector<JPField> JPFieldList;

#endif // _JPFIELD_H_

