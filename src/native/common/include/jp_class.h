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
#ifndef _JPCLASS_H_
#define _JPCLASS_H_

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPClass : public JPClassBase
{
public :
	JPClass(const JPTypeName& tname, jclass c);
	virtual~ JPClass();

public :
	/** 
	 * Called to fully load base classes and members 
	 */
	void postLoad();
	
	HostRef*                getStaticAttribute(string attr_name);
	void                    setStaticAttribute(string attr_name, HostRef* val);
	
	JPObject*               newInstance(vector<HostRef*>& args);
	
	JPField*                getInstanceField(const string& name);
	JPField*                getStaticField(const string& name);
	JPMethod*				getMethod(const string& name);
	vector<JPMethod*>		getMethods()
	{
		vector<JPMethod*> res;
		for (map<string, JPMethod*>::iterator cur = m_Methods.begin(); cur != m_Methods.end(); cur++)
		{
			res.push_back(cur->second);
		}
		return res;
	}

	jclass getClass()
	{
		return (jclass)JPEnv::getJava()->NewGlobalRef(m_Class);
	}
	
	map<string, JPField*>& getStaticFields()
	{
		return m_StaticFields;
	}
	
	map<string, JPField*>& getInstanceFields()
	{
		return m_InstanceFields;
	}

	bool isFinal();
	bool isAbstract();
	bool isInterface()
	{
		return m_IsInterface;
	}

	JPClass* getSuperClass();
	vector<JPClass*> getInterfaces();

	bool isSubclass(JPClass*);
	
	string describe();

public : // JPType implementation
	virtual HostRef*   asHostObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
private :
	void loadSuperClass();	
	void loadSuperInterfaces();	
	void loadFields();	
	void loadMethods();	
	void loadConstructors();	

	jvalue buildObjectWrapper(HostRef* obj);

private :
	bool                    m_IsInterface;
	JPClass*				m_SuperClass;
	vector<JPClass*>		m_SuperInterfaces;
	map<string, JPField*>   m_StaticFields;
	map<string, JPField*>   m_InstanceFields;
	map<string, JPMethod*>	m_Methods;
	JPMethod*				m_Constructors;
};


#endif // _JPCLASS_H_
