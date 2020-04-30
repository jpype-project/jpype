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
#include "jpype.h"
#include "pyjp.h"
#include "jp_arrayclass.h"
#include "jp_context.h"
#include "jp_stringtype.h"

JPArrayClass::JPArrayClass(JPJavaFrame& frame,
		jclass cls,
		const string& name,
		JPClass* superClass,
		JPClass* componentType,
		jint modifiers)
: JPClass(frame, cls, name, superClass, JPClassList(), modifiers)
{
	m_ComponentType = componentType;
}

JPArrayClass::~JPArrayClass()
{
}

JPMatch::Type JPArrayClass::findJavaConversion(JPMatch &match)
{
	JP_TRACE_IN("JPArrayClass::getJavaConversion");
	if (nullConversion->matches(match, this)
			|| objectConversion->matches(match, this)
			|| charArrayConversion->matches(match, this)
			|| byteArrayConversion->matches(match, this)
			|| sequenceConversion->matches(match, this)
			)
		return match.type;
	JP_TRACE("None");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

JPPyObject JPArrayClass::convertToPythonObject(JPJavaFrame& frame, jvalue value, bool cast)
{
	JP_TRACE_IN("JPArrayClass::convertToPythonObject");
	if (!cast)
	{
		if (value.l == NULL)
			return JPPyObject::getNone();
	}
	JPPyObject wrapper = PyJPClass_create(frame, this);
	JPPyObject obj = PyJPArray_create(frame, (PyTypeObject*) wrapper.get(), JPValue(this, value));
	return obj;
	JP_TRACE_OUT;
}

jvalue JPArrayClass::convertToJavaVector(JPJavaFrame& frame, JPPyObjectVector& refs, jsize start, jsize end)
{
	JP_TRACE_IN("JPArrayClass::convertToJavaVector");
	jsize length = (jsize) (end - start);

	jarray array = m_ComponentType->newArrayInstance(frame, length);
	jvalue res;
	for (jsize i = start; i < end; i++)
	{
		m_ComponentType->setArrayItem(frame, array, i - start, refs[i]);
	}
	res.l = array;
	return res;
	JP_TRACE_OUT;
}

JPValue JPArrayClass::newInstance(JPJavaFrame& frame, int length)
{
	jvalue v;
	v.l = m_ComponentType->newArrayInstance(frame, length);
	return JPValue(this, v);
}
