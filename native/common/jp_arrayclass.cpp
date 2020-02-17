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
#include "jp_arrayclass.h"

JPArrayClass::JPArrayClass(jclass c) : JPClass(c)
{
	m_ComponentType = JPTypeManager::findClass(JPJni::getComponentType(c));
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

	if (JPPyString::check(obj) && m_ComponentType == JPTypeManager::_char)
	{
		JP_TRACE("char[]");
		// Strings are also char[]
		return JPMatch::_implicit; // FIXME this should be JPMatch::_explicit under java rules.
	}

#if PY_MAJOR_VERSION >= 3
	// Bytes are byte[]
	if (PyBytes_Check(obj) && m_ComponentType == JPTypeManager::_byte)
	{
		return JPMatch::_implicit;
	}
#else
	// Bytes are byte[]
	if (PyString_Check(obj) && m_ComponentType == JPTypeManager::_byte)
	{
		return JPMatch::_implicit;
	}
#endif

	//	if (JPPyString::checkBytes(o) && m_ComponentType == JPTypeManager::_byte)
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
	JP_TRACE_IN("JPArrayClass::convertToPythonObject");
	return PyJPValue_create(JPValue(this, val));
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

	JPValue* value = PyJPValue_getJavaSlot(obj);
	if (value != NULL)
	{
		// Check if it is a slice, because slices must be cloned
		if (PyObject_IsInstance(obj, (PyObject*) PyJPArray_Type))
		{
			PyJPArray* array = (PyJPArray*) obj;
			if (array->m_Array->isSlice())
			{
				res.l = frame.keep(array->m_Array->clone(frame, obj));
				return res;
			}
		}
		return *value;
	}

	if (JPPyString::check(obj)
			&& m_ComponentType == JPTypeManager::_char)
	{
		JP_TRACE("char[]");

		// Convert to a string
		string str = JPPyString::asStringUTF8(obj);

		// Convert to new java string
		jstring jstr = JPJni::fromStringUTF8(str);

		// call toCharArray()
		jobject charArray = JPJni::stringToCharArray(jstr);
		res.l = frame.keep(charArray);
		return res;
	}

	if (PyBytes_Check(obj) && m_ComponentType == JPTypeManager::_byte)
	{
		Py_ssize_t size = 0;
		char *buffer = NULL;
		PyBytes_AsStringAndSize(obj, &buffer, &size); // internal reference
		jbyteArray byteArray = frame.NewByteArray(size);
		frame.SetByteArrayRegion(byteArray, 0, size, (jbyte*) buffer);
		res.l = frame.keep(byteArray);
		return res;
	}

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
	JP_RAISE(PyExc_TypeError, ss.str());
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

JPValue JPArrayClass::newInstance(JPJavaFrame& frame, int length)
{
	jvalue v;
	v.l = frame.keep(m_ComponentType->newArrayInstance(frame, length));
	return JPValue(this, v);
}

JPValue JPArrayClass::newInstance(JPJavaFrame& frame, JPPyObjectVector& args)
{
	JP_TRACE_IN("JPArrayClass::newInstance");
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
