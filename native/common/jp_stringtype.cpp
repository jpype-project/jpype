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

HostRef* JPStringType::asHostObject(jvalue val) 
{
	TRACE_IN("JPStringType::asHostObject");
	
	if (val.l == NULL)
	{
		return JPEnv::getHost()->getNone();
	}
	
	JPJavaFrame frame;
	jstring v = (jstring)val.l;
	if (JPEnv::getConvertStringObjects())
	{
		TRACE1(" Performing conversion");
		jsize len = frame.GetStringLength(v);

		jboolean isCopy;
		const jchar* str = frame.GetStringChars(v, &isCopy);

		HostRef* res = JPEnv::getHost()->newStringFromUnicode(str, len);
		
		frame.ReleaseStringChars(v, str);

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
	JPJavaFrame frame;

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

  // Wrapper types are already converted	
	if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}

	// java.lang.string is already a global object 
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

	// Otherwise convert the string
	JPJavaFrame frame;
	JCharString wstr = JPEnv::getHost()->stringAsJCharString(obj);
	jchar* jstr = new jchar[wstr.length()+1];
	jstr[wstr.length()] = 0;
	for (size_t i = 0; i < wstr.length(); i++)
	{
		jstr[i] = (jchar)wstr[i];  
	}
	jstring res = frame.NewString(jstr, (jint)wstr.length());
	delete[] jstr;

	v.l = frame.keep(res);
	return v;
	TRACE_OUT;
}

jclass JPStringType::getClass() const
{
  return JPJni::s_StringClass;
}

