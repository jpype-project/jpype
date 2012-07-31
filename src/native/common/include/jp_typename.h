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
#ifndef _JPTYPENAME_H_
#define _JPTYPENAME_H_  

/**
 * Enum for all kinds of well-known types. 
 */

/**
 * Since types have multiple name representation (simple, native) and a type, this class encapsulates it all.
 */

class JPTypeName
{
public:
//AT's comments on porting:
// 1) Originally this ETypes enum was declraed outside of this JPTypeName class.
// This, however, broke compilation on AIX platform because of a conflict with some system headers;
// As a compromise, it was made part of JPTypeName
        enum ETypes {
             _unknown,
             _void,
             _byte, _short, _int, _long,
             _float, _double,
             _char,
             _boolean,

             _object,
             _class,
             _string,
             _array
        };

	JPTypeName() :
		m_Type(_unknown)
	{
	}
	
private :
	JPTypeName(string simple, string native, ETypes t): 
		m_SimpleName(simple), 
		m_NativeName(native), 
		m_Type(t)
	{
	}
	
public :
	/** Copy Constructor */
	JPTypeName(const JPTypeName& tn) : 
		m_SimpleName(tn.m_SimpleName), 
		m_NativeName(tn.m_NativeName), 
		m_Type(tn.m_Type)
	{
	}
	
	/** Destructor */
	virtual ~JPTypeName() 
	{}
	
public :
	/**
	 * Initialize the cache of type-name to ETypes
	 */
	static void init();
	
	/** Factory method from a simple, human-readable name */
	static JPTypeName fromSimple(const char* name);
	static JPTypeName fromType(ETypes t);
	
	string getSimpleName()
	{
		return m_SimpleName;
	}
	
	string getNativeName()
	{
		return m_NativeName;
	}

	JPTypeName getComponentName();

	ETypes getType()
	{
		return m_Type;
	}

	bool isObjectType()
	{
		return m_Type >= _object;
	}

private :
	string m_SimpleName;
	string m_NativeName;
	ETypes m_Type;	
};

#endif // _JPTYPENAME_H_
