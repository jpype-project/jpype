// --- file: common/jp_pybasetype.cpp ---
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
#include "jp_pybasetype.h"

JPPybaseType::JPPybaseType(JPJavaFrame& frame,
		jclass clss,
		const string& name,
		JPClass* super,
		JPClassList& interfaces,
		jint modifiers)
: JPClass(frame, clss, name, super, interfaces, modifiers)
{
}

JPPybaseType::~JPPybaseType()
= default;

JPMatch::Type JPPybaseType::findJavaConversion(JPMatch& match)
{
	// Rules for java.lang.Object
	JP_TRACE_IN("JPPybaseType::canConvertToJava");
	if (nullConversion->matches(this, match)
			|| j2pythonConversion->matches(this, match)
			|| objectConversion->matches(this, match)
			|| pythonConversion->matches(this, match)
			|| proxyConversion->matches(this, match)
			|| hintsConversion->matches(this, match)
			)
		return match.type;
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPPybaseType::getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info)
{
	JP_TRACE_IN("JPPybaseType::getConversionInfo");
	nullConversion->getInfo(frame, this, info);
	objectConversion->getInfo(frame, this, info);
	j2pythonConversion->getInfo(frame, this, info);
	pythonConversion->getInfo(frame, this, info);
	proxyConversion->getInfo(frame, this, info);
	hintsConversion->getInfo(frame, this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
	JP_TRACE_OUT;
}
