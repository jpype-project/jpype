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
#ifndef _JPOBJECT_H_
#define _JPOBJECT_H_

class JPObject : public JPObjectBase
{
public :
	JPObject(JPClass* clazz, jobject inst);
	JPObject(JPTypeName& clazz, jobject inst);
	virtual ~JPObject();
	

	JPClass* getClass()
	{
		return m_Class;
	}

	jobject      getObject()
	{
		return JPEnv::getJava()->NewLocalRef(m_Object);
	}

	JCharString toString();

	HostRef* getAttribute(string name);
	void     setAttribute(string name, HostRef* value);

private :
	JPClass* m_Class;
	jobject	 m_Object;
};

#endif // _JPOBJECT_H_
