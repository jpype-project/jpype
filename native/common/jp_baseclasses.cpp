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

JPObjectBaseClass::JPObjectBaseClass() : JPClass(JPJni::s_ObjectClass)
{
}

JPObjectBaseClass::~JPObjectBaseClass()
{
}

JPMatch::Type JPObjectBaseClass::canConvertToJava(PyObject* pyobj)
{
	// Implicit rules for java.lang.Object
	JP_TRACE_IN("JPObjectBaseClass::canConvertToJava");
	if (JPPyObject::isNone(pyobj))
	{
		return JPMatch::_implicit;
	}

	// arrays are objects
	JPValue *value = JPPythonEnv::getJavaValue(pyobj);
	if (value != NULL)
	{
		if (value->getClass() == this)
			return JPMatch::_exact;

		JP_TRACE("From jvalue");
		return JPMatch::_implicit;
	}

	// Strings are objects too
	if (JPPyString::check(pyobj))
	{
		JP_TRACE("From string");
		return JPMatch::_implicit;
	}

	// Class are objects too
	JPClass* cls = JPPythonEnv::getJavaClass(pyobj);
	if (cls != NULL)
	{
		JP_TRACE("implicit array class");
		return JPMatch::_implicit;
	}

	// Let'a allow primitives (int, long, float and boolean) to convert implicitly too ...
	if (JPPyFloat::checkConvertable(pyobj))
	{
		JP_TRACE("implicit float");
		return JPMatch::_implicit;
	}

	if (JPPyLong::checkConvertable(pyobj))
	{
		JP_TRACE("implicit long");
		return JPMatch::_implicit;
	}

	JPProxy* proxy = JPPythonEnv::getJavaProxy(pyobj);
	if (proxy != NULL)
	{
		JP_TRACE("implicit python proxy");
		return JPMatch::_implicit;
	}

	return JPMatch::_none;
	JP_TRACE_OUT;
}

// java.lang.Object can be converted to from all object classes, 
// all primitive types (via boxing), strings, arrays, and python bridge classes

jvalue JPObjectBaseClass::convertToJava(PyObject* pyobj)
{
	JP_TRACE_IN("JPObjectBaseClass::convertToJava");
	JPJavaFrame frame;
	jvalue res;
	res.l = NULL;

	// assume it is convertible;
	if (JPPyObject::isNone(pyobj))
	{
		return res;
	}

	JPValue *value = JPPythonEnv::getJavaValue(pyobj);
	if (value != NULL)
	{
		if (dynamic_cast<JPPrimitiveType*> (value->getClass()) == NULL)
		{
			res.l = frame.NewLocalRef(value->getJavaObject());
			res.l = frame.keep(res.l);
			return res;
		}
		else
		{
			// Okay we need to box it.
			JPPrimitiveType* type = (JPPrimitiveType*) (value->getClass());
			res = type->getBoxedClass()->convertToJava(pyobj);
			res.l = frame.keep(res.l);
			return res;
		}
	}

	if (JPPyString::check(pyobj))
	{
		res = JPTypeManager::_java_lang_String->convertToJava(pyobj);
		res.l = frame.keep(res.l);
		return res;
	}

	if (JPPyBool::check(pyobj))
	{
		res = JPTypeManager::_boolean->getBoxedClass()->convertToJava(pyobj);
		res.l = frame.keep(res.l);
		return res;
	}

	if (JPPyFloat::check(pyobj))
	{
		res = JPTypeManager::_double->getBoxedClass()->convertToJava(pyobj);
		res.l = frame.keep(res.l);
		return res;
	}

	if (JPPyLong::check(pyobj))
	{
		res = JPTypeManager::_long->getBoxedClass()->convertToJava(pyobj);
		res.l = frame.keep(res.l);
		return res;
	}

	// It is only an integer type if it can be used as a slice PEP-357
	if (JPPyLong::checkConvertable(pyobj) && JPPyLong::checkIndexable(pyobj))
	{
		res = JPTypeManager::_long->getBoxedClass()->convertToJava(pyobj);
		res.l = frame.keep(res.l);
		return res;
	}

	// Okay so if it does not have bit operations we will go to float
	if (JPPyFloat::checkConvertable(pyobj))
	{
		res = JPTypeManager::_double->getBoxedClass()->convertToJava(pyobj);
		res.l = frame.keep(res.l);
		return res;
	}

	JPClass* cls = JPPythonEnv::getJavaClass(pyobj);
	if (cls != NULL)
	{
		res.l = frame.NewLocalRef(cls->getJavaClass());
		res.l = frame.keep(res.l);
		return res;
	}

	JPProxy* proxy = JPPythonEnv::getJavaProxy(pyobj);
	if (proxy != NULL)
	{
		res.l = frame.keep(proxy->getProxy());
		return res;
	}

	JP_RAISE_TYPE_ERROR("Unable to convert to object");
	return res;
	JP_TRACE_OUT;
}

//=======================================================

JPClassBaseClass::JPClassBaseClass() : JPClass(JPJni::s_ClassClass)
{
}

JPClassBaseClass::~JPClassBaseClass()
{
}

JPMatch::Type JPClassBaseClass::canConvertToJava(PyObject* pyobj)
{
	JP_TRACE_IN("JPClassBaseClass::convertToJava");
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

jvalue JPClassBaseClass::convertToJava(PyObject* pyobj)
{
	JP_TRACE_IN("JPClassBaseClass::convertToJava");
	JP_TRACE(JPPyObject::getTypeName(pyobj));

	jvalue res;
	JPJavaFrame frame;

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
