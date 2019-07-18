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
// Class<java.lang.Object> and Class<java.lang.Class> have special rules

JPObjectType::JPObjectType(JPContext* context,
		jclass clss,
		const string& name,
		JPClass* super,
		JPClassList& interfaces,
		jint modifiers)
: JPClass(context, clss, name, super, interfaces, modifiers)
{
}

JPObjectType::~JPObjectType()
{
}

JPMatch::Type JPObjectType::getJavaConversion(JPMatch& match, JPJavaFrame& frame, PyObject* pyobj)
{
	// Implicit rules for java.lang.Object
	JP_TRACE_IN("JPObjectType::canConvertToJava");
	if (nullConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
		return match.type;
	if (javaObjectAnyConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
		return match.type;

	// User conversions come before general attempts.

	if (JPPyString::check(pyobj))
	{
		match.conversion = stringConversion;
		return match.type = JPMatch::_implicit;
	}
	if (JPPyBool::check(pyobj))
	{
		match.conversion = boxBooleanConversion;
		return match.type = JPMatch::_implicit;
	}

	if (JPPyFloat::check(pyobj))
	{
		match.conversion = boxDoubleConversion;
		return match.type = JPMatch::_implicit;
	}

	if (JPPyLong::check(pyobj) || (JPPyLong::checkConvertable(pyobj) && JPPyLong::checkIndexable(pyobj)))
	{
		match.conversion = boxLongConversion;
		return match.type = JPMatch::_implicit;
	}

	if (JPPyFloat::checkConvertable(pyobj))
	{
		match.conversion = boxDoubleConversion;
		return match.type = JPMatch::_implicit;
	}

	if (classConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
		return match.type;

	JPProxy* proxy = JPPythonEnv::getJavaProxy(pyobj);
	if (proxy != NULL)
	{
		match.conversion = proxyConversion;
		return match.type = JPMatch::_implicit;
	}

	return match.type;
	JP_TRACE_OUT;
}
