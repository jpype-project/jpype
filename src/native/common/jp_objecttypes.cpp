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
	JPCleaner cleaner;

	jobject r = JPEnv::getJava()->GetStaticObjectField(c, fid);
	cleaner.addLocal(r);

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
	JPCleaner cleaner;
	jobject r = JPEnv::getJava()->GetObjectField(c, fid);
	cleaner.addLocal(r);

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
	JPCleaner cleaner;
	
	jobject res = JPEnv::getJava()->CallStaticObjectMethodA(claz, mth, val);
	cleaner.addLocal(res);

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
	JPCleaner cleaner;

	jobject res = JPEnv::getJava()->CallNonvirtualObjectMethodA(claz, clazz, mth, val);
	cleaner.addLocal(res);

	jvalue v;
	v.l = res;

	JPTypeName name = JPJni::getClassName(v.l);
	JPType* type = JPTypeManager::getType(name);
	HostRef* ref = type->asHostObject(v);
	TRACE1("Successfulyl converted to host reference");
	return ref;
	
	TRACE_OUT;
}

void JPObjectType::setStaticValue(jclass c, jfieldID fid, HostRef* obj) 
{
	TRACE_IN("JPObjectType::setStaticValue");
	JPCleaner cleaner;

	jobject val = convertToJava(obj).l;
	cleaner.addLocal(val);

	JPEnv::getJava()->SetStaticObjectField(c, fid, val);
	TRACE_OUT;
}

void JPObjectType::setInstanceValue(jobject c, jfieldID fid, HostRef* obj) 
{
	TRACE_IN("JPObjectType::setInstanceValue");
	JPCleaner cleaner;

	jobject val = convertToJava(obj).l;
	cleaner.addLocal(val);

	JPEnv::getJava()->SetObjectField(c, fid, val);
	TRACE_OUT;
}

jarray JPObjectType::newArrayInstance(int sz)
{
	JPCleaner cleaner;
	
	jclass c = getClass();
	cleaner.addLocal(c);
	
	return JPEnv::getJava()->NewObjectArray(sz, c, NULL);
}

vector<HostRef*> JPObjectType::getArrayRange(jarray a, int start, int length)
{
	jobjectArray array = (jobjectArray)a;	
	JPCleaner cleaner;
	
	vector<HostRef*> res;
	
	jvalue v;
	for (int i = 0; i < length; i++)
	{
		v.l = JPEnv::getJava()->GetObjectArrayElement(array, i+start);
		cleaner.addLocal(v.l);

		JPTypeName name = JPJni::getClassName(v.l);
		JPType* t = JPTypeManager::getType(name);
		
		HostRef* pv = t->asHostObject(v);		

		res.push_back(pv);
	}
	
	return res;  
}

void JPObjectType::setArrayRange(jarray a, int start, int length, vector<HostRef*>& vals)
{
	jobjectArray array = (jobjectArray)a;	
	JPCleaner cleaner;
	
	jvalue v;
	for (int i = 0; i < length; i++)
	{
		HostRef* pv = vals[i];
		
		v = convertToJava(pv);
		cleaner.addLocal(v.l);

		JPEnv::getJava()->SetObjectArrayElement(array, i+start, v.l);
	}
}

void JPObjectType::setArrayItem(jarray a, int ndx, HostRef* val)
{
	jobjectArray array = (jobjectArray)a;	
	JPCleaner cleaner;
	
	jvalue v = convertToJava(val);
	cleaner.addLocal(v.l);
	
	JPEnv::getJava()->SetObjectArrayElement(array, ndx, v.l);		
}

HostRef* JPObjectType::getArrayItem(jarray a, int ndx)
{
	TRACE_IN("JPObjectType::getArrayItem");
	jobjectArray array = (jobjectArray)a;	
	JPCleaner cleaner;
	
	jobject obj = JPEnv::getJava()->GetObjectArrayElement(array, ndx);
	cleaner.addLocal(obj);
	
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

void JPObjectType::setArrayValues(jarray a, HostRef* values)
{
    jobjectArray array = (jobjectArray)a;    
    JPCleaner cleaner;

    try {
		bool converted = true;

		// Optimize what I can ...
		// TODO also optimize array.array ...
		if (JPEnv::getHost()->isSequence(values))
		{
			int len = JPEnv::getHost()->getSequenceLength(values);
			for (int i = 0; i < len; i++)
			{
				HostRef* v = JPEnv::getHost()->getSequenceItem(values, i);
				JPEnv::getJava()->SetObjectArrayElement(array, i, convertToJava(v).l);
				delete v;
			}

			converted = true;
		}	
		else
		{
			RAISE(JPypeException, "Unable to convert to Object array");
		}

    }
    RETHROW_CATCH( ; );
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
	JPCleaner cleaner;

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
	JPCleaner cleaner;
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
	delete jstr;
	
	v.l = res;
	
	return v;
	TRACE_OUT;
}

jclass JPStringType::getClass()
{
	return (jclass)JPEnv::getJava()->NewGlobalRef(JPJni::s_StringClass);
}

//-------------------------------------------------------------------------------

HostRef* JPClassType::asHostObject(jvalue val) 
{
	jclass lclass = (jclass)val.l;
	
	JPTypeName name = JPJni::getName(lclass);
	
	JPClass* res = JPTypeManager::findClass(name);

	
	return JPEnv::getHost()->newClass(res);
}

EMatchType JPClassType::canConvertToJava(HostRef* obj)
{
	JPCleaner cleaner;

	if (JPEnv::getHost()->isNone(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isClass(obj))
	{
		return _exact;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);

		if (name.getType() == JPTypeName::_class)
		{
			return _exact;
		}
	}

	return _none;
}

jvalue JPClassType::convertToJava(HostRef* obj)
{
	JPCleaner cleaner;

	jvalue v;
	if (JPEnv::getHost()->isNone(obj))
	{
		v.l = NULL;
		return v;
	}
	
	else if (JPEnv::getHost()->isWrapper(obj))
	{
		v = JPEnv::getHost()->getWrapperValue(obj);
	}
	else
	{
		JPClass* w = JPEnv::getHost()->asClass(obj);

		jclass lr = w->getClass();

		v.l = lr;
	}

	return v;		
}

jclass JPClassType::getClass()
{
	return (jclass)JPEnv::getJava()->NewGlobalRef(JPJni::s_ClassClass);
}

