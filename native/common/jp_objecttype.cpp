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
#include "jpype.h"
#include "pyjp.h"
#include "jp_objecttype.h"

JPObjectType::JPObjectType(JPJavaFrame& frame,
		jclass clss,
		const string& name,
		JPClass* super,
		JPClassList& interfaces,
		jint modifiers)
: JPClass(frame, clss, name, super, interfaces, modifiers)
{
}

JPObjectType::~JPObjectType()
{
}

JPMatch::Type JPObjectType::findJavaConversion(JPMatch& match)
{
	// Rules for java.lang.Object
	JP_TRACE_IN("JPObjectType::canConvertToJava");
	if (nullConversion->matches(match, this)
			|| javaObjectAnyConversion->matches(match, this)
			|| stringConversion->matches(match, this)
			|| boxBooleanConversion->matches(match, this)
			|| boxLongConversion->matches(match, this)
			|| boxDoubleConversion->matches(match, this)
			|| classConversion->matches(match, this)
			|| proxyConversion->matches(match, this)
			|| hintsConversion->matches(match, this)
			)
		return match.type;
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}
