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

HostRef* JPVoidType::getStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, JPTypeName& tgtType) 
{
	RAISE(JPypeException, "void cannot be the type of a static field.");
}

HostRef* JPVoidType::getInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, JPTypeName& tgtType) 
{
	RAISE(JPypeException, "void cannot be the type of a field.");
}

HostRef* JPVoidType::asHostObject(jvalue val) 
{
	return JPEnv::getHost()->getNone();
}
	
HostRef* JPVoidType::asHostObjectFromObject(jvalue val) 
{
	return JPEnv::getHost()->getNone();
}

EMatchType JPVoidType::canConvertToJava(HostRef* obj)
{
	return _none;
}

jvalue JPVoidType::convertToJava(HostRef* obj)
{
	jvalue res;
	res.l = NULL;
	return res;
}

HostRef* JPVoidType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	frame.CallStaticVoidMethodA(claz, mth, val);
	return JPEnv::getHost()->getNone();
}

HostRef* JPVoidType::invoke(JPJavaFrame& frame, jobject claz, jclass clazz, jmethodID mth, jvalue* val)
{
	frame.CallVoidMethodA(claz, mth, val);
	return JPEnv::getHost()->getNone();
}

void JPVoidType::setStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, HostRef*) 
{
	RAISE(JPypeException, "void cannot be the type of a static field.");
}

void JPVoidType::setInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, HostRef*) 
{
	RAISE(JPypeException, "void cannot be the type of a field.");
}

vector<HostRef*> JPVoidType::getArrayRange(JPJavaFrame& frame, jarray, int, int)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

void JPVoidType::setArrayRange(JPJavaFrame& frame, jarray, int, int, vector<HostRef*>&)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

HostRef* JPVoidType::getArrayItem(JPJavaFrame& frame, jarray, int)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

void JPVoidType::setArrayItem(JPJavaFrame& frame, jarray, int, HostRef*)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

jarray JPVoidType::newArrayInstance(JPJavaFrame& frame, int)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

HostRef* JPVoidType::convertToDirectBuffer(HostRef* src)
{
	RAISE(JPypeException, "Unable to convert to Direct Buffer");
}
