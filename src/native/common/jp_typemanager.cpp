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

//AT's on porting:
//  1) the original definition of global static object leads to crashing
//on HP-UX platform. Cause: it is suspected to be an undefined order of initialization of static objects
//
//  2) TODO: in any case, use of static objects may impose problems in multi-threaded environment.
//Therefore, they must be guarded with a mutex.
typedef map<JPTypeName::ETypes, JPType*> TypeMap;
typedef map<string, JPClass* > JavaClassMap;
typedef map<string, JPArrayClass* > JavaArrayClassMap;

static JavaArrayClassMap javaArrayClassMap;

// Warning: this eliminates "static initalization order" problem but should also be guarded with a lock for MT
template <class _T> _T& GetMap()
{
	static _T container;
	return container;
}

#define GET_TypeMap GetMap<TypeMap>
#define GET_JavaClassMap GetMap<JavaClassMap>
#define GET_JavaArrayClassMap GetMap<JavaArrayClassMap>

void JPTypeManager::init()
{
	GET_TypeMap()[JPTypeName::_void] = new JPVoidType();			
	GET_TypeMap()[JPTypeName::_byte] = new JPByteType();			
	GET_TypeMap()[JPTypeName::_short] = new JPShortType();			
	GET_TypeMap()[JPTypeName::_int] = new JPIntType();			
	GET_TypeMap()[JPTypeName::_long] = new JPLongType();			
	GET_TypeMap()[JPTypeName::_float] = new JPFloatType();			
	GET_TypeMap()[JPTypeName::_double] = new JPDoubleType();			
	GET_TypeMap()[JPTypeName::_char] = new JPCharType();			
	GET_TypeMap()[JPTypeName::_boolean] = new JPBooleanType();				
	GET_TypeMap()[JPTypeName::_string] = new JPStringType();			
	GET_TypeMap()[JPTypeName::_class] = new JPClassType();	

	// Preload the "primitive" types
	GET_JavaClassMap()["byte"] = new JPClass(JPTypeName::fromSimple("byte"), JPJni::getByteClass());
	GET_JavaClassMap()["short"] = new JPClass(JPTypeName::fromSimple("short"), JPJni::getShortClass());
	GET_JavaClassMap()["int"] = new JPClass(JPTypeName::fromSimple("int"), JPJni::getIntegerClass());
	GET_JavaClassMap()["long"] = new JPClass(JPTypeName::fromSimple("long"), JPJni::getLongClass());
	GET_JavaClassMap()["float"] = new JPClass(JPTypeName::fromSimple("float"), JPJni::getFloatClass());
	GET_JavaClassMap()["double"] = new JPClass(JPTypeName::fromSimple("double"), JPJni::getDoubleClass());
	GET_JavaClassMap()["char"] = new JPClass(JPTypeName::fromSimple("char"), JPJni::getCharacterClass());
	GET_JavaClassMap()["boolean"] = new JPClass(JPTypeName::fromSimple("boolean"), JPJni::getBooleanClass());
	GET_JavaClassMap()["void"] = new JPClass(JPTypeName::fromSimple("void"), JPJni::getVoidClass());
}

JPClass* JPTypeManager::findClass(JPTypeName& name)
{
	// Fist check in the map ...
	JavaClassMap::iterator cur = GET_JavaClassMap().find(name.getSimpleName());		
	
	if (cur != GET_JavaClassMap().end())
	{
		return cur->second;
	}
	
	TRACE_IN("JPTypeManager::findClass");
	TRACE1(name.getSimpleName());

	// No we havent got it .. lets load it!!!
	JPCleaner cleaner;
	jclass cls = JPEnv::getJava()->FindClass(name.getNativeName().c_str());
	cleaner.addLocal(cls);

	JPClass* res = new JPClass(name, cls);
	
	// Register it here before we do anything else
	GET_JavaClassMap()[name.getSimpleName()] = res;
	
	// Finish loading it
	res->postLoad();		

	return res;
	TRACE_OUT;
}

JPArrayClass* JPTypeManager::findArrayClass(JPTypeName& name)
{
	// Fist check in the map ...
	JavaArrayClassMap::iterator cur = GET_JavaArrayClassMap().find(name.getSimpleName());		
	
	if (cur != GET_JavaArrayClassMap().end())
	{
		return cur->second;
	}
	
	// No we havent got it .. lets load it!!!
	JPCleaner cleaner;
	jclass cls = JPEnv::getJava()->FindClass(name.getNativeName().c_str());

	cleaner.addLocal(cls);

	JPArrayClass* res = new JPArrayClass(name, cls);
	
	// Register it here before we do anything else
	GET_JavaArrayClassMap()[name.getSimpleName()] = res;
	
	return res;
}

JPType* JPTypeManager::getType(JPTypeName& t)
{
	JPCleaner cleaner;
	TRACE_IN("JPTypeManager::getType");
	map<JPTypeName::ETypes, JPType*>::iterator it = GET_TypeMap().find(t.getType());
	
	if (it != GET_TypeMap().end())
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

void JPTypeManager::flushCache()
{
	// TODO delete the values 
	GET_JavaClassMap().clear();
	GET_JavaArrayClassMap().clear();
}

int JPTypeManager::getLoadedClasses()
{
	// dignostic tools ... unlikely to load more classes than int can hold ...
	return (int)(GET_JavaClassMap().size() + GET_JavaArrayClassMap().size());
}

