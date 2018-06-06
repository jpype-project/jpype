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
#ifndef _JPMETHODOVERLOAD_H_
#define _JPMETHODOVERLOAD_H_

#include <list>
class JPObject;
class JPMethod;
class JPMethodOverload
{
public :
	friend class JPMethod;
	typedef std::list<JPMethodOverload*> OverloadList;

	JPMethodOverload(JPClass* claz, jobject mth);
	
	virtual ~JPMethodOverload();
	
	EMatchType              matches(bool ignoreFirst, vector<HostRef*>& args) ;

	HostRef*                invokeInstance(vector<HostRef*>& arg);

	HostRef*                invokeStatic(vector<HostRef*>& arg);

	JPObject*               invokeConstructor(jclass, vector<HostRef*>& arg);

private:
	JPMethodOverload(const JPMethodOverload& o);
	JPMethodOverload& operator=(const JPMethodOverload&);

public :	
	bool isStatic() const
	{
		return m_IsStatic;
	}
	
	bool isFinal() const
	{
		return m_IsFinal;
	}

	bool isVarArgs() const
	{
		return m_IsVarArgs;
	}

	const JPTypeName& getReturnType() const
	{
		return m_ReturnType;
	}

	unsigned char getArgumentCount() const
	{
		return (unsigned char)m_Arguments.size();
	}

	string toString();

	bool isSameOverload(JPMethodOverload& o);
	string matchReport(vector<HostRef*>& args);
	bool isMoreSpecificThan(JPMethodOverload& other) const;

	bool checkMoreSpecificThan(JPMethodOverload* other) const
	{
		for (OverloadList::const_iterator it = m_MoreSpecificOverloads.begin();
		     it != m_MoreSpecificOverloads.end();
		     ++it)
		{
			if (other == *it)
				return true;
		}
		return false;
	}

private:
  void packArgs(vector<jvalue>& v, vector<HostRef*>& arg, size_t skip);
	void ensureTypeCache() const;
private :
	JPClass*                 m_Class;
	jobject                  m_Method;
	jmethodID                m_MethodID;
	JPTypeName               m_ReturnType;
	vector<JPTypeName>       m_Arguments;
	bool                     m_IsStatic;
	bool                     m_IsFinal;
	bool                     m_IsVarArgs;
	bool                     m_IsConstructor;
	mutable vector<JPType*>  m_ArgumentsTypeCache;
	mutable JPType*          m_ReturnTypeCache;
	OverloadList             m_MoreSpecificOverloads;
	bool                     m_Ordered;
};

#endif // _JPMETHODOVERLOAD_H_
