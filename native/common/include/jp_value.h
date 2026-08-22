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

	/** Persistent, pool-tracked handle - used when a JPValue is embedded as
	 * a Python object's Java binding slot (see PyJPValue_assignJavaSlot)
	 * and must survive past any single frame. Never dereference m_Ref
	 * directly; it is only meaningful via GlobalPool routing.
	 */
	static JPValue fromGlobal(JPClass* clazz, jref ref)
	{
		JPValue out;
		out.m_Class = clazz;
		out.m_Ref = ref;
		return out;
	}

	jref getRef() const
	{
		return m_Ref;
	}

	/** Resolve this value's Java object, whichever form it is stored in:
	 * a transient local ref (m_Value.l, the common "no memory management"
	 * case documented on this class) or a persistent pool-tracked handle
	 * (m_Ref, from fromGlobal()). This is the one blessed way to read an
	 * object out of a JPValue - do not read .getValue().l directly, since
	 * that silently misses the persistent case.
	 */
	jobject getJavaObject(JPJavaFrame& frame) const
	{
		if (m_Ref.value != 0)
			return frame.retrieveGlobal(m_Ref);
		return m_Value.l;
	}

	/** Cheap "is this Java object null" check that needs no frame/JNI call.
	 * GlobalPool special-cases null to the handle 0 rather than allocating
	 * a slot (see GlobalPool.add), so a persistent value is null iff its
	 * handle is 0 - same bit pattern a never-pooled/transient value uses
	 * for "no object" (m_Value.l == nullptr). Only meaningful for
	 * non-primitive values.
	 */
	bool isJavaNull() const
	{
		return m_Ref.value == 0 && m_Value.l == nullptr;
	}

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

private:
	JPClass* m_Class{};
	jvalue  m_Value{};
	jref m_Ref{};
} ;

#endif // _JPVALUE_H_
