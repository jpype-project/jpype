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

JPClassType::JPClassType(JPContext* context,
			 jclass clss,
			 const string& name,
			 JPClass* super,
			 JPClassList& interfaces,
			 jint modifiers)
: JPClass(context, clss, name, super, interfaces, modifiers)
{
}

JPClassType::~JPClassType()
{
}

JPMatch::Type JPClassType::canConvertToJava(PyObject* pyobj)
{
	JP_TRACE_IN("JPObjectType::convertToJava");
	if (JPPyObject::isNone(pyobj))
		return JPMatch::_implicit;

	JPValue* value = JPPythonEnv::getJavaValue(pyobj);
	if (value != NULL)
	{
		if (value->getClass() == this)
			return JPMatch::_exact;
		return JPMatch::_none;
	}

	JPClass* cls = JPPythonEnv::getJavaClass(pyobj);
	if (cls != NULL)
		return JPMatch::_exact;

	return JPMatch::_none;
	JP_TRACE_OUT;
}

jvalue JPClassType::convertToJava(PyObject* pyobj)
{
	JP_TRACE_IN("JPObjectType::convertToJava");
	JP_TRACE(JPPyObject::getTypeName(pyobj));

	jvalue res;
	JPJavaFrame frame(m_Context);

	res.l = NULL;

	// assume it is convertible;
	if (JPPyObject::isNone(pyobj))
	{
		return res;
	}

	JPValue* value = JPPythonEnv::getJavaValue(pyobj);
	if (value != NULL)
	{
		if (value->getClass() == this)
		{
			res.l = frame.NewLocalRef(value->getValue().l);
			res.l = frame.keep(res.l);
			return res;
		}
		JP_RAISE_TYPE_ERROR("Unable to convert to java class");
	}

	JPClass* cls = JPPythonEnv::getJavaClass(pyobj);
	if (cls != NULL)
	{
		res.l = frame.NewLocalRef(cls->getJavaClass());
		res.l = frame.keep(res.l);
		return res;
	}
	JP_RAISE_TYPE_ERROR("Unable to convert to java class");
	return res;
	JP_TRACE_OUT;
}
