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

class JPConversionJavaObjectAny : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch& match, JPContext* context, JPClass* cls, PyObject* pyobj)
	{
		JPValue *value = JPPythonEnv::getJavaValue(pyobj);
		if (value == NULL)
			return JPMatch::_none;
		match.conversion = this;
		match.type = (value->getClass() == this) ? JPMatch::_exact : JPMatch::_implicit;
		return match.type;
	}

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		jvalue res;
		JPValue *value = JPPythonEnv::getJavaValue(pyobj);
		if (dynamic_cast<JPPrimitiveType*> (value->getClass()) == NULL)
		{
			res.l = frame.NewLocalRef(value->getJavaObject());
			return res;
		}
		else
		{
			// Okay we need to box it.
			JPPrimitiveType* type = (JPPrimitiveType*) (value->getClass());
			res = type->getBoxedClass()->convertToJava(pyobj);
			return res;
		}
	}
} javaObjectAnyConversion;

class JPConversionString : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch& match, JPContext* context, JPClass* cls, PyObject* pyobj)
	{
		if (!JPPyString::check(pyobj))
			return JPMatch::_none;
		match.conversion = this;
		match.type = JPMatch::_implicit;
		return match.type;
	}

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		return frame.getContext()->_java_lang_String->convertToJava(pyobj);
	}
} stringConversion;

class JPConversionBoxLong : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch& match, JPContext* context, JPClass* cls, PyObject* pyobj)
	{
		return JPMatch::_none;
	}

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		return frame.getContext()->_java_lang_Boolean->convertToJava(pyobj);
	}
} boxBooleanConversion;

class JPConversionBoxLong : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch& match, JPContext* context, JPClass* cls, PyObject* pyobj)
	{
		XXX
		if (JPPyLong::checkConvertable(pyobj) && JPPyLong::checkIndexable(pyobj))
			return JPMatch::_implicit;
		return JPMatch::_none;
	}

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		return frame.getContext()->_java_lang_Long->convertToJava(pyobj);
	}
} boxLongConversion;

class JPConversionBoxDouble : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch& match, JPContext* context, JPClass* cls, PyObject* pyobj)
	{
		if (JPPyLong::checkConvertable(pyobj) && JPPyLong::checkIndexable(pyobj))
			return JPMatch::_implicit;
		return JPMatch::_none;
	}

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		return frame.getContext()->_java_lang_Double->convertToJava(pyobj);
	}
} boxDoubleConversion;

class JPConversionProxyAny : public JPConversion
{
public:

	JPMatch::Type matches(JPMatch& match, JPContext* context, JPClass* cls, PyObject* pyobj)
	{
		JPProxy* proxy = JPPythonEnv::getJavaProxy(pyobj);
		if (!proxy)
			return JPMatch::_none;
		match.conversion = this;
		return match.type = JPMatch::_implicit;
	}

	jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj)
	{
		jvalue res;
		JPProxy* proxy = JPPythonEnv::getJavaProxy(pyobj);
		res.l = proxy->getProxy();
		return res;
	}
} proxyAnyConversion;

JPMatch::Type JPObjectType::getJavaConversion(JPMatch& match, JPJavaFrame& frame, PyObject* pyobj)
{
	// Implicit rules for java.lang.Object
	JP_TRACE_IN("JPObjectType::canConvertToJava");
	if (nullConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
		return match.type;
	if (javaObjectAnyConversion.matches(match, frame, this, pyobj) != JPMatch::_none)
		return match.type;

	// User conversions come before general attempts.

	if (JPPyString::check(pyobj))
	{
		match.conversion = &stringConversion;
		return match.type = JPMatch::_implicit;
	}
	if (JPPyBool::check(pyobj))
	{
		match.conversion = &boxBooleanConversion;
		return match.type = JPMatch::_implicit;
	}

	if (JPPyFloat::check(pyobj))
	{
		match.conversion = &boxDoubleConversion;
		return match.type = JPMatch::_implicit;
	}

	if (JPPyLong::check(pyobj) || (JPPyLong::checkConvertable(pyobj) && JPPyLong::checkIndexable(pyobj)))
	{
		match.conversion = &boxLongConversion;
		return match.type = JPMatch::_implicit;
	}

	if (JPPyFloat::checkConvertable(pyobj))
	{
		match.conversion = &boxDoubleConversion;
		return match.type = JPMatch::_implicit;
	}

	if (classConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
		return match.type;
	if (proxyConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
		return match.type;
	return NULL;
	JP_TRACE_OUT;
}
