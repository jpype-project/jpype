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
#include <jp_typemanager.h>

namespace
{
	//AT's on porting:
	// 1) TODO: test on HP-UX platform. Cause: it is suspected to be an undefined order of initialization of static objects
	//
	//  2) TODO: in any case, use of static objects may impose problems in multi-threaded environment.
	typedef map<string, JPClass* > JavaClassMap;

	JavaClassMap javaClassMap;

	//	TypeMap typeMap;
	//	JavaClassMap javaClassMap;
	//	JavaArrayClassMap javaArrayClassMap;
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

	JPBoxedClass* _java_lang_Void;
	JPBoxedClass* _java_lang_Boolean;
	JPBoxedClass* _java_lang_Byte;
	JPBoxedClass* _java_lang_Char;
	JPBoxedClass* _java_lang_Short;
	JPBoxedClass* _java_lang_Integer;
	JPBoxedClass* _java_lang_Long;
	JPBoxedClass* _java_lang_Float;
	JPBoxedClass* _java_lang_Double;
}

JPClass* registerClass(JPClass* classWrapper)
{
	JP_TRACE_IN("JPTypeManager::registerClass (specialized)");
	const string& simple = classWrapper->getCanonicalName();
	javaClassMap[simple] = classWrapper;
	JP_TRACE(simple, classWrapper);
	classWrapper->postLoad();
	return classWrapper;
	JP_TRACE_OUT;
}

JPClass* registerArrayClass(string name, jclass jc)
{
	JP_TRACE_IN("JPTypeManager::registerArrayClass");
	JPClass* cls = new JPArrayClass(jc);
	JP_TRACE(name, cls);
	javaClassMap[name] = cls;
	cls->postLoad();
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

void JPTypeManager::init()
{
	// Everything that requires specialization must be created here.
	JPJavaFrame frame;
	JP_TRACE_IN("JPTypeManager::init");
	registerClass(_java_lang_Object = new JPObjectType());
	registerClass(_java_lang_Class = new JPClassType());
	registerClass(_java_lang_String = new JPStringClass());

	registerClass(_java_lang_Void = new JPBoxedVoidType());
	registerClass(_java_lang_Boolean = new JPBoxedBooleanType());
	registerClass(_java_lang_Byte = new JPBoxedByteType());
	registerClass(_java_lang_Char = new JPBoxedCharacterType());
	registerClass(_java_lang_Short = new JPBoxedShortType());
	registerClass(_java_lang_Integer = new JPBoxedIntegerType());
	registerClass(_java_lang_Long = new JPBoxedLongType());
	registerClass(_java_lang_Float = new JPBoxedFloatType());
	registerClass(_java_lang_Double = new JPBoxedDoubleType());

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
	return findClass(JPJni::getClass(obj));
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
	}
	else
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
