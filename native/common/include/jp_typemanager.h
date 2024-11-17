/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#ifndef _JPTYPE_MANAGER_H_
#define _JPTYPE_MANAGER_H_
#pragma once

#include "jni.h"
#include "jp_context.h"

#include <string>

using std::string;

class JPJavaFrame;
class JPClass;

using JPObjectRef = JPRef<jobject>;

/**
 * These functions will manage the cache of found type, be it primitive types, class types or the "magic" types.
 */

class JPTypeManager
{
	friend class JPContext;
public:

	/**
	 * Initialize the type manager caches
	 */
	explicit JPTypeManager(JPJavaFrame& frame);
	~JPTypeManager() = default;

	/**
	 * Find a class using a native name.
	 *
	 * The pointer returned is NOT owned by the caller
	 */
	JPClass* findClass(jclass cls);
	JPClass* findExtensionBaseClass(jclass cls);
	JPClass* findClassByName(const std::string_view& str);
	JPClass* findClassForObject(jobject obj);
	void populateMethod(void* method, jobject obj);
	void populateMembers(JPClass* cls);
    int interfaceParameterCount(JPClass* cls);

private:
	JPContext* m_Context;
	JPObjectRef m_JavaTypeManager;
	jmethodID m_FindClass;
	jmethodID m_FindExtensionBaseClass;
	jmethodID m_FindClassByName;
	jmethodID m_FindClassForObject;
	jmethodID m_PopulateMethod;
	jmethodID m_PopulateMembers;
    jmethodID m_InterfaceParameterCount;
} ;

#endif // _JPCLASS_H_
