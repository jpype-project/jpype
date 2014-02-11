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
#include <jpype.h>
#undef JPYPE_TRACING_INTERNAL

//AT's on porting:
//  1) the original definition of global static object leads to crashing
//on HP-UX platform. Cause: it is suspected to be an undefined order of initialization of static objects
//
//  2) TODO: in any case, use of static objects may impose problems in multi-threaded environment.
//Therefore, they must be guarded with a mutex.
map<string, string>& GetNativeNamesMap()
{
	static map<string, string> nativeNames;
	return nativeNames;
}

map<string, JPTypeName::ETypes>& GetDefinedTypesMap()
{                         
	static map<string, JPTypeName::ETypes> definedTypes;
	return definedTypes;
}

map<JPTypeName::ETypes, string>& GetNativeTypesMap()
{
	static map<JPTypeName::ETypes, string> nativeTypes;
	return nativeTypes;
}


static string convertToNativeClassName(const std::string& str)
{
	// simple names are of the form :
	//   a.c.d.E
	
	// native names are of the form :
	//   La/c/d/E;
	
	string name = str;
	string result = string("L")+name+";";
	for (unsigned int j = 0; j < result.length(); j++)
	{
		if (result[j] == '.') 
		{
			result[j] = '/';
		}
	}
	
	return result;
}

void JPTypeName::init()
{
	GetNativeNamesMap()["void"] = "V";
	GetNativeNamesMap()["byte"] = "B";
	GetNativeNamesMap()["short"] = "S";
	GetNativeNamesMap()["int"] = "I";
	GetNativeNamesMap()["long"] = "J";
	GetNativeNamesMap()["float"] = "F";
	GetNativeNamesMap()["double"] = "D";
	GetNativeNamesMap()["boolean"] = "Z";
	GetNativeNamesMap()["char"] = "C";
	
	GetDefinedTypesMap()["void"] = _void;
	GetDefinedTypesMap()["byte"] = _byte;
	GetDefinedTypesMap()["short"] = _short;
	GetDefinedTypesMap()["int"] = _int;
	GetDefinedTypesMap()["long"] = _long;
	GetDefinedTypesMap()["float"] = _float;
	GetDefinedTypesMap()["double"] = _double;
	GetDefinedTypesMap()["boolean"] = _boolean;
	GetDefinedTypesMap()["char"] = _char;
	GetDefinedTypesMap()["java.lang.String"] = _string;
	GetDefinedTypesMap()["java.lang.Class"] = _class;

	for (map<string, JPTypeName::ETypes>::iterator it = GetDefinedTypesMap().begin(); it != GetDefinedTypesMap().end(); ++it)
	{
		GetNativeTypesMap()[it->second] = it->first;
	}

}
	
JPTypeName JPTypeName::fromType(JPTypeName::ETypes t)
{
	return fromSimple(GetNativeTypesMap()[t].c_str());
}

JPTypeName JPTypeName::fromSimple(const char* name)
{
	string simple = name;
	string componentName = simple;
	string nativeComponent;
	JPTypeName::ETypes t;
		
	// is it an array?
	size_t arrayDimCount = 0;
	if (simple.length() > 0 && simple[simple.length()-1] == ']')
	{
		size_t i = simple.length()-1;
		while (simple[i] == ']' || simple[i] == '[')
		{
			i--;
		}
		
		componentName = simple.substr(0, i+1);
		arrayDimCount = (simple.length() - componentName.length())/2;
	}
	
	map<string, string>::iterator nativeIt = GetNativeNamesMap().find(componentName);
	if (nativeIt == GetNativeNamesMap().end())
	{
		nativeComponent = convertToNativeClassName(componentName);		
	}
	else
	{
		nativeComponent = nativeIt->second;
	}
	
	string native;
	if (arrayDimCount > 0)
	{
		stringstream str;
		for (unsigned int i = 0; i < arrayDimCount; i++)
		{
			str << "[";
		}
		str << nativeComponent;
		native = str.str();
	}
	else
	{
		native = nativeComponent;
	}
	
	map<string, JPTypeName::ETypes>::iterator typeIt = GetDefinedTypesMap().find(name);
	if (typeIt == GetDefinedTypesMap().end()) 
	{
		if (native[0] == '[') 
		{
			t = _array;
		}
		else
		{
			t = _object;
		}
	}
	else
	{
		t = typeIt->second;
	}
	
	return JPTypeName(simple, native, t);
}
	
JPTypeName JPTypeName::getComponentName() const
{
	if (m_Type != _array)
	{
		RAISE(JPypeException, "Not an array type");
	}
	
	string sname = m_SimpleName.substr(0,m_SimpleName.length()-2);
	JPTypeName compName = fromSimple(sname.c_str());
	return compName;
}

