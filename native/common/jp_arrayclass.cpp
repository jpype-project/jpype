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
#include <Python.h> // FIXME work on bytes, remove when complete
#include <jpype.h>

#include "jp_context.h"

JPArrayClass::JPArrayClass(JPContext* context,
		jclass cls,
		const string& name,
		JPClass* superClass,
		JPClass* componentType,
		jint modifiers)
: JPClass(context, cls, name, superClass, JPClassList(), modifiers)
{
	m_ComponentType = componentType;
}

JPArrayClass::~JPArrayClass()
{
}

class JPConversionCharArray : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		JP_TRACE("char[]");
		jvalue res;

		// Convert to a string
		string str = JPPyString::asStringUTF8(pyobj);

		// Convert to new java string
		jstring jstr = frame.getContext()->fromStringUTF8(str);

		// call toCharArray()
		jobject charArray = frame.getContext()->_java_lang_String->stringToCharArray(jstr);
		res.l = charArray;
		return res;
	}
} charArrayConversion;

class JPConversionByteArray : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		jvalue res;
		Py_ssize_t size = 0;
		char *buffer = NULL;

#if PY_MAJOR_VERSION >= 3
		if (PyBytes_Check(pyobj))
		{
			PyBytes_AsStringAndSize(pyobj, &buffer, &size); // internal reference
		}
#else
		if (PyString_Check(pyobj))
		{
			PyString_AsStringAndSize(pyobj, &buffer, &size); // internal reference
		}
#endif
		jbyteArray byteArray = frame.NewByteArray((jsize)size);
		frame.SetByteArrayRegion(byteArray, 0, (jsize) size, (jbyte*) buffer);
		res.l = frame.keep(byteArray);
		return res;
	}
} byteArrayConversion;

class JPConversionSequence : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame& frame, JPClass* cls, PyObject* pyobj) override
	{
		jvalue res;
		JPArrayClass *acls = (JPArrayClass *)cls;
		JP_TRACE("sequence");
		JPPySequence seq(JPPyRef::_use, pyobj);
		jsize length = (jsize) seq.size();

		jarray array = acls->m_ComponentType->newArrayInstance(frame, (jsize) length);
		for (jsize i = 0; i < length; i++)
		{
			acls->m_ComponentType->setArrayItem(frame, array, i, seq[i].get());
		}
		res.l = frame.keep(array);
		return res;
	}
} sequenceConversion;

JPMatch::Type JPArrayClass::getJavaConversion(JPJavaFrame& frame, JPMatch& match, PyObject* pyobj)
{
	JP_TRACE_IN("JPArrayClass::getJavaConversion");
	if (nullConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
	{
		JP_TRACE("Null", match.type);
		return match.type;
	}
	if (objectConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
	{
		JP_TRACE("Object", match.type);
		return match.type;
	}

	if (JPPyString::check(pyobj) && m_ComponentType == m_Context->_char)
	{
		JP_TRACE("String");
		match.conversion = &charArrayConversion;
		return match.type = JPMatch::_implicit;
	}

#if PY_MAJOR_VERSION >= 3
	// Bytes are byte[]
	if (PyBytes_Check(pyobj) && m_ComponentType == m_Context->_byte)
	{
		JP_TRACE("Byte");
		match.conversion = &byteArrayConversion;
		return match.type = JPMatch::_implicit;
	}
#else
	// Bytes are byte[]
	if (PyString_Check(pyobj) && m_ComponentType == m_Context->_byte)
	{
		JP_TRACE("Byte");
		match.conversion = &byteArrayConversion;
		return match.type = JPMatch::_implicit;
	}
#endif

	if (JPPyObject::isSequenceOfItems(pyobj))
	{
		JP_TRACE("Sequence");
		JPPySequence seq(JPPyRef::_use, pyobj);
		jlong length = seq.size();
		match.type = JPMatch::_implicit;
		JPMatch imatch;
		for (jlong i = 0; i < length && match.type > JPMatch::_none; i++)
		{
			m_ComponentType->getJavaConversion(frame, imatch, seq[i].get());
			if (imatch.type < match.type)
			{
				match.type = imatch.type;
			}
		}
		match.conversion = &sequenceConversion;
		return match.type;
	}

	JP_TRACE("None");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

JPPyObject JPArrayClass::convertToPythonObject(jvalue val)
{
	JP_TRACE_IN("JPArrayClass::convertToPythonObject")
	return JPPythonEnv::newJavaObject(JPValue(this, val));
	JP_TRACE_OUT;
}

jvalue JPArrayClass::convertToJavaVector(JPPyObjectVector& refs, jsize start, jsize end)
{
	JPJavaFrame frame(m_Context);
	JP_TRACE_IN("JPArrayClass::convertToJavaVector");
	JP_TRACE("component type", m_ComponentType->getCanonicalName());
	jsize length = (jsize) (end - start);

	jarray array = m_ComponentType->newArrayInstance(frame, length);
	jvalue res;
	for (jsize i = start; i < end; i++)
	{
		m_ComponentType->setArrayItem(frame, array, i - start, refs[i]);
	}
	res.l = frame.keep(array);
	return res;
	JP_TRACE_OUT;
}

JPValue JPArrayClass::newInstance(int length)
{
	JPJavaFrame frame(m_Context);
	jvalue v;
	v.l = m_ComponentType->newArrayInstance(frame, length);
	return JPValue(this, v);
}
