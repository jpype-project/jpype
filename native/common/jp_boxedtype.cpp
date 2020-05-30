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
}

JPBoxedType::~JPBoxedType()
{
}

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
	JPJavaFrame frame = JPJavaFrame::outer(m_Context);
	m_PrimitiveType->getConversionInfo(info);
	JPPyObject::call(PyObject_CallMethod(info.expl, "extend", "O", info.implicit));
	JPPyObject::call(PyObject_CallMethod(info.implicit, "clear", ""));
	JPPyObject::call(PyObject_CallMethod(info.implicit, "extend", "O", info.exact));
	JPPyObject::call(PyObject_CallMethod(info.exact, "clear", ""));
	JPClass::getConversionInfo(info);
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
		if (value.l == NULL)
		{
			return JPPyObject::getNone();
		}

		cls = frame.findClassForObject(value.l);
		if (cls != this)
			return cls->convertToPythonObject(frame, value, true);
	}

	JPPyObject wrapper = PyJPClass_create(frame, cls);
	JPPyObject obj;
	JPContext *context = frame.getContext();
	if (this->getPrimitive() == context->_char)
	{
		jchar value2 = 0;
		// Not null get the char value
		if (value.l != 0)
			value2 = context->_char->getValueFromObject(JPValue(this, value)).getValue().c;
		// Create a char string object
		obj = JPPyObject::call(PyJPChar_Create((PyTypeObject*) wrapper.get(), value2));
	} else
		obj = PyJPNumber_create(frame, wrapper, JPValue(cls, value));
	PyJPValue_assignJavaSlot(frame, obj.get(), JPValue(cls, value));
	return obj;
}
