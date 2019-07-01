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
#include <jpype.h>

JPField::JPField(JPClass* cls,
		 const string& name,
		 jobject field,
		 jfieldID fid,
		 JPClass* fieldType,
		 jint modifiers)
: m_Field(cls->getContext(), field)
{
	this->m_Class = cls;
	this->m_Name = name;
	this->m_FieldID = fid;
	this->m_Type = fieldType;
	this->m_Modifiers = modifiers;
}

JPField::~JPField()
{
}

string JPField::toString()
{
	return getContext()->toString(m_Field.get());
}

JPPyObject JPField::getStaticField()
{
	JP_TRACE_IN("JPField::getStaticAttribute");
	JPJavaFrame frame(m_Class->getContext());
	jclass claz = m_Class->getJavaClass();
	return m_Type->getStaticField(frame, claz, m_FieldID);
	JP_TRACE_OUT;
}

void JPField::setStaticField(PyObject* val)
{
	JP_TRACE_IN("JPField::setStaticAttribute");
	JPJavaFrame frame(m_Class->getContext());

	if (isFinal())
	{
		stringstream err;
		err << "Field " << m_Name << " is read-only";
		JP_RAISE_ATTRIBUTE_ERROR(err.str().c_str());
	}

	if (m_Type->canConvertToJava(val) <= JPMatch::_explicit)
	{
		stringstream err;
		err << "unable to convert to " << m_Type->getCanonicalName();
		JP_RAISE_TYPE_ERROR(err.str().c_str());
	}

	jclass claz = m_Class->getJavaClass();
	m_Type->setStaticField(frame, claz, m_FieldID, val);
	JP_TRACE_OUT;
}

JPPyObject JPField::getField(jobject inst)
{
	JP_TRACE_IN("JPField::getAttribute");
	JPJavaFrame frame(m_Class->getContext());
	ASSERT_NOT_NULL(m_Type);
	JP_TRACE("field type", m_Type->getCanonicalName());
	return m_Type->getField(frame, inst, m_FieldID);
	JP_TRACE_OUT;
}

void JPField::setField(jobject inst, PyObject* val)
{
	JP_TRACE_IN("JPField::setAttribute");
	JPJavaFrame frame(m_Class->getContext());
	if (isFinal())
	{
		stringstream err;
		err << "Field " << m_Name << " is read-only";
		JP_RAISE_ATTRIBUTE_ERROR(err.str().c_str());
	}

	if (m_Type->canConvertToJava(val) <= JPMatch::_explicit)
	{
		stringstream err;
		err << "unable to convert to " << m_Type->getCanonicalName();
		JP_RAISE_TYPE_ERROR(err.str());
	}

	m_Type->setField(frame, inst, m_FieldID, val);
	JP_TRACE_OUT;
}
