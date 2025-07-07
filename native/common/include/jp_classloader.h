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
#ifndef _JPCLASSLOADER_H_
#define _JPCLASSLOADER_H_

#include "jp_class.h"

/**
 * The class loader is reponsible for loading classes within JPype.
 *
 * Depending on the settings it may either use an internal boot loader
 * to get the jar from within the _jpype module, or it may use the
 * system class loader.
 */
class JPClassLoader
{
public:
	/** Initialize the class loader.
	 */
	explicit JPClassLoader(JPJavaFrame& frame);

	/** Load a class by name from the jpype.jar.
	 *
	 * String is specified as a Java binary name.
	 * https://docs.oracle.com/javase/7/docs/api/java/lang/ClassLoader.html#name
	 *
	 * @returns the class loaded
	 * @throws RuntimeException if the class is not found.
	 */
	jclass findClass(JPJavaFrame& frame, const string& name);

	// Classloader for Proxy
	jobject getBootLoader();

private:
	JPClassRef m_ClassClass;
	JPObjectRef m_SystemClassLoader;
	JPObjectRef m_BootLoader;
	jmethodID m_ForNameID;
} ;

#endif // _JPCLASSLOADER_H_
