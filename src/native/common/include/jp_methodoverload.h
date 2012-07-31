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
#ifndef _JPMETHODOVERLOAD_H_
#define _JPMETHODOVERLOAD_H_

class JPObject;
class JPMethodOverload
{
public :
	JPMethodOverload();
	JPMethodOverload(const JPMethodOverload& o);
	JPMethodOverload(JPClass* claz, jobject mth);
	
	virtual ~JPMethodOverload();
	
	EMatchType              matches(bool ignoreFirst, vector<HostRef*>& args) ;

	HostRef*                invokeInstance(vector<HostRef*>& arg);

	HostRef*                invokeStatic(vector<HostRef*>& arg);

	JPObject*               invokeConstructor(jclass, vector<HostRef*>& arg);

public :	
	string getSignature();

	bool isStatic()
	{
		return m_IsStatic;
	}
	
	bool isFinal()
	{
		return m_IsFinal;
	}

	JPTypeName getReturnType()
	{
		return m_ReturnType;
	}

	unsigned char getArgumentCount()
	{
		return (unsigned char)m_Arguments.size();
	}

	string getArgumentString();

	bool isSameOverload(JPMethodOverload& o);
	string matchReport(vector<HostRef*>& args);

private :
	JPClass*               m_Class;
	jobject                m_Method;
	jmethodID              m_MethodID;
	JPTypeName             m_ReturnType;
	vector<JPTypeName>     m_Arguments;
	bool                   m_IsStatic;
	bool                   m_IsFinal;
	bool                   m_IsConstructor;
};

#endif // _JPMETHODOVERLOAD_H_
