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
#include "jp_voidtype.h"

JPVoidType::JPVoidType()
: JPPrimitiveType("void")
{
}

JPVoidType::~JPVoidType()
{
}

JPPyObject JPVoidType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	{
		JPPyCallRelease call;
		frame.CallStaticVoidMethodA(claz, mth, val);
	}
	return JPPyObject::getNone();
}

JPPyObject JPVoidType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	{
		JPPyCallRelease call;
		if (clazz == NULL)
			frame.CallVoidMethodA(obj, mth, val);
		else
			frame.CallNonvirtualVoidMethodA(obj, clazz, mth, val);
	}
	return JPPyObject::getNone();
}

JPValue JPVoidType::getValueFromObject(const JPValue& obj)
{
	// This is needed if we call a caller sensitive method
	// and we get a return which is expected to be a void object
	JP_TRACE_IN("JPVoidType::getValueFromObject");
	return JPValue(this, (jobject) 0);
	JP_TRACE_OUT;
}

// GCOVR_EXCL_START

JPPyObject JPVoidType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	JP_RAISE(PyExc_SystemError, "void cannot be the type of a static field.");
}

JPPyObject JPVoidType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	JP_RAISE(PyExc_SystemError, "void cannot be the type of a field.");
}

JPPyObject JPVoidType::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast)
{
	return JPPyObject::getNone();
}

JPMatch::Type JPVoidType::findJavaConversion(JPMatch &match)
{
	return match.type = JPMatch::_none;
}

void JPVoidType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject*)
{
	JP_RAISE(PyExc_SystemError, "void cannot be the type of a static field.");
}

void JPVoidType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject*)
{
	JP_RAISE(PyExc_SystemError, "void cannot be the type of a field.");
}

void JPVoidType::setArrayRange(JPJavaFrame& frame, jarray,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_RAISE(PyExc_SystemError, "void cannot be the type of an array.");
}

JPPyObject JPVoidType::getArrayItem(JPJavaFrame& frame, jarray, jsize)
{
	JP_RAISE(PyExc_SystemError, "void cannot be the type of an array.");
}

void JPVoidType::setArrayItem(JPJavaFrame& frame, jarray, jsize, PyObject*)
{
	JP_RAISE(PyExc_SystemError, "void cannot be the type of an array.");
}

jarray JPVoidType::newArrayInstance(JPJavaFrame& frame, jsize)
{
	JP_RAISE(PyExc_SystemError, "void cannot be the type of an array.");
}

void JPVoidType::getView(JPArrayView& view)
{
}

void JPVoidType::releaseView(JPArrayView& view)
{
}

const char* JPVoidType::getBufferFormat()
{
	return NULL;
}

ssize_t JPVoidType::getItemSize()
{
	return 0;
}

void JPVoidType::copyElements(JPJavaFrame &frame,
		jarray a, jsize start, jsize len,
		void* memory, int offset)
{
}

char JPVoidType::getTypeCode()
{
	return 'V';
}

jlong JPVoidType::getAsLong(jvalue v)
{
	return 0;
}

jdouble JPVoidType::getAsDouble(jvalue v)
{
	return 0;
}

PyObject *JPVoidType::newMultiArray(JPJavaFrame &frame,
		JPPyBuffer& view, int subs, int base, jobject dims)
{
	return NULL;
}

// GCOVR_EXCL_STOP