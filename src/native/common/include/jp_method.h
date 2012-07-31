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
#ifndef _JPMETHOD_H_
#define _JPMETHOD_H_

class JPObject;

class JPMethod
{
public :
	/**
	 * Create a new method based on class and a name;
	 */
	JPMethod(jclass clazz, string name, bool isConstructor);
	virtual ~JPMethod()
	{
		JPEnv::getJava()->DeleteGlobalRef(m_Class);
	}


public :
	string getName();
	string getClassName();
	
	void addOverload(JPClass* clazz, jobject mth);
	void addOverloads(JPMethod* o);
	
	bool hasStatic(); 
	size_t getCount()
	{
		return m_Overloads.size();
	}
	
	bool isBeanMutator();
	bool isBeanAccessor();

	HostRef*  invoke(vector<HostRef*>&); 
	HostRef*  invokeStatic(vector<HostRef*>&); 
	HostRef*  invokeInstance(vector<HostRef*>&); 
	JPObject* invokeConstructor(vector<HostRef*>& args); 

	string describe(string prefix);

	string matchReport(vector<HostRef*>&);

private :
	JPMethodOverload* findOverload(vector<HostRef*>& arg, bool needStatic);

	jclass                        m_Class;
	string                        m_Name;
	map<string, JPMethodOverload> m_Overloads;
	bool                          m_IsConstructor;
	
};

#endif // _JPMETHOD_H_

