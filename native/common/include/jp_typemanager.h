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
#ifndef _JPTYPE_MANAGER_H_
#define _JPTYPE_MANAGER_H_

/**
 * These functions will manage the cache of found type, be it primitive types, class types or the "magic" types.
 */
namespace JPTypeFactory
{
	void init(JPContext* content);
}

namespace JPTypeManager
{
	extern JPVoidType* _void;
	extern JPBooleanType* _boolean;
	extern JPByteType* _byte;
	extern JPCharType* _char;
	extern JPShortType* _short;
	extern JPIntType* _int;
	extern JPLongType* _long;
	extern JPFloatType* _float;
	extern JPDoubleType* _double;
	extern JPClass* _java_lang_Object;
	extern JPClass* _java_lang_Class;
	extern JPStringClass* _java_lang_String;

	extern JPBoxedClass* _java_lang_Void;
	extern JPBoxedClass* _java_lang_Boolean;
	extern JPBoxedClass* _java_lang_Byte;
	extern JPBoxedClass* _java_lang_Char;
	extern JPBoxedClass* _java_lang_Short;
	extern JPBoxedClass* _java_lang_Integer;
	extern JPBoxedClass* _java_lang_Long;
	extern JPBoxedClass* _java_lang_Float;
	extern JPBoxedClass* _java_lang_Double;

	/**
	 * Initialize the type manager caches
	 */
	void init();

	/**
	 * delete allocated typenames, should only be called at program termination
	 */
	void shutdown();

	/**
	 * Find a class using a native name.
	 * 
	 * The pointer returned is NOT owned by the caller
	 */
	JPClass* findClass(const string& str);
	JPClass* findClass(jclass cls);
	JPClass* findClassForObject(jobject obj);

	void flushCache();

	int getLoadedClasses();
} // namespace

#endif // _JPCLASS_H_
