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

HostRef* JPObjectType::getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType) 
{
	TRACE_IN("JPObjectType::getStaticValue");
	JPLocalFrame frame;

	jobject r = JPEnv::getJava()->GetStaticObjectField(c, fid);

	jvalue v;
	v.l = r;
	
	JPTypeName name = JPJni::getClassName(v.l);
	JPType* type = JPTypeManager::getType(name);
	return type->asHostObject(v);

	TRACE_OUT;
}

HostRef* JPObjectType::getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType) 
{
	TRACE_IN("JPObjectType::getInstanceValue");
	JPLocalFrame frame;
	jobject r = JPEnv::getJava()->GetObjectField(c, fid);

	jvalue v;
	v.l = r;
	
	JPTypeName name = JPJni::getClassName(v.l);
	JPType* type = JPTypeManager::getType(name);
	return type->asHostObject(v);

	TRACE_OUT;
}

HostRef* JPObjectType::invokeStatic(jclass claz, jmethodID mth, jvalue* val)
{
	TRACE_IN("JPObjectType::invokeStatic");
	JPLocalFrame frame;
	
	jobject res = JPEnv::getJava()->CallStaticObjectMethodA(claz, mth, val);

	jvalue v;
	v.l = res;

	JPTypeName name = JPJni::getClassName(v.l);
	JPType* type = JPTypeManager::getType(name);
	return type->asHostObject(v);

	TRACE_OUT;
}

HostRef* JPObjectType::invoke(jobject claz, jclass clazz, jmethodID mth, jvalue* val)
{
	TRACE_IN("JPObjectType::invoke");
	JPLocalFrame frame;

	// Call method
	jobject res = JPEnv::getJava()->CallNonvirtualObjectMethodA(claz, clazz, mth, val);

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

void JPObjectType::setStaticValue(jclass c, jfieldID fid, HostRef* obj) 
{
	TRACE_IN("JPObjectType::setStaticValue");
	JPLocalFrame frame;

	jobject val = convertToJava(obj).l;

	JPEnv::getJava()->SetStaticObjectField(c, fid, val);
	TRACE_OUT;
}

void JPObjectType::setInstanceValue(jobject c, jfieldID fid, HostRef* obj) 
{
	TRACE_IN("JPObjectType::setInstanceValue");
	JPLocalFrame frame;

	jobject val = convertToJava(obj).l;

	JPEnv::getJava()->SetObjectField(c, fid, val);
	TRACE_OUT;
}

jarray JPObjectType::newArrayInstance(int sz)
{
	return JPEnv::getJava()->NewObjectArray(sz, getClass(), NULL);
}

vector<HostRef*> JPObjectType::getArrayRange(jarray a, int start, int length)
{
	jobjectArray array = (jobjectArray)a;	
	JPLocalFrame frame;
	
	vector<HostRef*> res;
	
	jvalue v;
	for (int i = 0; i < length; i++)
	{
		v.l = JPEnv::getJava()->GetObjectArrayElement(array, i+start);

		JPTypeName name = JPJni::getClassName(v.l);
		JPType* t = JPTypeManager::getType(name);
		
		HostRef* pv = t->asHostObject(v);		

		res.push_back(pv);
	}
	
	return res;  
}

void JPObjectType::setArrayRange(jarray a, int start, int length, vector<HostRef*>& vals)
{
	JPLocalFrame frame(8+length);
	jobjectArray array = (jobjectArray)a;	
	
	jvalue v;
	for (int i = 0; i < length; i++)
	{
		HostRef* pv = vals[i];
		
		v = convertToJava(pv);

		JPEnv::getJava()->SetObjectArrayElement(array, i+start, v.l);
	}
}

void JPObjectType::setArrayItem(jarray a, int ndx, HostRef* val)
{
	JPLocalFrame frame;
	jobjectArray array = (jobjectArray)a;	
	
	jvalue v = convertToJava(val);
	
	JPEnv::getJava()->SetObjectArrayElement(array, ndx, v.l);		
}

HostRef* JPObjectType::getArrayItem(jarray a, int ndx)
{
	JPLocalFrame frame;
	TRACE_IN("JPObjectType::getArrayItem");
	jobjectArray array = (jobjectArray)a;	
	
	jobject obj = JPEnv::getJava()->GetObjectArrayElement(array, ndx);
	
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
	JPLocalFrame frame;
	jclass ourClass = getClass();
	jclass otherClass = otherObjectType->getClass();
	// IsAssignableFrom is a jni method and the order of parameters is counterintuitive
	bool otherIsSuperType = JPEnv::getJava()->IsAssignableFrom(ourClass, otherClass);
	//std::cout << other.getName().getSimpleName() << " isSuperType of " << getName().getSimpleName() << " " << otherIsSuperType << std::endl;
	return otherIsSuperType;
}


//-------------------------------------------------------------------------------

HostRef* JPStringType::asHostObject(jvalue val) 
{
	TRACE_IN("JPStringType::asHostObject");
	
	if (val.l == NULL)
	{
		return JPEnv::getHost()->getNone();
	}
	
	jstring v = (jstring)val.l;

	if (JPEnv::getJava()->getConvertStringObjects())
	{
		TRACE1(" Performing conversion");
		jsize len = JPEnv::getJava()->GetStringLength(v);

		jboolean isCopy;
		const jchar* str = JPEnv::getJava()->GetStringChars(v, &isCopy);

		HostRef* res = JPEnv::getHost()->newStringFromUnicode(str, len);
		
		JPEnv::getJava()->ReleaseStringChars(v, str);

		return res;
	}
	else
	{
		TRACE1(" Performing wrapping");
		HostRef* res = JPEnv::getHost()->newStringWrapper(v);
		TRACE1(" Wrapping successfull");
		return res;
	}
	TRACE_OUT;
}

EMatchType JPStringType::canConvertToJava(HostRef* obj)
{
	TRACE_IN("JPStringType::canConvertToJava");
	JPLocalFrame frame;

	if (obj == NULL || JPEnv::getHost()->isNone(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isString(obj))
	{
		return _exact;
	}
	
	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);

		if (name.getType() == JPTypeName::_string)
		{
			return _exact;
		}
	}

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* o = JPEnv::getHost()->asObject(obj);

		JPClass* oc = o->getClass();
		if (oc->getName().getSimpleName() == "java.lang.String")
		{
			return _exact;
		}
	}
	return _none;
	TRACE_OUT;
}

jvalue JPStringType::convertToJava(HostRef* obj)
{
	TRACE_IN("JPStringType::convertToJava");
	jvalue v;
	
	if (JPEnv::getHost()->isNone(obj))
	{
		v.l = NULL;
		return v;
	}
	
	if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* o = JPEnv::getHost()->asObject(obj);

		JPClass* oc = o->getClass();
		if (oc->getName().getSimpleName() == "java.lang.String")
		{
			v.l = o->getObject(); 
			return v;
		}
	}

	JCharString wstr = JPEnv::getHost()->stringAsJCharString(obj);

	jchar* jstr = new jchar[wstr.length()+1];
	jstr[wstr.length()] = 0;
	for (size_t i = 0; i < wstr.length(); i++)
	{
		jstr[i] = (jchar)wstr[i];  
	}
	jstring res = JPEnv::getJava()->NewString(jstr, (jint)wstr.length());
	delete[] jstr;
	
	v.l = res;
	
	return v;
	TRACE_OUT;
}

jclass JPStringType::getClass() const
{
  return JPJni::s_StringClass;
}

