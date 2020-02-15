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

JPStringClass::JPStringClass(jclass cls) : JPClass(cls)
// JPJni::s_StringClass)
{
}

JPStringClass::~JPStringClass()
{
}

JPPyObject JPStringClass::convertToPythonObject(jvalue val)
{
	JP_TRACE_IN("JPStringType::convertToPythonObject");

	if (val.l == NULL)
	{
		return JPPyObject::getNone();
	}

	if (JPEnv::getConvertStrings())
	{
		string str = JPJni::toStringUTF8((jstring) (val.l));
		return JPPyString::fromStringUTF8(str);
	}

	return PyJPValue_create(JPValue(this, val));
	JP_TRACE_OUT;
}

JPMatch::Type JPStringClass::canConvertToJava(PyObject* obj)
{
	JPJavaFrame frame;
	JP_TRACE_IN("JPStringType::canConvertToJava");
	ASSERT_NOT_NULL(obj);

	if (obj == NULL || JPPyObject::isNone(obj))
	{
		return JPMatch::_implicit;
	}

	JPValue* value = PyJPValue_getJavaSlot(obj);
	if (value != NULL)
	{
		if (value->getClass() == this)
		{
			return JPMatch::_exact;
		}

		if (frame.IsAssignableFrom(value->getJavaClass(), m_Class.get()))
		{
			return JPMatch::_implicit;
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
	JPJavaFrame frame;
	jvalue res;
	res.l = NULL;

	if (JPPyObject::isNone(obj))
	{
		return res;
	}

	// java.lang.string is already a global object
	JPValue* value = PyJPValue_getJavaSlot(obj);
	if (value != NULL)
	{
		if (value->getClass() == this || frame.IsAssignableFrom(value->getJavaClass(), m_Class.get()))
		{
			res.l = frame.NewLocalRef(value->getJavaObject());
			res.l = frame.keep(res.l);
			return res;
		}
		JP_RAISE(PyExc_TypeError, "Attempt to convert a non string java object");
	}

	// Otherwise convert the string
	if (JPPyString::check(obj))
	{
		string str = JPPyString::asStringUTF8(obj);
		jstring jstr = JPJni::fromStringUTF8(str);
		res.l = frame.keep(jstr);
		return res;
	}
	JP_RAISE(PyExc_TypeError, "Unable to convert to java string");
	return res;
	JP_TRACE_OUT;
}

JPValue JPStringClass::newInstance(JPJavaFrame& frame, JPPyObjectVector& args)
{
	JP_TRACE_IN("JPStringClass::newInstance");
	if (args.size() == 1 && JPPyString::check(args[0]))
	{
		// JNI has a short cut for constructing java.lang.String
		JP_TRACE("Direct");
		string str = JPPyString::asStringUTF8(args[0]);
		jvalue res;
		res.l = JPJni::fromStringUTF8(str);
		return JPValue(this, res);
	}
	return JPClass::newInstance(frame, args);
	JP_TRACE_OUT;
}
