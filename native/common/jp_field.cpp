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

JPField::JPField(JPClass* clazz, jobject fld) : m_Field(fld)
{
	JPJavaFrame frame;
	//	JP_TRACE_IN("JPField::JPField1");
	m_Class = clazz;
	m_Name = JPJni::getMemberName(fld);
	m_IsStatic = JPJni::isMemberStatic(fld);
	m_IsFinal = JPJni::isMemberFinal(fld);
	m_FieldID = frame.FromReflectedField(fld);
	m_Type = JPJni::getFieldType(m_Field.get());
	m_TypeCache = NULL;

	//	JP_TRACE_OUT
}

JPField::~JPField()
{
}

bool JPField::isStatic() const
{
	return m_IsStatic;
}

string JPField::toString() const
{
	return JPJni::toString(m_Field.get());
}

const string& JPField::getName() const
{
	return m_Name;
}

JPPyObject JPField::getStaticField()
{
	ensureTypeCache();
	JP_TRACE_IN("JPField::getStaticAttribute");
	JPJavaFrame frame;
	jclass claz = m_Class->getJavaClass();
	return m_TypeCache->getStaticField(frame, claz, m_FieldID);
	JP_TRACE_OUT;
}

void JPField::setStaticField(PyObject* val)
{
	ensureTypeCache();
	JP_TRACE_IN("JPField::setStaticAttribute");
	JPJavaFrame frame;

	if (m_IsFinal)
	{
		stringstream err;
		err << "Field " << m_Name << " is read-only";
		JP_RAISE_ATTRIBUTE_ERROR(err.str().c_str());
	}

	if (m_TypeCache->canConvertToJava(val) <= JPMatch::_explicit)
	{
		stringstream err;
		err << "unable to convert to " << m_TypeCache->getCanonicalName();
		JP_RAISE_TYPE_ERROR(err.str().c_str());
	}

	jclass claz = m_Class->getJavaClass();
	m_TypeCache->setStaticField(frame, claz, m_FieldID, val);
	JP_TRACE_OUT;
}

JPPyObject JPField::getField(jobject inst)
{
	ensureTypeCache();
	JP_TRACE_IN("JPField::getAttribute");
	JPJavaFrame frame;
	ASSERT_NOT_NULL(m_TypeCache);
	JP_TRACE("field type", m_TypeCache->getCanonicalName());
	return m_TypeCache->getField(frame, inst, m_FieldID);
	JP_TRACE_OUT;
}

void JPField::setField(jobject inst, PyObject* val)
{
	ensureTypeCache();
	JP_TRACE_IN("JPField::setAttribute");
	JPJavaFrame frame;
	if (m_IsFinal)
	{
		stringstream err;
		err << "Field " << m_Name << " is read-only";
		JP_RAISE_ATTRIBUTE_ERROR(err.str().c_str());
	}

	if (m_TypeCache->canConvertToJava(val) <= JPMatch::_explicit)
	{
		stringstream err;
		err << "unable to convert to " << m_TypeCache->getCanonicalName();
		JP_RAISE_TYPE_ERROR(err.str());
	}

	m_TypeCache->setField(frame, inst, m_FieldID, val);
	JP_TRACE_OUT;
}

void JPField::ensureTypeCache()
{
	if (m_TypeCache != NULL)
		return;
	m_TypeCache = JPTypeManager::findClass(m_Type.get());
	ASSERT_NOT_NULL(m_TypeCache);
}
