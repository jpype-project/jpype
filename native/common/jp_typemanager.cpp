/*****************************************************************************
   Copyright 2004 Steve Menard

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

namespace {
//AT's on porting:
// 1) TODO: test on HP-UX platform. Cause: it is suspected to be an undefined order of initialization of static objects
//
//  2) TODO: in any case, use of static objects may impose problems in multi-threaded environment.
	typedef map<JPTypeName::ETypes, JPType*> TypeMap;
	typedef map<string, JPClass* > JavaClassMap;
	typedef map<string, JPArrayClass* > JavaArrayClassMap;

	TypeMap typeMap;
	JavaClassMap javaClassMap;
	JavaArrayClassMap javaArrayClassMap;

//	TypeMap typeMap;
//	JavaClassMap javaClassMap;
//	JavaArrayClassMap javaArrayClassMap;
}

namespace JPTypeManager {

void init()
{
	TRACE_IN("JPTypeManager::init");
	typeMap[JPTypeName::_void] = new JPVoidType();
	typeMap[JPTypeName::_byte] = new JPByteType();
	typeMap[JPTypeName::_short] = new JPShortType();
	typeMap[JPTypeName::_int] = new JPIntType();
	typeMap[JPTypeName::_long] = new JPLongType();
	typeMap[JPTypeName::_float] = new JPFloatType();
	typeMap[JPTypeName::_double] = new JPDoubleType();
	typeMap[JPTypeName::_char] = new JPCharType();
	typeMap[JPTypeName::_boolean] = new JPBooleanType();
	typeMap[JPTypeName::_string] = new JPStringType();

	// Preload the "primitive" types
	javaClassMap["byte"] = new JPClass(JPTypeName::fromSimple("byte"), JPJni::getByteClass());
	javaClassMap["short"] = new JPClass(JPTypeName::fromSimple("short"), JPJni::getShortClass());
	javaClassMap["int"] = new JPClass(JPTypeName::fromSimple("int"), JPJni::getIntegerClass());
	javaClassMap["long"] = new JPClass(JPTypeName::fromSimple("long"), JPJni::getLongClass());
	javaClassMap["float"] = new JPClass(JPTypeName::fromSimple("float"), JPJni::getFloatClass());
	javaClassMap["double"] = new JPClass(JPTypeName::fromSimple("double"), JPJni::getDoubleClass());
	javaClassMap["char"] = new JPClass(JPTypeName::fromSimple("char"), JPJni::getCharacterClass());
	javaClassMap["boolean"] = new JPClass(JPTypeName::fromSimple("boolean"), JPJni::getBooleanClass());
	javaClassMap["void"] = new JPClass(JPTypeName::fromSimple("void"), JPJni::getVoidClass());
	TRACE_OUT;
}

JPClass* findClass(const JPTypeName& name)
{
	// Fist check in the map ...
	JavaClassMap::iterator cur = javaClassMap.find(name.getSimpleName());

	if (cur != javaClassMap.end())
	{
		return cur->second;
	}

	TRACE_IN("JPTypeManager::findClass");
	TRACE1(name.getSimpleName());

	// No we havent got it .. lets load it!!!
	JPJavaFrame frame;
	jclass cls = frame.FindClass(name.getNativeName().c_str());
	JPClass* res = new JPClass(name, cls);
	// Finish loading it
	res->postLoad();
	// Register it here before we do anything else
	javaClassMap[name.getSimpleName()] = res;

	return res;
	TRACE_OUT;
}

JPArrayClass* findArrayClass(const JPTypeName& name)
{
	// Fist check in the map ...
	JavaArrayClassMap::iterator cur = javaArrayClassMap.find(name.getSimpleName());

	if (cur != javaArrayClassMap.end())
	{
		return cur->second;
	}

	// No we havent got it .. lets load it!!!
	JPJavaFrame frame;
	jclass cls = frame.FindClass(name.getNativeName().c_str());
	JPArrayClass* res = new JPArrayClass(name, cls);

	// Register it here before we do anything else
	javaArrayClassMap[name.getSimpleName()] = res;

	return res;
}

JPType* getType(const JPTypeName& t)
{
	TRACE_IN("JPTypeManager::getType");
	TRACE1(t.getSimpleName());
	map<JPTypeName::ETypes, JPType*>::iterator it = typeMap.find(t.getType());

	if (it != typeMap.end())
	{
		return it->second;
	}

	if (t.getType() == JPTypeName::_array)
	{
		JPArrayClass* c = findArrayClass(t);
		return c;
	}
	else
	{
		JPClass* c = findClass(t);
		return c;
	}
	TRACE_OUT;
}

void shutdown()
{
	TRACE_IN("JPTypeManager::shutdown");
	flushCache();

	// delete primitive types
	for(TypeMap::iterator i = typeMap.begin(); i != typeMap.end(); ++i)
	{
		delete i->second;
	}
	TRACE_OUT;
}

void flushCache()
{
	for(JavaClassMap::iterator i = javaClassMap.begin(); i != javaClassMap.end(); ++i)
	{
		delete i->second;
	}

	for(JavaArrayClassMap::iterator i = javaArrayClassMap.begin();
			i != javaArrayClassMap.end(); ++i)
	{
		delete i->second;
	}

	javaClassMap.clear();
	javaArrayClassMap.clear();
}

int getLoadedClasses()
{
	// dignostic tools ... unlikely to load more classes than int can hold ...
	return (int)(javaClassMap.size() + javaArrayClassMap.size());
}

} // end of namespace JPTypeManager
