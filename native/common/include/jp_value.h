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
#ifndef _JPVALUE_H_
#define _JPVALUE_H_

#include "jp_class.h"
#include "jp_javaframe.h"

/** Lightweight representative of a jvalue with its corresponding class.
 * This should not have any memory management.  The user of the class
 * must take a global reference if needed.
 */
class JPValue
{
public:

	JPValue()
	{
		m_Value.l = nullptr;
	}

	JPValue(JPClass* clazz, const jvalue& value)
	: m_Class(clazz), m_Value(value)
	{
	}

	JPValue(JPClass* clazz, jobject value)
	: m_Class(clazz)
	{
		m_Value.l = value;
	}

	~JPValue() = default;

	JPClass* getClass() const
	{
		return m_Class;
	}

	jvalue& getValue()
	{
		return m_Value;
	}

	const jvalue& getValue() const
	{
		return m_Value;
	}

	jobject getJavaObject() const;

    // Cast operators to jvalue.
    // TODO: these could be explicit too, right?
	operator jvalue&()
	{
		return m_Value;
	}

	operator const jvalue&() const
	{
		return m_Value;
	}

    // TODO: never used.
	JPValue& global(JPJavaFrame& frame)
	{
		m_Value.l = frame.NewGlobalRef(m_Value.l);
		return *this;
	}

private:
	JPClass* m_Class{};
	jvalue  m_Value{};
} ;

#endif // _JPVALUE_H_
