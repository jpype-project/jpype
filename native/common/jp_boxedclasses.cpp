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
#include <jp_boxedclasses.h>

JPBoxedType::JPBoxedType(JPContext* context, jclass clss,
			 const string& name,
			 JPClass* super,
			 JPClassList& interfaces,
			 jint modifiers)
: JPClass(context, clss, name, super, interfaces, modifiers)
{
}

JPBoxedType::~JPBoxedType()
{
}

void JPBoxedType::setPrimitiveType(JPPrimitiveType* primitiveType)
{
	m_PrimitiveType = primitiveType;
}

JPMatch::Type JPBoxedType::getJavaConversion(JPMatch& match, JPJavaFrame& frame, PyObject* pyobj)
{
	JPClass::getJavaConversion(match, frame, pyobj);
	if (match.type != JPMatch::_none)
		return match.type;
	if (this->m_PrimitiveType->getJavaConversion(match, frame, pyobj) != JPMatch::_none)
	{
		match.conversion = boxConversion;
		return match.type = JPMatch::_explicit;
	}
	return match.type = JPMatch::_none;
}

