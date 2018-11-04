/*****************************************************************************
   Copyright 2004 Steve M�nard

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

#include <list>
class JPObject;

class JPMethod
{
public :
	typedef std::list<JPMethodOverload*> OverloadList;

	/**
	 * Create a new method based on class and a name;
	 */
	JPMethod(jclass clazz, const string& name, bool isConstructor);
	virtual ~JPMethod();

private:
	JPMethod(const JPMethod& method);
	JPMethod& operator=(const JPMethod& method);

public :
	const string& getName() const;
	string getClassName() const;
	
	void addOverload(JPClass* clazz, jobject mth);
	
	bool hasStatic() 
	{
		return m_hasStatic;
	}

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
	JPMethodOverload* findOverload(vector<HostRef*>& arg, bool searchStatic, bool searchInstance);
	void ensureOverloadCache();
	void dumpOverloads();

	jclass                        m_Class;
	string                        m_Name;
	OverloadList  m_Overloads;
	bool                          m_IsConstructor;
	bool          m_hasStatic;
	bool          m_Cached;
	
};

#endif // _JPMETHOD_H_

