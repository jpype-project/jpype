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
#include "jp_typemanager.h"

namespace
{
//AT's on porting:
// 1) TODO: test on HP-UX platform. Cause: it is suspected to be an undefined order of initialization of static objects
//
//  2) TODO: in any case, use of static objects may impose problems in multi-threaded environment.
typedef map<string, JPClass* > JavaClassMap;

JavaClassMap javaClassMap;
jclass utility;
jmethodID getClassForID;
jmethodID callMethodID;
jmethodID isCallerSensitiveID;
}

namespace JPTypeManager
{
JPVoidType* _void;
JPBooleanType* _boolean;
JPByteType* _byte;
JPCharType* _char;
JPShortType* _short;
JPIntType* _int;
JPLongType* _long;
JPFloatType* _float;
JPDoubleType* _double;
JPClass* _java_lang_Object;
JPClass* _java_lang_Class;
JPStringClass* _java_lang_String;
JPStringClass* _java_lang_CharSequence;

JPBoxedClass* _java_lang_Void;
JPBoxedClass* _java_lang_Boolean;
JPBoxedClass* _java_lang_Byte;
JPBoxedClass* _java_lang_Character;
JPBoxedClass* _java_lang_Short;
JPBoxedClass* _java_lang_Integer;
JPBoxedClass* _java_lang_Long;
JPBoxedClass* _java_lang_Float;
JPBoxedClass* _java_lang_Double;
JPClass* _java_lang_Number;
JPClass* _java_lang_Throwable;
}

JPClass* JPTypeManager::registerClass(JPClass* classWrapper)
{
	JP_TRACE_IN("JPTypeManager::registerClass (specialized)");
	const string& simple = classWrapper->getCanonicalName();
	javaClassMap[simple] = classWrapper;
	JP_TRACE(simple, classWrapper);
	return classWrapper;
	JP_TRACE_OUT;
}

JPClass* registerArrayClass(string name, jclass jc)
{
	JP_TRACE_IN("JPTypeManager::registerArrayClass");
	JPClass* cls = new JPArrayClass(jc);
	JP_TRACE(name, cls);
	javaClassMap[name] = cls;
	return cls;
	JP_TRACE_OUT;
}

JPClass* registerObjectClass(string name, jclass jc)
{
	JP_TRACE_IN("JPTypeManager::registerObjectClass");
	JPClass* cls = new JPClass(jc);
	JP_TRACE(name, cls);
	javaClassMap[name] = cls;
	cls->postLoad();
	return cls;
	JP_TRACE_OUT;
}

jclass JPTypeManager::getClassFor(jobject obj)
{
	if (getClassForID == 0)
		return NULL;
	JPJavaFrame frame;
	jvalue v;
	v.l = obj;
	return (jclass) frame.keep(frame.CallStaticObjectMethodA(utility, getClassForID, &v));
}

bool JPTypeManager::isCallerSensitive(jobject obj)
{
	if (isCallerSensitiveID == 0)
		return false;
	JPJavaFrame frame;
	jvalue v;
	v.l = obj;
	return frame.CallStaticBooleanMethodA(utility, isCallerSensitiveID, &v) != 0;
}

jobject JPTypeManager::callMethod(jobject method, jobject obj, jobject args)
{
	JP_TRACE_IN("JPTypeManager::callMethod");
	if (callMethodID == 0)
		return NULL;
	JPJavaFrame frame;
	jvalue v[3];
	v[0].l = method;
	v[1].l = obj;
	v[2].l = args;
	return frame.keep(frame.CallStaticObjectMethodA(utility, callMethodID, v));
	JP_TRACE_OUT;
}

