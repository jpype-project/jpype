/*****************************************************************************
   Copyright 2004 Steve Ménard

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
#include <jp_boxedclasses.h>

JPBoxedClass::JPBoxedClass(jclass c) : JPClass(c)
{
}

JPBoxedClass::~JPBoxedClass()
{
}

void JPBoxedClass::setPrimitiveType(JPPrimitiveType* primitiveType)
{
	m_PrimitiveType = primitiveType;
}

jvalue JPBoxedClass::convertToJava(PyObject* obj)
{
	JP_TRACE_IN("JPBoxedClass::convertToJava");
	JPJavaFrame frame;
	jvalue res;

	res.l = NULL;

	// assume it is convertible;
	if (JPPyObject::isNone(obj))
	{
		res.l = NULL;
		return res;
	}

	JPValue* value = JPPythonEnv::getJavaValue(obj);
	if (value != NULL && value->getClass() == this)
	{
		res.l = value->getJavaObject();
		return res;
	}

	JPProxy* proxy = JPPythonEnv::getJavaProxy(obj);
	if (proxy != NULL)
	{
		res.l = frame.keep(proxy->getProxy());
		return res;
	}

	// Call a constructor using the object
	JPPyObjectVector args(obj, NULL);
	JPValue pobj = newInstance(args);
	res.l = frame.keep(pobj.getJavaObject());
	return res;
	JP_TRACE_OUT;
}

JPMatch::Type JPBoxedClass::canConvertToJava(PyObject* pyobj)
{
	JP_TRACE_IN("JPBoxedClass::canConvertToJava");
	JPMatch::Type base = JPClass::canConvertToJava(pyobj);
	if (base == JPMatch::_none && this->m_PrimitiveType->canConvertToJava(pyobj))
		return JPMatch::_explicit;
	return base;
	JP_TRACE_OUT;
}

// Specializations for each of the boxed types.
// This sets up the table of conversions that we allow

//============================================================

jclass findClass(const string& str)
{
	JPJavaFrame frame;
	return (jclass) frame.keep((jobject) frame.FindClass(str));
}

//============================================================

JPBoxedVoidClass::JPBoxedVoidClass()
: JPBoxedClass(findClass("java/lang/Void"))
{
}

JPBoxedVoidClass::~JPBoxedVoidClass()
{
}

//============================================================

JPBoxedBooleanClass::JPBoxedBooleanClass()
: JPBoxedClass(findClass("java/lang/Boolean"))
{
}

JPBoxedBooleanClass::~JPBoxedBooleanClass()
{
}

//============================================================

JPBoxedByteClass::JPBoxedByteClass()
: JPBoxedClass(findClass("java/lang/Byte"))
{
}

JPBoxedByteClass::~JPBoxedByteClass()
{
}


//============================================================

JPBoxedCharacterClass::JPBoxedCharacterClass()
: JPBoxedClass(findClass("java/lang/Character"))
{
}

JPBoxedCharacterClass::~JPBoxedCharacterClass()
{
}


//============================================================

JPBoxedShortClass::JPBoxedShortClass()
: JPBoxedClass(findClass("java/lang/Short"))
{
}

JPBoxedShortClass::~JPBoxedShortClass()
{
}

//============================================================

JPBoxedIntegerClass::JPBoxedIntegerClass()
: JPBoxedClass(findClass("java/lang/Integer"))
{
}

JPBoxedIntegerClass::~JPBoxedIntegerClass()
{
}

//============================================================

JPBoxedLongClass::JPBoxedLongClass()
: JPBoxedClass(findClass("java/lang/Long"))
{
}

JPBoxedLongClass::~JPBoxedLongClass()
{
}

//============================================================

JPBoxedFloatClass::JPBoxedFloatClass()
: JPBoxedClass(findClass("java/lang/Float"))
{
}

JPBoxedFloatClass::~JPBoxedFloatClass()
{
}

//============================================================

JPBoxedDoubleClass::JPBoxedDoubleClass()
: JPBoxedClass(findClass("java/lang/Double"))
{
}

JPBoxedDoubleClass::~JPBoxedDoubleClass()
{
}
