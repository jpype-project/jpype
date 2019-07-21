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

JPVoidType::JPVoidType(JPContext* context, jclass clss,
		       const string& name,
		       JPBoxedType* boxedClass,
		       jint modifiers)
: JPPrimitiveType(context, clss, name, boxedClass, modifiers)
{
}

JPVoidType::~JPVoidType()
{
}

JPPyObject JPVoidType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	JP_RAISE_RUNTIME_ERROR("void cannot be the type of a static field.");
}

JPPyObject JPVoidType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	JP_RAISE_RUNTIME_ERROR("void cannot be the type of a field.");
}

JPPyObject JPVoidType::convertToPythonObject(jvalue val)
{
	return JPPyObject::getNone();
}

JPMatch::Type JPVoidType::getJavaConversion(JPMatch& match, JPJavaFrame& frame, PyObject* pyobj)
{
	return match.type = JPMatch::_none;
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

void JPVoidType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject*)
{
	JP_RAISE_RUNTIME_ERROR("void cannot be the type of a static field.");
}

void JPVoidType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject*)
{
	JP_RAISE_RUNTIME_ERROR("void cannot be the type of a field.");
}

JPPyObject JPVoidType::getArrayRange(JPJavaFrame& frame, jarray, jsize start, jsize length)
{
	JP_RAISE_RUNTIME_ERROR("void cannot be the type of an array.");
}

void JPVoidType::setArrayRange(JPJavaFrame& frame, jarray, jsize, jsize, PyObject*)
{
	JP_RAISE_RUNTIME_ERROR("void cannot be the type of an array.");
}

JPPyObject JPVoidType::getArrayItem(JPJavaFrame& frame, jarray, jsize)
{
	JP_RAISE_RUNTIME_ERROR("void cannot be the type of an array.");
}

void JPVoidType::setArrayItem(JPJavaFrame& frame, jarray, jsize, PyObject*)
{
	JP_RAISE_RUNTIME_ERROR("void cannot be the type of an array.");
}

jarray JPVoidType::newArrayInstance(JPJavaFrame& frame, jsize)
{
	JP_RAISE_RUNTIME_ERROR("void cannot be the type of an array.");
}
