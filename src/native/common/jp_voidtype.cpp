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

HostRef* JPVoidType::getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType) 
{
	RAISE(JPypeException, "void cannot be the type of a static field.");
}

HostRef* JPVoidType::getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType) 
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

HostRef* JPVoidType::invokeStatic(jclass claz, jmethodID mth, jvalue* val)
{
	JPEnv::getJava()->CallStaticVoidMethodA(claz, mth, val);
	return JPEnv::getHost()->getNone();
}

HostRef* JPVoidType::invoke(jobject claz, jclass clazz, jmethodID mth, jvalue* val)
{
	JPEnv::getJava()->CallVoidMethodA(claz, mth, val);
	return JPEnv::getHost()->getNone();
}

void JPVoidType::setStaticValue(jclass c, jfieldID fid, HostRef*) 
{
	RAISE(JPypeException, "void cannot be the type of a static field.");
}

void JPVoidType::setInstanceValue(jobject c, jfieldID fid, HostRef*) 
{
	RAISE(JPypeException, "void cannot be the type of a field.");
}

vector<HostRef*> JPVoidType::getArrayRange(jarray, int, int)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

void JPVoidType::setArrayRange(jarray, int, int, vector<HostRef*>&)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

HostRef* JPVoidType::getArrayItem(jarray, int)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

void JPVoidType::setArrayItem(jarray, int, HostRef*)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

jarray JPVoidType::newArrayInstance(int)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

HostRef* JPVoidType::convertToDirectBuffer(HostRef* src)
{
	RAISE(JPypeException, "Unable to convert to Direct Buffer");
}

void JPVoidType::setArrayValues(jarray a, HostRef* values)
{
	RAISE(JPypeException, "Unable to convert to Direct Buffer");
}
