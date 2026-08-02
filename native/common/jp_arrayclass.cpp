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

	// componentType is always fully constructed already (array classes are
	// built bottom-up), so this can be resolved once here rather than with
	// a dynamic_cast walk on every JPConversionMultiArrayBuffer::matches().
	auto *actl = dynamic_cast<JPArrayClass*>(componentType);
	if (actl != nullptr)
	{
		m_MultiArrayDepth = actl->m_MultiArrayDepth + 1;
		m_MultiArrayLeaf = actl->m_MultiArrayLeaf;
	} else
	{
		m_MultiArrayDepth = 1;
		m_MultiArrayLeaf = dynamic_cast<JPPrimitiveType*>(componentType);
	}
}

JPArrayClass::~JPArrayClass()
= default;

JPMatch::Type JPArrayClass::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPArrayClass::findJavaConversion");
	// m_MultiArrayDepth < 2 covers every existing 1D array class (the vast
	// majority of calls into this function) -- skip even calling into
	// multiArrayBufferConversion for those so this addition costs nothing
	// beyond one integer compare on the paths it doesn't apply to.
	if (nullConversion->matches(this, match)
			|| objectConversion->matches(this, match)
			|| charArrayConversion->matches(this, match)
			|| byteArrayConversion->matches(this, match)
			|| bufferConversion->matches(this, match)
			|| (m_MultiArrayDepth >= 2 && multiArrayBufferConversion->matches(this, match))
			|| sequenceConversion->matches(this, match)
			|| hintsConversion->matches(this, match)
			)
		return match.type;
	JP_TRACE("None");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPArrayClass::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	objectConversion->getInfo(this, info);
	charArrayConversion->getInfo(this, info);
	byteArrayConversion->getInfo(this, info);
	sequenceConversion->getInfo(this, info);
	hintsConversion->getInfo(this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}

JPPyObject JPArrayClass::convertToPythonObject(JPJavaFrame& frame, jvalue value, bool cast)
{
	JP_TRACE_IN("JPArrayClass::convertToPythonObject");
	if (!cast)
	{
		if (value.l == nullptr)
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
	auto length = (jsize) (end - start);

	jarray array = m_ComponentType->newArrayOf(frame, length);
	jvalue res;
	for (jsize i = start; i < end; i++)
	{
		m_ComponentType->setArrayItem(frame, array, i - start, refs[i]);
	}
	res.l = array;
	return res;
	JP_TRACE_OUT;
}

JPValue JPArrayClass::newArray(JPJavaFrame& frame, int length)
{
	jvalue v;
	v.l = m_ComponentType->newArrayOf(frame, length);
	return JPValue(this, v);
}
