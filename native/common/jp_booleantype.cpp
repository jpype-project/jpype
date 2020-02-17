/*****************************************************************************
   Copyright 2004-2008 Steve Ménard

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
#include "jp_primitive_accessor.h"
#include "jp_booleantype.h"

JPBooleanType::JPBooleanType() : JPPrimitiveType(JPTypeManager::_java_lang_Boolean)
{
}

JPBooleanType::~JPBooleanType()
{
}

bool JPBooleanType::isSubTypeOf(JPClass* other) const
{
	return other == JPTypeManager::_boolean;
}

JPPyObject JPBooleanType::convertToPythonObject(jvalue val)
{
	return JPPyObject(JPPyRef::_claim, PyBool_FromLong(val.z ? 1 : 0));
}

JPValue JPBooleanType::getValueFromObject(jobject obj)
{
	jvalue v;
	field(v) = JPJni::booleanValue(obj);
	return JPValue(this, v);
}

JPMatch::Type JPBooleanType::canConvertToJava(PyObject* obj)
{
	ASSERT_NOT_NULL(obj);
	if (JPPyObject::isNone(obj))
	{
		return JPMatch::_none;
	}

	// Exact Python match
	if (PyBool_Check(obj))
	{
		return JPMatch::_exact;
	}

	JPValue* value = PyJPValue_getJavaSlot(obj);
	if (value != NULL)
	{
		// Wrapper
		if (value->getClass() == this)
		{
			return JPMatch::_exact;
		}
		// Implicit conversion from boxed to primitive (JLS 5.1.8)
		if (value->getClass() == this->m_BoxedClass)
		{
			return JPMatch::_implicit;
		}
		return JPMatch::_none;
	}

	if (PyBool_Check(obj))
	{
		return JPMatch::_exact;
	}

	// Java does not consider ints to be bools, but we may need
	// it when returning from a proxy.
	if (JPPyLong::checkConvertable(obj))
	{
		// If it implements logical operations it is an integer type
		if (JPPyLong::checkIndexable(obj))
			return JPMatch::_implicit;
			// Otherwise it may be a float or list.
		else
			return JPMatch::_explicit;
	}

	return JPMatch::_none;
}

jvalue JPBooleanType::convertToJava(PyObject* obj)
{
	JP_TRACE_IN("JPBooleanType::convertToJava");
	jvalue res;

	JPValue* value = PyJPValue_getJavaSlot(obj);
	if (value != NULL)
	{
		if (value->getClass() == this)
			return *value;
		if (value->getClass() == m_BoxedClass)
		{
			if (value->getJavaObject() == NULL)
				JP_RAISE(PyExc_RuntimeError, "Null pointer in implicit conversion from boxed.");
			return getValueFromObject(value->getJavaObject());
		}
	} else if (JPPyLong::checkConvertable(obj))
	{
		res.z = (jboolean) JPPyLong::asLong(obj) != 0;
	}
	return res;
	JP_TRACE_OUT;
}

jarray JPBooleanType::newArrayInstance(JPJavaFrame& frame, jsize sz)
{
	return frame.NewBooleanArray(sz);
}

JPPyObject JPBooleanType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticBooleanField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPBooleanType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetBooleanField(c, fid);
	return convertToPythonObject(v);
}

JPPyObject JPBooleanType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticBooleanMethodA(claz, mth, val);
	}
	return convertToPythonObject(v);
}

JPPyObject JPBooleanType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == NULL)
			field(v) = frame.CallBooleanMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualBooleanMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(v);
}

void JPBooleanType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticBooleanField(c, fid, val);
}

void JPBooleanType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetBooleanField(c, fid, val);
}

#define HAVE_PTR(suboffsets, dim) (suboffsets && suboffsets[dim] >= 0)
#define ADJUST_PTR(ptr, suboffsets, dim) \
    (HAVE_PTR(suboffsets, dim) ? *((char**)ptr) + suboffsets[dim] : ptr)

void JPBooleanType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPBooleanType::setArrayRange");
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetBooleanArrayElements, &JPJavaFrame::ReleaseBooleanArrayElements);

	type_t* val = accessor.get();
	// First check if assigning sequence supports buffer API
	if (PyObject_CheckBuffer(sequence))
	{
		JPPyBuffer buffer(sequence, PyBUF_FULL_RO);
		if (buffer.valid())
		{
			Py_buffer& view = buffer.getView();
			if (view.ndim != 1)
				JP_RAISE(PyExc_TypeError, "buffer dims incorrect");
			Py_ssize_t vshape = view.shape[0];
			Py_ssize_t vstep = view.strides[0];
			if (vshape != length)
				JP_RAISE(PyExc_ValueError, "mismatched size");

			char* memory = (char*) view.buf;
			if (view.suboffsets && view.suboffsets[0] >= 0)
				memory = *((char**) memory) + view.suboffsets[0];
			jsize index = start;
			jconverter conv = getConverter(view.format, view.itemsize, "z");
			for (Py_ssize_t i = 0; i < length; ++i, index += step)
			{
				jvalue r = conv(memory);
				val[index] = r.z;
				memory += vstep;
			}
			accessor.commit();
			return;
		} else
		{
			PyErr_Clear();
		}
	}

	// Use sequence API
	JPPySequence seq(JPPyRef::_use, sequence);
	jsize index = start;
	for (Py_ssize_t i = 0; i < length; ++i, index += step)
	{
		val[index] = PyObject_IsTrue(seq[i].get());
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPBooleanType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetBooleanArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(v);
}

void JPBooleanType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	array_t array = (array_t) a;
	type_t val = field(convertToJava(obj));
	frame.SetBooleanArrayRegion(array, ndx, 1, &val);
}

void JPBooleanType::getView(JPArrayView& view)
{
	JPJavaFrame frame;
	view.memory = (void*) frame.GetBooleanArrayElements(
			(jbooleanArray) view.array->getJava(), &view.isCopy);
	view.buffer.format = "?";
	view.buffer.itemsize = sizeof (jboolean);
}

void JPBooleanType::releaseView(JPArrayView& view, bool complete)
{
	JPJavaFrame frame;
	if (complete)
	{
		frame.ReleaseBooleanArrayElements((jbooleanArray) view.array->getJava(),
				(jboolean*) view.memory, view.buffer.readonly ? JNI_ABORT : 0);
	}
}
