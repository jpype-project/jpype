/*****************************************************************************
   Copyright 2004 Steve M�nard

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
#include <jp_classtype.h>

// Class<java.lang.Class> has special rules

JPClassType::JPClassType(JPJavaFrame& frame,
		jclass clss,
		const string& name,
		JPClass* super,
		JPClassList& interfaces,
		jint modifiers)
: JPClass(frame, clss, name, super, interfaces, modifiers)
{
}

JPClassType::~JPClassType()
{
}

JPMatch::Type JPClassType::findJavaConversion(JPMatch& match)
{
	JP_TRACE_IN("JPClass::getJavaConversion");
	if (nullConversion->matches(match, this) != JPMatch::_none)
		return match.type;
	if (objectConversion->matches(match, this) != JPMatch::_none)
		return match.type;
	if (classConversion->matches(match, this) != JPMatch::_none)
		return match.type;
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}
