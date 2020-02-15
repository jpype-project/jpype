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
public:
	/**
	 * Create a new field based on class and java.lang.Field object
	 */
	JPField(JPClass* clazz, jobject fld);

	/**
	 * destructor
	 */
	virtual ~JPField();

public:
	bool isStatic() const;

	string toString() const;

	const string& getName() const;

	JPPyObject getStaticField();
	void     setStaticField(PyObject* val);

	JPPyObject getField(jobject inst);
	void     setField(jobject inst, PyObject* val);

	bool isFinal() const
	{
		return m_IsFinal;
	}

	JPClass *getClass()
	{
		return m_Class;
	}

private:
	JPField(const JPField&);
	JPField& operator=(const JPField&) ;

	void ensureTypeCache();

private:
	string           m_Name;
	JPClass*         m_Class;
	bool             m_IsStatic;
	bool             m_IsFinal;
	JPObjectRef      m_Field;
	jfieldID         m_FieldID;
	JPClassRef       m_Type;
	JPClass*         m_TypeCache;
} ;

#endif // _JPFIELD_H_