void JPTypeManager::init()
{
	// Everything that requires specialization must be created here.
	JPJavaFrame frame;
	JP_TRACE_IN("JPTypeManager::init");

	// Get utility class
	utility = (jclass) frame.NewGlobalRef(JPClassLoader::findClass("org.jpype.Utility"));

	// Get support methods
	callMethodID = frame.GetStaticMethodID(utility, "callMethod",
			"(Ljava/lang/reflect/Method;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");

	isCallerSensitiveID = frame.GetStaticMethodID(utility, "isCallerSensitive",
			"(Ljava/lang/reflect/Method;)Z");

	getClassForID = frame.GetStaticMethodID(utility, "getClassFor",
			"(Ljava/lang/Object;)Ljava/lang/Class;");

	jclass cls;
	cls = (jclass) frame.FindClass("java/lang/Object");
	registerClass(_java_lang_Object = new JPObjectBaseClass(cls));
	cls = (jclass) frame.FindClass("java/lang/Class");
	registerClass(_java_lang_Class = new JPClassBaseClass(cls));

	cls = (jclass) frame.FindClass("java/lang/String");
	registerClass(_java_lang_String = new JPStringClass(cls));
	cls = (jclass) frame.FindClass("java/lang/CharSequence");
	registerClass(_java_lang_CharSequence = new JPStringClass(cls));
	cls = (jclass) frame.FindClass("java/lang/Number");
	registerClass(_java_lang_Number = new JPClass(cls));
	cls = (jclass) frame.FindClass("java/lang/Throwable");
	registerClass(_java_lang_Throwable = new JPClass(cls));

	cls = (jclass) frame.FindClass("java/lang/Void");
	registerClass(_java_lang_Void = new JPBoxedVoidClass(cls));
	cls = (jclass) frame.FindClass("java/lang/Boolean");
	registerClass(_java_lang_Boolean = new JPBoxedBooleanClass(cls));
	cls = (jclass) frame.FindClass("java/lang/Byte");
	registerClass(_java_lang_Byte = new JPBoxedByteClass(cls));
	cls = (jclass) frame.FindClass("java/lang/Character");
	registerClass(_java_lang_Character = new JPBoxedCharacterClass(cls));
	cls = (jclass) frame.FindClass("java/lang/Short");
	registerClass(_java_lang_Short = new JPBoxedShortClass(cls));
	cls = (jclass) frame.FindClass("java/lang/Integer");
	registerClass(_java_lang_Integer = new JPBoxedIntegerClass(cls));
	cls = (jclass) frame.FindClass("java/lang/Long");
	registerClass(_java_lang_Long = new JPBoxedLongClass(cls));
	cls = (jclass) frame.FindClass("java/lang/Float");
	registerClass(_java_lang_Float = new JPBoxedFloatClass(cls));
	cls = (jclass) frame.FindClass("java/lang/Double");
	registerClass(_java_lang_Double = new JPBoxedDoubleClass(cls));

	registerClass(_void = new JPVoidType());
	registerClass(_boolean = new JPBooleanType());
	registerClass(_byte = new JPByteType());
	registerClass(_char = new JPCharType());
	registerClass(_short = new JPShortType());
	registerClass(_int = new JPIntType());
	registerClass(_long = new JPLongType());
	registerClass(_float = new JPFloatType());
	registerClass(_double = new JPDoubleType());

	JP_TRACE_OUT;
}

JPClass* JPTypeManager::findClass(const string& name)
{
	JP_TRACE_IN("JPTypeManager::findClass");
	try
	{
		JP_TRACE("Finding", name);
		// First check in the map ...
		JavaClassMap::iterator cur = javaClassMap.find(name);

		if (cur != javaClassMap.end())
		{
			return cur->second;
		}

		// Convert to native name
		string cname = name;
		for (size_t i = 0; i < cname.size(); ++i)
		{
			if (cname[i] == '.')
				cname[i] = '/';
		}

		// Okay so it isn't already loaded, we need to find the class then make a wrapper for it
		JPJavaFrame frame;
		jclass cls = (jclass) frame.FindClass(cname.c_str());
		string aname = JPJni::getCanonicalName(cls);
		JP_TRACE("FIXME ", cname, aname);
		return findClass((jclass) frame.keep(cls));
	}
	JP_CATCH;
	JP_TRACE_OUT;
}

JPClass* JPTypeManager::findClassForObject(jobject obj)
{
	if (obj == NULL)
		return NULL;
	return findClass(getClassFor(obj));
}

JPClass* JPTypeManager::findClass(jclass cls)
{
	if (cls == NULL)
		return NULL;

	string name = JPJni::getCanonicalName(cls);

	// Fist check in the map ...
	JavaClassMap::iterator cur = javaClassMap.find(name);

	if (cur != javaClassMap.end())
	{
		return cur->second;
	}

	JP_TRACE_IN("JPTypeManager::findClassLoad");
	JP_TRACE(name);

	// No we haven't got it .. lets load it!!!
	JPJavaFrame frame;
	if (JPJni::isArray(cls))
	{
		return registerArrayClass(name, cls);
	} else
	{
		return registerObjectClass(name, cls);
	}
	JP_TRACE_OUT;
}

void JPTypeManager::shutdown()
{
	JP_TRACE_IN("JPTypeManager::shutdown");
	flushCache();
	JP_TRACE_OUT;
}

void JPTypeManager::flushCache()
{
	for (JavaClassMap::iterator i = javaClassMap.begin(); i != javaClassMap.end(); ++i)
	{
		delete i->second;
	}
	javaClassMap.clear();
}

int JPTypeManager::getLoadedClasses()
{
	// diagnostic tools ... unlikely to load more classes than int can hold ...
	return (int) javaClassMap.size();
}
