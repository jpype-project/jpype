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

JPMatch::Type JPObjectType::getJavaConversion(JPJavaFrame* frame, JPMatch& match, PyObject* pyobj)
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
	if (PyBool_Check(pyobj))
	{
		match.conversion = boxBooleanConversion;
		return match.type = JPMatch::_implicit;
	}

	if (PyFloat_Check(pyobj))
	{
		match.conversion = boxDoubleConversion;
		return match.type = JPMatch::_implicit;
	}

	if (PyLong_CheckExact(pyobj) || PyIndex_Check(pyobj))
	{
		match.conversion = boxLongConversion;
		return match.type = JPMatch::_implicit;
	}

	if (PyFloat_Check(pyobj))
	{
		match.conversion = boxDoubleConversion;
		return match.type = JPMatch::_implicit;
	}

	if (classConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
		return match.type;

	JPProxy* proxy = PyJPProxy_getJPProxy(pyobj);
	if (proxy != NULL)
	{
		match.conversion = proxyConversion;
		return match.type = JPMatch::_implicit;
	}

	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}
