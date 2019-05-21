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
#ifndef _JPVALUE_H_
#define _JPVALUE_H_

#include "jp_class.h"

/** Lightwieght representative of a jvalue with its corresponding class.
 * This should not have any memory management.  The user of the class
 * must take a global reference if needed.
 */
class JPValue
{
public:

	JPValue(JPClass* clazz, const jvalue& value)
	: m_Class(clazz), m_Value(value)
	{
	}

	~JPValue()
	{
	}

	jclass getJavaClass() const;

	JPClass* getClass() const
	{
		return m_Class;
	}

	const jvalue& getValue() const
	{
		return m_Value;
	}

	jobject getJavaObject() const;

	operator jvalue&()
	{
		return m_Value;
	}

	operator const jvalue&() const
	{
		return m_Value;
	}

private:
	JPClass* m_Class;
	jvalue  m_Value;
} ;

#endif // _JPVALUE_H_
