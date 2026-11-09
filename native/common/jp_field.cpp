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
		JPClass* fieldType,
		jint modifiers)
: m_Field(frame, field)
{
	m_Class = cls;
	m_Name = name;
	m_Type = fieldType;
	m_Modifiers = modifiers;
}

JPField::~JPField()
= default;

jfieldID JPField::getFieldID(JPJavaFrame& frame)
{
	jfieldID fid = m_FieldID.load(std::memory_order_acquire);
	if (fid == nullptr)
	{
		fid = frame.FromReflectedField(m_Field.get());
		m_FieldID.store(fid, std::memory_order_release);
	}
	return fid;
}

JPPyObject JPField::getStaticField()
{
	JP_TRACE_IN("JPField::getStaticAttribute");
	JPJavaFrame frame = JPJavaFrame::outer();
	return m_Type->getStaticField(frame, m_Class->getJavaClass(), getFieldID(frame));
	JP_TRACE_OUT;
}

void JPField::setStaticField(PyObject *pyobj)
{
	JP_TRACE_IN("JPField::setStaticAttribute");
	JPJavaFrame frame = JPJavaFrame::outer();
	m_Type->setStaticField(frame, m_Class->getJavaClass(), getFieldID(frame), pyobj);
	JP_TRACE_OUT;
}

JPPyObject JPField::getField(jobject inst)
{
	JP_TRACE_IN("JPField::getAttribute");
	JPJavaFrame frame = JPJavaFrame::outer();
	ASSERT_NOT_NULL(m_Type);
	JP_TRACE("field type", m_Type->getCanonicalName());
	return m_Type->getField(frame, inst, getFieldID(frame));
	JP_TRACE_OUT;
}

void JPField::setField(jobject inst, PyObject *pyobj)
{
	JP_TRACE_IN("JPField::setAttribute");
	JPJavaFrame frame = JPJavaFrame::outer();
	m_Type->setField(frame, inst, getFieldID(frame), pyobj);
	JP_TRACE_OUT;
}
