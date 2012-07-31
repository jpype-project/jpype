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
 * This class will manage the cache of found type, be it primitive types, class types or the "magic" types.
 */
class JPTypeManager
{
public :
	/**
	 * Initialize the type manager caches
	 */
	static void                         init();
	
	static JPType*                      getType(JPTypeName& name);
	
	/**
	 * The pointer returned is NOT owned by the caller
	 */
	static JPClass*					   findClass(JPTypeName&);

	/**
	 * The pointer returned is NOT owned by the caller
	 */
	static JPArrayClass*			   findArrayClass(JPTypeName&);

	static void                        flushCache();

	static int                         getLoadedClasses();
	
public:
};

#endif // _JPCLASS_H_
