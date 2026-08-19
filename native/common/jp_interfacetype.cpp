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
#include "jp_interfacetype.h"

JPInterfaceType::JPInterfaceType(JPJavaFrame& frame,
		jclass clss,
		const string& name,
		JPClass* super,
		JPClassList& interfaces,
		jint modifiers)
: JPClass(frame, clss, name, super, interfaces, modifiers)
{
}

JPInterfaceType::~JPInterfaceType()
= default;

JPMatch::Type JPInterfaceType::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPInterfaceType::findJavaConversion");
	if (nullConversion->matches(this, match)
			|| objectConversion->matches(this, match)
			|| proxyConversion->matches(this, match)
			|| hintsConversion->matches(this, match)
			)
		return match.type;
	JP_TRACE("No match");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPInterfaceType::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	nullConversion->getInfo(this, info);
	objectConversion->getInfo(this, info);
	proxyConversion->getInfo(this, info);
	hintsConversion->getInfo(this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}
