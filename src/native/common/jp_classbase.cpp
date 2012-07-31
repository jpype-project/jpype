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

JPClassBase::JPClassBase(const JPTypeName& tname, jclass c) :
	JPObjectType(JPTypeName::_unknown, JPTypeName::fromType(JPTypeName::_void)),
	m_Name(tname)
{
	m_Class = (jclass)JPEnv::getJava()->NewGlobalRef(c);
}

JPClassBase::~JPClassBase()
{
	JPEnv::getJava()->DeleteGlobalRef(m_Class);	
}
