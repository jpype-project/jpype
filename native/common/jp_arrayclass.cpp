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

JPMatch::Type JPArrayClass::getJavaConversion(JPJavaFrame *frame, JPMatch &match, PyObject *pyobj)
{
	JP_TRACE_IN("JPArrayClass::getJavaConversion");
	JPContext *context = NULL;
	if (frame != NULL)
		context = frame->getContext();

	if (nullConversion->matches(match, frame, this, pyobj) != JPMatch::_none
			|| objectConversion->matches(match, frame, this, pyobj) != JPMatch::_none
			|| charArrayConversion->matches(match, frame, this, pyobj) != JPMatch::_none
			|| byteArrayConversion->matches(match, frame, this, pyobj) != JPMatch::_none)
		return match.type;

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
		match.conversion = sequenceConversion;
		return match.type;
	}

	JP_TRACE("None");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

JPPyObject JPArrayClass::convertToPythonObject(JPJavaFrame& frame, jvalue val)
{
	JP_TRACE_IN("JPArrayClass::convertToPythonObject");
	return PyJPValue_create(frame, JPValue(this, val));
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
	res.l = frame.keep(array);
	return res;
	JP_TRACE_OUT;
}

JPValue JPArrayClass::newInstance(JPJavaFrame& frame, int length)
{
	jvalue v;
	v.l = frame.keep(m_ComponentType->newArrayInstance(frame, length));
	return JPValue(this, v);
}

JPValue JPArrayClass::newInstance(JPJavaFrame& frame, JPPyObjectVector& args)
{
	JP_TRACE_IN("JPArrayClass::newInstance", this);
	if (args.size() != 1)
		JP_RAISE(PyExc_TypeError, "Arrays require one argument");

	if (PySequence_Check(args[0]) == 1)
	{
		JP_TRACE("Sequence");
		Py_ssize_t sz = PySequence_Size(args[0]);
		jobject inst = m_ComponentType->newArrayInstance(frame, (jsize) sz);
		JPArray array(JPValue(this,  inst));
		array.setRange(0, (jsize) sz, 1, args[0]);
		return JPValue(this, inst);
	}

	if (PyIndex_Check(args[0]))
	{
		JP_TRACE("Index");
		Py_ssize_t sz = PyNumber_AsSsize_t(args[0], NULL);
		if (sz < 0 )
			JP_RAISE(PyExc_ValueError, "Invalid size");
		return JPValue(this, m_ComponentType->newArrayInstance(frame, (int) sz));
	}

	JP_RAISE(PyExc_TypeError, "Arrays require int or sequence parameters");
	JP_TRACE_OUT;
}
