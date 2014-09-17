/*****************************************************************************
   Copyright 2004 Steve Menard

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

JPTypeName::NativeNamesMap JPTypeName::nativeNames;
JPTypeName::DefinedTypesMap JPTypeName::definedTypes;
JPTypeName::NativeTypesMap JPTypeName::nativeTypes;

static string convertToNativeClassName(const string& str)
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
	nativeNames["void"] = "V";
	nativeNames["byte"] = "B";
	nativeNames["short"] = "S";
	nativeNames["int"] = "I";
	nativeNames["long"] = "J";
	nativeNames["float"] = "F";
	nativeNames["double"] = "D";
	nativeNames["boolean"] = "Z";
	nativeNames["char"] = "C";
	
	definedTypes["void"] = _void;
	definedTypes["byte"] = _byte;
	definedTypes["short"] = _short;
	definedTypes["int"] = _int;
	definedTypes["long"] = _long;
	definedTypes["float"] = _float;
	definedTypes["double"] = _double;
	definedTypes["boolean"] = _boolean;
	definedTypes["char"] = _char;
	definedTypes["java.lang.String"] = _string;
	definedTypes["java.lang.Class"] = _class;

	for (DefinedTypesMap::iterator it = definedTypes.begin(); it != definedTypes.end(); ++it)
	{
		nativeTypes[it->second] = it->first;
	}

}
	
JPTypeName JPTypeName::fromType(JPTypeName::ETypes t)
{
	return fromSimple(nativeTypes[t].c_str());
}

JPTypeName JPTypeName::fromSimple(const char* name)
{
	string simple = name;
	string componentName = simple;
	string nativeComponent;
	ETypes t;
		
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
	
	NativeNamesMap::iterator nativeIt = nativeNames.find(componentName);
	if (nativeIt == nativeNames.end())
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
	
	DefinedTypesMap::iterator typeIt = definedTypes.find(name);
	if (typeIt == definedTypes.end())
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

