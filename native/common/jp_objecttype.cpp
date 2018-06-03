/*****************************************************************************
   Copyright 2004-2008 Steve Menard

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

HostRef* JPObjectType::getStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, JPTypeName& tgtType) 
{
	TRACE_IN("JPObjectType::getStaticValue");

	jobject r = frame.GetStaticObjectField(c, fid);
	
	JPTypeName name = JPJni::getClassName(r);
	JPType* type = JPTypeManager::getType(name);

	jvalue v;
	v.l = r;
	return type->asHostObject(v);

	TRACE_OUT;
}

HostRef* JPObjectType::getInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, JPTypeName& tgtType) 
{
	TRACE_IN("JPObjectType::getInstanceValue");
	jobject r = frame.GetObjectField(c, fid);

	JPTypeName name = JPJni::getClassName(r);
	JPType* type = JPTypeManager::getType(name);

	jvalue v;
	v.l = r;
	return type->asHostObject(v);

	TRACE_OUT;
}

HostRef* JPObjectType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	TRACE_IN("JPObjectType::invokeStatic");
	
	jobject res = frame.CallStaticObjectMethodA(claz, mth, val);

	jvalue v;
	v.l = res;

	JPTypeName name = JPJni::getClassName(v.l);
	JPType* type = JPTypeManager::getType(name);
	return type->asHostObject(v);

	TRACE_OUT;
}

HostRef* JPObjectType::invoke(JPJavaFrame& frame, jobject claz, jclass clazz, jmethodID mth, jvalue* val)
{
	TRACE_IN("JPObjectType::invoke");

	// Call method
	jobject res = frame.CallNonvirtualObjectMethodA(claz, clazz, mth, val);

	// Get the return type
	JPTypeName name = JPJni::getClassName(res);
	JPType* type = JPTypeManager::getType(name);

	// Convert the object
	jvalue v;
	v.l = res;
	HostRef* ref = type->asHostObject(v);
	TRACE1("Successfully converted to host reference");
	return ref;
	
	TRACE_OUT;
}

void JPObjectType::setStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, HostRef* obj) 
{
	TRACE_IN("JPObjectType::setStaticValue");

	jobject val = convertToJava(obj).l;

	frame.SetStaticObjectField(c, fid, val);
	TRACE_OUT;
}

void JPObjectType::setInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, HostRef* obj) 
{
	TRACE_IN("JPObjectType::setInstanceValue");

	jobject val = convertToJava(obj).l;

	frame.SetObjectField(c, fid, val);
	TRACE_OUT;
}

jarray JPObjectType::newArrayInstance(JPJavaFrame& frame, int sz)
{
	return frame.NewObjectArray(sz, getClass(), NULL);
}

vector<HostRef*> JPObjectType::getArrayRange(JPJavaFrame& frame, jarray a, int start, int length)
{
	jobjectArray array = (jobjectArray)a;	
	
	vector<HostRef*> res;
	
	jvalue v;
	for (int i = 0; i < length; i++)
	{
		v.l = frame.GetObjectArrayElement(array, i+start);

		JPTypeName name = JPJni::getClassName(v.l);
		JPType* t = JPTypeManager::getType(name);
		
		HostRef* pv = t->asHostObject(v);		

		res.push_back(pv);
	}
	
	return res;  
}

void JPObjectType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, vector<HostRef*>& vals)
{
	jobjectArray array = (jobjectArray)a;	
	
	jvalue v;
	for (int i = 0; i < length; i++)
	{
		v = convertToJava(vals[i]);
		frame.SetObjectArrayElement(array, i+start, v.l);
	}
}

void JPObjectType::setArrayItem(JPJavaFrame& frame, jarray a, int ndx, HostRef* val)
{
	jobjectArray array = (jobjectArray)a;	
	
	jvalue v = convertToJava(val);
	
	frame.SetObjectArrayElement(array, ndx, v.l);		
}

HostRef* JPObjectType::getArrayItem(JPJavaFrame& frame, jarray a, int ndx)
{
	TRACE_IN("JPObjectType::getArrayItem");
	jobjectArray array = (jobjectArray)a;	
	
	jobject obj = frame.GetObjectArrayElement(array, ndx);
	
	if (obj == NULL)
	{
		return JPEnv::getHost()->getNone();
	}
	
	jvalue v;
	v.l = obj;

	JPTypeName name = JPJni::getClassName(v.l);
	JPType* t = JPTypeManager::getType(name);
	
	return t->asHostObject(v);
	TRACE_OUT;
}

jobject JPObjectType::convertToJavaObject(HostRef* obj)
{
	jvalue v = convertToJava(obj);
	return v.l;
}

HostRef* JPObjectType::asHostObjectFromObject(jvalue val)
{
	return asHostObject(val);
}

HostRef* JPObjectType::convertToDirectBuffer(HostRef* src)
{
	RAISE(JPypeException, "Unable to convert to Direct Buffer");
}

bool JPObjectType::isSubTypeOf(const JPType& other) const
{
	const JPObjectType* otherObjectType = dynamic_cast<const JPObjectType*>(&other);
	if (!otherObjectType)
	{
		return false;
	}
	JPJavaFrame frame;
	jclass ourClass = getClass();
	jclass otherClass = otherObjectType->getClass();
	// IsAssignableFrom is a jni method and the order of parameters is counterintuitive
	bool otherIsSuperType = frame.IsAssignableFrom(ourClass, otherClass);
	//std::cout << other.getName().getSimpleName() << " isSuperType of " << getName().getSimpleName() << " " << otherIsSuperType << std::endl;
	return otherIsSuperType;
}


