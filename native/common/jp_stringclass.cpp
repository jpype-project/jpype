/*****************************************************************************
   Copyright 2004-2008 Steve Ménard

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

JPStringClass::JPStringClass(JPContext* context,
		jclass clss,
		const string& name,
		JPClass* super,
		JPClassList& interfaces,
		jint modifiers)
: JPClass(context, clss, name, super, interfaces, modifiers)
{
	JPJavaFrame frame(context);
	m_String_ToCharArrayID = frame.GetMethodID(clss, "toCharArray", "()[C");
}

JPStringClass::~JPStringClass()
{
}

jobject JPStringClass::stringToCharArray(jstring str)
{
	JPJavaFrame frame(m_Context);
	jobject res = frame.CallObjectMethod(str, m_String_ToCharArrayID);
	return frame.keep(res);
}

JPPyObject JPStringClass::convertToPythonObject(jvalue val)
{
	JP_TRACE_IN("JPStringType::asHostObject");

	if (val.l == NULL)
	{
		return JPPyObject::getNone();
	}

	return JPPythonEnv::newJavaObject(JPValue(this, val));
	JP_TRACE_OUT;
}

JPMatch::Type JPStringClass::canConvertToJava(PyObject* obj)
{
	JP_TRACE_IN("JPStringType::canConvertToJava");
	ASSERT_NOT_NULL(obj);

	if (obj == NULL || JPPyObject::isNone(obj))
	{
		return JPMatch::_implicit;
	}

	JPValue* value = JPPythonEnv::getJavaValue(obj);
	if (value != NULL)
	{
		if (value->getClass() == this)
		{
			return JPMatch::_exact;
		}
		return JPMatch::_none;
	}

	if (JPPyString::check(obj))
	{
		return JPMatch::_exact;
	}

	return JPMatch::_none;
	JP_TRACE_OUT;
}

jvalue JPStringClass::convertToJava(PyObject* obj)
{
	JP_TRACE_IN("JPStringType::convertToJava");
	JPJavaFrame frame(m_Context);
	jvalue res;
	res.l = NULL;

	if (JPPyObject::isNone(obj))
	{
		return res;
	}

	// java.lang.string is already a global object 
	JPValue* value = JPPythonEnv::getJavaValue(obj);
	if (value != NULL)
	{
		if (value->getClass() == this)
		{
			res.l = frame.NewLocalRef(value->getJavaObject());
			res.l = frame.keep(res.l);
			return res;
		}
		JP_RAISE_TYPE_ERROR("Attempt to convert a non string java object");
	}

	// Otherwise convert the string
	if (JPPyString::check(obj))
	{
		string str = JPPyString::asStringUTF8(obj);
		jstring jstr = m_Context->fromStringUTF8(str);
		res.l = frame.keep(jstr);
		return res;
	}
	JP_RAISE_TYPE_ERROR("Unable to convert to java string");
	return res;
	JP_TRACE_OUT;
}

JPValue JPStringClass::newInstance(JPPyObjectVector& args)
{
	JP_TRACE_IN("JPStringClass::newInstance");
	if (args.size() == 1 && JPPyString::check(args[0]))
	{
		// JNI has a short cut for constructing java.lang.String
		JP_TRACE("Direct");
		string str = JPPyString::asStringUTF8(args[0]);
		jvalue res;
		res.l = m_Context->fromStringUTF8(str);
		return JPValue(this, res);
	}
	return JPClass::newInstance(args);
	JP_TRACE_OUT;
}
