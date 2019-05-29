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
		JPContext* superClass,
		JPContext* componentType,
		jint modifiers) : JPClass(context, cls, name, superClass, JPClassList(), modifiers)
{
	m_ComponentType = componentType;
}

JPArrayClass::~JPArrayClass()
{
}

JPMatch::Type JPArrayClass::canConvertToJava(PyObject* obj)
{
	JP_TRACE_IN("JPArrayClass::canConvertToJava");
	JPJavaFrame frame;

	if (JPPyObject::isNone(obj))
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

		if (frame.IsAssignableFrom(value->getJavaClass(), m_Class.get()))
		{
			return JPMatch::_implicit;
		}
		return JPMatch::_none;
	}

	if (JPPyString::check(obj) && m_ComponentType == m_Context->_char)
	{
		JP_TRACE("char[]");
		// Strings are also char[]
		return JPMatch::_implicit; // FIXME this should be JPMatch::_explicit under java rules.
	}

#if PY_MAJOR_VERSION >= 3 
	// Bytes are byte[]
	if (PyBytes_Check(obj) && m_ComponentType == m_Context->_byte)
	{
		return JPMatch::_implicit;
	}
#else
	// Bytes are byte[]
	if (PyString_Check(obj) && m_ComponentType == m_Context->_byte)
	{
		return JPMatch::_implicit;
	}
#endif

	//	if (JPPyString::checkBytes(o) && m_ComponentType == m_Context->_byte)
	//	{
	//		TRACE1("char[]");
	//		// Strings are also char[]
	//		return JPMatch::_implicit;
	//	}

	JPPySequence seq(JPPyRef::_use, obj);
	if (JPPyObject::isSequenceOfItems(obj))
	{
		JP_TRACE("Sequence");
		JPMatch::Type match = JPMatch::_implicit;
		jlong length = seq.size();
		for (jlong i = 0; i < length && match > JPMatch::_none; i++)
		{
			JPMatch::Type newMatch = m_ComponentType->canConvertToJava(seq[i].get());
			if (newMatch < match)
			{
				match = newMatch;
			}
		}
		return match;
	}

	return JPMatch::_none;
	JP_TRACE_OUT;
}

JPPyObject JPArrayClass::convertToPythonObject(jvalue val)
{
	JP_TRACE_IN("JPArrayClass::convertToPythonObject")
	return JPPythonEnv::newJavaObject(JPValue(this, val));
	JP_TRACE_OUT;
}

jvalue JPArrayClass::convertToJava(PyObject* obj)
{
	JP_TRACE_IN("JPArrayClass::convertToJava");
	JPJavaFrame frame;
	jvalue res;
	res.l = NULL;

	if (JPPyObject::isNone(obj))
	{
		return res;
	}

	JPValue* value = JPPythonEnv::getJavaValue(obj);
	if (value != NULL)
	{
		return *value;
	}

	if (JPPyString::check(obj)
			&& m_ComponentType == m_Context->_char)
	{
		JP_TRACE("char[]");

		// Convert to a string
		string str = JPPyString::asStringUTF8(obj);

		// Convert to new java string
		jstring jstr = m_Context->fromStringUTF8(str);

		// call toCharArray()
		jobject charArray = m_Context->_java_lang_String->stringToCharArray(jstr);
		res.l = frame.keep(charArray);
		return res;
	}

#if PY_MAJOR_VERSION >= 3 
	if (PyBytes_Check(obj) && m_ComponentType == m_Context->_byte)
	{
		Py_ssize_t size = 0;
		char *buffer = NULL;
		PyBytes_AsStringAndSize(obj, &buffer, &size); // internal reference
		jbyteArray byteArray = frame.NewByteArray(size);
		frame.SetByteArrayRegion(byteArray, 0, size, (jbyte*) buffer);
		res.l = frame.keep(byteArray);
		return res;
	}
#else
	if (PyString_Check(obj) && m_ComponentType == m_Context->_byte)
	{
		Py_ssize_t size = 0;
		char *buffer = NULL;
		PyString_AsStringAndSize(obj, &buffer, &size); // internal reference
		jbyteArray byteArray = frame.NewByteArray(size);
		frame.SetByteArrayRegion(byteArray, 0, size, (jbyte*) buffer);
		res.l = frame.keep(byteArray);
		return res;
	}
#endif

	if (JPPyObject::isSequenceOfItems(obj))
	{
		JP_TRACE("sequence");
		JPPySequence seq(JPPyRef::_use, obj);
		jsize length = (jsize) seq.size();

		jarray array = m_ComponentType->newArrayInstance(frame, (jsize) length);

		for (jsize i = 0; i < length; i++)
		{
			m_ComponentType->setArrayItem(frame, array, i, seq[i].get());
		}
		res.l = frame.keep(array);
		return res;
	}

	stringstream ss;
	ss << "Cannot convert value of type " << JPPyObject::getTypeName(obj)
			<< " to Java array type " << this->m_CanonicalName;
	JP_RAISE_TYPE_ERROR(ss.str());
	return res;
	JP_TRACE_OUT;
}

jvalue JPArrayClass::convertToJavaVector(JPPyObjectVector& refs, jsize start, jsize end)
{
	JPJavaFrame frame;
	JP_TRACE_IN("JPArrayClass::convertToJavaVector");
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
	JPJavaFrame frame;
	jvalue v;
	v.l = m_ComponentType->newArrayInstance(frame, length);
	return JPValue(this, v);
}
