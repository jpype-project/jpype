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
#ifndef _JPARRAYCLASS_H_
#define _JPARRAYCLASS_H_

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPArrayClass : public JPClassBase
{
public :
	JPArrayClass(const JPTypeName& tname, jclass c);
	virtual~ JPArrayClass();

public : // JPType implementation
	virtual HostRef*    asHostObject(jvalue val);
	virtual EMatchType  canConvertToJava(HostRef* obj);
	virtual jvalue      convertToJava(HostRef* obj);
	virtual JPType*     getComponentType()
	{
		return m_ComponentType;
	}

	JPArray* newInstance(int length);

public :
	JPType* m_ComponentType;
};


#endif // _JPARRAYCLASS_H_
