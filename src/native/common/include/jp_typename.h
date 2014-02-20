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
		 m_SimpleName(""), m_NativeName(""), m_Type(_unknown)
	{
	}
	
private :
	JPTypeName(const string& simple, const string& native, ETypes t):
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
	
	JPTypeName& operator=(const JPTypeName& other) {
		this->m_SimpleName = other.getSimpleName();
		this->m_NativeName = other.getNativeName();
		this->m_Type = other.getType();
		return *this;
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
	
	const string& getSimpleName() const
	{
		return m_SimpleName;
	}
	
	const string& getNativeName() const
	{
		return m_NativeName;
	}

	JPTypeName getComponentName() const;

	ETypes getType() const
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

	typedef map<string, string> NativeNamesMap;
	typedef map<string, ETypes> DefinedTypesMap;
	typedef map<ETypes, string> NativeTypesMap;

	static NativeNamesMap nativeNames;
	static DefinedTypesMap definedTypes;
	static NativeTypesMap nativeTypes;
};

#endif // _JPTYPENAME_H_
