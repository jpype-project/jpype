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
#ifndef _JPCLASS_BASE_H_
#define _JPCLASS_BASE_H_

/**
 * Base class for Java Class based types
 */
class JPClassBase : public JPObjectType
{
protected :
	JPClassBase(const JPTypeName& tname, jclass c);	
	virtual ~JPClassBase();
	
public : // JPType implementation
	virtual JPTypeName  getName()
	{
		return m_Name;
	}

	virtual JPTypeName  getObjectType()
	{
		return m_Name;
	}

	virtual jclass getClass()
	{
		return (jclass)JPEnv::getJava()->NewLocalRef(m_Class);
	}

protected :
	JPTypeName                 m_Name;
	jclass					   m_Class;
};

#endif // _JPCLASS_BASE_H_
