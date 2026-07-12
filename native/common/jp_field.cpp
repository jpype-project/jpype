// --- file: common/jp_field.cpp ---
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
#include "jpype.h"
#include "jp_field.h"

JPField::JPField(JPJavaFrame& frame,
		JPClass* cls,
		const string& name,
		jobject field,
		jfieldID fid,
		JPClass* fieldType,
		jint modifiers)
{
	m_Class = cls;
	m_Field = frame.storeGlobal(field);
	m_Name = name;
	m_FieldID = fid;
	m_Type = fieldType;
	m_Modifiers = modifiers;
}

JPField::~JPField()
{
	tryRelease(m_Field);
}

JPPyObject JPField::getStaticField(JPJavaFrame& frame)
{
	JP_TRACE_IN("JPField::getStaticAttribute");
	return m_Type->getStaticField(frame, m_Class->getJavaClass(frame), m_FieldID);
	JP_TRACE_OUT;
}

void JPField::setStaticField(JPJavaFrame& frame, PyObject *pyobj)
{
	JP_TRACE_IN("JPField::setStaticAttribute");
	m_Type->setStaticField(frame, m_Class->getJavaClass(frame), m_FieldID, pyobj);
	JP_TRACE_OUT;
}

JPPyObject JPField::getField(JPJavaFrame& frame, jobject inst)
{
	JP_TRACE_IN("JPField::getAttribute");
	ASSERT_NOT_NULL(m_Type, "JPField::getAttribute");
	JP_TRACE("field type", m_Type->getCanonicalName());
	return m_Type->getField(frame, inst, m_FieldID);
	JP_TRACE_OUT;
}

void JPField::setField(JPJavaFrame& frame, jobject inst, PyObject *pyobj)
{
	JP_TRACE_IN("JPField::setAttribute");
	m_Type->setField(frame, inst, m_FieldID, pyobj);
	JP_TRACE_OUT;
}
