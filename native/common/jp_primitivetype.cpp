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
#include "jp_boxedclasses.h"

JPPrimitiveType::JPPrimitiveType(JPBoxedClass* cls)
: JPClass(JPJni::getPrimitiveClass(cls->getJavaClass())), m_BoxedClass(cls)
{
	cls->setPrimitiveType(this);
}

JPPrimitiveType::~JPPrimitiveType()
{
}

bool JPPrimitiveType::isPrimitive() const
{
	return true;
}
