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

class JPConversionCharArray : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame *frame, JPClass *cls, PyObject *pyobj) override
	{
		JPContext *context = frame->getContext();
		JP_TRACE("char[]");
		jvalue res;

		// Convert to a string
		string str = JPPyString::asStringUTF8(pyobj);

		// Convert to new java string
		jstring jstr = frame->fromStringUTF8(str);

		// call toCharArray()
		res.l = context->_java_lang_String->stringToCharArray(*frame, jstr);
		return res;
	}
} charArrayConversion;

class JPConversionByteArray : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame *frame2, JPClass *cls, PyObject *pyobj) override
	{
		JPJavaFrame frame(*frame2);
		jvalue res;
		Py_ssize_t size = 0;
		char *buffer = NULL;

		if (PyBytes_Check(pyobj))
		{
			PyBytes_AsStringAndSize(pyobj, &buffer, &size); // internal reference
		}
		jbyteArray byteArray = frame.NewByteArray((jsize) size);
		frame.SetByteArrayRegion(byteArray, 0, (jsize) size, (jbyte*) buffer);
		res.l = frame.keep(byteArray);
		return res;
	}
} byteArrayConversion;

class JPConversionSequence : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame *frame2, JPClass *cls, PyObject *pyobj) override
	{
		JPJavaFrame frame(*frame2);
		jvalue res;
		JPArrayClass *acls = (JPArrayClass *) cls;
		JP_TRACE("sequence");
		JPPySequence seq(JPPyRef::_use, pyobj);
		jsize length = (jsize) seq.size();

		jarray array = acls->getComponentType()->newArrayInstance(frame, (jsize) length);
		for (jsize i = 0; i < length; i++)
		{
			acls->getComponentType()->setArrayItem(frame, array, i, seq[i].get());
		}
		res.l = frame.keep(array);
		return res;
	}
} sequenceConversion;

JPMatch::Type JPArrayClass::getJavaConversion(JPJavaFrame *frame, JPMatch &match, PyObject *pyobj)
{
	JP_TRACE_IN("JPArrayClass::getJavaConversion");
	JPContext *context = NULL;
	if (frame != NULL)
		context = frame->getContext();

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

	if (JPPyString::check(pyobj) && m_ComponentType == context->_char)
	{
		JP_TRACE("String");
		match.conversion = &charArrayConversion;
		return match.type = JPMatch::_implicit;
	}

	// Bytes are byte[]
	if (context != NULL && PyBytes_Check(pyobj) && m_ComponentType == context->_byte)
	{
		JP_TRACE("Byte");
		match.conversion = &byteArrayConversion;
		return match.type = JPMatch::_implicit;
	}

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

JPPyObject JPArrayClass::convertToPythonObject(JPJavaFrame& frame, jvalue val)
{
	JP_TRACE_IN("JPArrayClass::convertToPythonObject")
	return JPPythonEnv::newJavaObject(JPValue(this, val));
	JP_TRACE_OUT;
}

jvalue JPArrayClass::convertToJavaVector(JPJavaFrame& frame, JPPyObjectVector& refs, jsize start, jsize end)
{
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

JPValue JPArrayClass::newInstance(JPPyObjectVector& args)
{
	JPJavaFrame frame(getContext());
	if (args.size() != 1)
		JP_RAISE_TYPE_ERROR("Arrays require one argument");

	if (PySequence_Check(args[0]) == 1)
	{
		Py_ssize_t sz = PySequence_Size(args[0]);
		jvalue v;
		v.l = m_ComponentType->newArrayInstance(frame, (jsize) sz);
		JPArray array(this, (jarray) v.l);
		array.setRange(0, (jsize) sz, args[0]);
		return JPValue(this, v);
	}

	if (PyIndex_Check(args[0]))
	{
		Py_ssize_t sz = PyNumber_AsSsize_t(args[0], NULL);
		if (sz < 0 )
			JP_RAISE_VALUE_ERROR("Invalid size");
		jvalue v;
		v.l = m_ComponentType->newArrayInstance(frame, (int) sz);
		return JPValue(this, v);
	}

	JP_RAISE_TYPE_ERROR("Arrays require int or sequence parameters");
}
