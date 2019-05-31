/*****************************************************************************
   Copyright 2004-2008 Steve Ménard

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

jobject JPValue::getJavaObject() const
{
	if (dynamic_cast<JPPrimitiveType*> (m_Class) != m_Class)
		return m_Value.l;

	// This method is only used internally, thus it requires a logical code
	// error to trigger. We will use type error in case there is some
	// way a user can trigger it.
	JP_RAISE_TYPE_ERROR("access Java primitive value as Java object");
}

jclass JPValue::getJavaClass() const
{
	if (m_Class == NULL)
		JP_RAISE_RUNTIME_ERROR("Null class");
	return m_Class->getJavaClass();
}

