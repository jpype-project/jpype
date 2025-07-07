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
#include "pyjp.h"
#include "jp_boxedtype.h"

JPBoxedType::JPBoxedType(JPJavaFrame& frame, jclass clss,
		const string& name,
		JPClass* super,
		JPClassList& interfaces,
		jint modifiers,
		JPPrimitiveType* primitiveType)
: JPClass(frame, clss, name, super, interfaces, modifiers),
m_PrimitiveType(primitiveType)
{
	if (name != "java.lang.Void")
	{
		string s = string("(") + primitiveType->getTypeCode() + ")V";
		m_CtorID = frame.GetMethodID(clss, "<init>", s.c_str());
	}

	m_DoubleValueID = nullptr;
	m_FloatValueID = nullptr;
	m_LongValueID = nullptr;
	m_IntValueID = nullptr;
	m_BooleanValueID = nullptr;
	m_CharValueID = nullptr;

	if (name != "java.lang.Void" && name != "java.lang.Boolean" && name != "java.lang.Character" )
	{
		m_DoubleValueID = frame.GetMethodID(clss, "doubleValue", "()D");
		m_FloatValueID = frame.GetMethodID(clss, "floatValue", "()F");
		m_IntValueID = frame.GetMethodID(clss, "intValue", "()I");
		m_LongValueID = frame.GetMethodID(clss, "longValue", "()J");
	}

	if (name == "java.lang.Boolean")
	{
		m_BooleanValueID = frame.GetMethodID(clss, "booleanValue", "()Z");
	}

	if (name == "java.lang.Character")
	{
		m_CharValueID = frame.GetMethodID(clss, "charValue", "()C");
	}

}

JPBoxedType::~JPBoxedType()
= default;

JPMatch::Type JPBoxedType::findJavaConversion(JPMatch &match)
{
	JP_TRACE_IN("JPBoxedType::findJavaConversion");
	JPClass::findJavaConversion(match);
	if (match.type != JPMatch::_none)
		return match.type;
	if (m_PrimitiveType->findJavaConversion(match) != JPMatch::_none)
	{
		JP_TRACE("Primitive", match.type);
		match.conversion = boxBooleanConversion;
		match.closure = this;
		if (match.type == JPMatch::_exact)
			return match.type = JPMatch::_implicit;
		return match.type = JPMatch::_explicit;
	}
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPBoxedType::getConversionInfo(JPConversionInfo &info)
{
	JP_TRACE_IN("JPBoxedType::getConversionInfo");
	JPJavaFrame frame = JPJavaFrame::outer();
	m_PrimitiveType->getConversionInfo(info);
	JPPyObject::call(PyObject_CallMethod(info.expl, "extend", "O", info.implicit));
	JPPyObject::call(PyObject_CallMethod(info.implicit, "clear", ""));
	JPPyObject::call(PyObject_CallMethod(info.implicit, "extend", "O", info.exact));
	JPPyObject::call(PyObject_CallMethod(info.exact, "clear", ""));
	JPClass::getConversionInfo(info);
	JP_TRACE_OUT;
}

jobject JPBoxedType::box(JPJavaFrame &frame, jvalue v)
{
	return frame.NewObjectA(m_Class.get(), m_CtorID, &v);
}

JPPyObject JPBoxedType::convertToPythonObject(JPJavaFrame& frame, jvalue value, bool cast)
{
	JPClass *cls = this;
	if (!cast)
	{
		// This loses type
		if (value.l == nullptr)
		{
			return JPPyObject::getNone();
		}

		cls = frame.findClassForObject(value.l);
		if (cls != this)
			return cls->convertToPythonObject(frame, value, true);
	}

	JPPyObject wrapper = PyJPClass_create(frame, cls);
	JPPyObject obj;
	JPContext *context = JPContext_global;
	if (this->getPrimitive() == context->_char)
	{
		jchar value2 = 0;
		// Not null get the char value
		if (value.l != nullptr)
			value2 = context->_char->getValueFromObject(frame, JPValue(this, value)).getValue().c;
		// Create a char string object
		obj = JPPyObject::call(PyJPChar_Create((PyTypeObject*) wrapper.get(), value2));
	} else
		obj = PyJPNumber_create(frame, wrapper, JPValue(cls, value));
	PyJPValue_assignJavaSlot(frame, obj.get(), JPValue(cls, value));
	return obj;
}
