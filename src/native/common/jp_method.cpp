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

JPMethod::JPMethod(jclass clazz, string name, bool isConstructor) :
	m_Name(name),
	m_IsConstructor(isConstructor)
{
	m_Class = (jclass)JPEnv::getJava()->NewGlobalRef(clazz);
}

string JPMethod::getName()
{
	return m_Name;
}

string JPMethod::getClassName()
{
	JPTypeName name = JPJni::getClassName(m_Class);
	return name.getSimpleName();
}

bool JPMethod::hasStatic()
{
	for (map<string, JPMethodOverload>::iterator it = m_Overloads.begin(); it != m_Overloads.end(); it++)
	{
		if (it->second.isStatic())
		{
			return true;
		}
	}
	return false;
}
	
void JPMethod::addOverload(JPClass* claz, jobject mth) 
{
	JPMethodOverload over(claz, mth);

	m_Overloads[over.getSignature()] = over;	
}

void JPMethod::addOverloads(JPMethod* o)
{
	TRACE_IN("JPMethod::addOverloads");
	
	for (map<string, JPMethodOverload>::iterator it = o->m_Overloads.begin(); it != o->m_Overloads.end(); it++)
	{
		bool found = false;
		for (map<string, JPMethodOverload>::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
		{
			if (cur->second.isSameOverload(it->second))
			{
				found = true;
				break;
			}
		}

		if (! found)
		{
			// Add it only if we do not already overload it ...
			m_Overloads[it->first] = it->second;	
		}
	}
	TRACE_OUT;
}

JPMethodOverload* JPMethod::findOverload(vector<HostRef*>& arg, bool needStatic)
{
	TRACE_IN("JPMethod::findOverload");
	TRACE2("Got overlaod to check", m_Overloads.size());
	
	JPMethodOverload* currentMatch = NULL;
	EMatchType isExact = _none;
		
	for (map<string, JPMethodOverload>::iterator it = m_Overloads.begin(); it != m_Overloads.end(); it++)
	{
		if ((! needStatic) || it->second.isStatic())
		{
			TRACE2("Trying t match", it->second.getSignature());
			EMatchType match = it->second.matches(false, arg);
			TRACE2("  match ended", match);
			if (match >= isExact && match >= _implicit)
			{
				if (currentMatch == NULL)
				{
					currentMatch = &(it->second);
					isExact = match;
				}
				else if (match == _exact && isExact != _exact)
				{
					currentMatch = &(it->second);  
					isExact = _exact;
				}
				else {
					RAISE(JPypeException, "Multiple overloads possible.");
				}			
			}
			else
			{
				TRACE1(" Match is smaller than exact AND explicit");
			}
		}			
	}
	
	if (currentMatch == NULL)
	{
		RAISE(JPypeException, "No matching overloads found.");
	}

	return currentMatch;
	TRACE_OUT;
}

HostRef* JPMethod::invoke(vector<HostRef*>& args)
{
	JPMethodOverload* currentMatch = findOverload(args, false);
	HostRef* res;

	if (currentMatch->isStatic())
	{	
		res = currentMatch->invokeStatic(args);	
	}
	else
	{
		res = currentMatch->invokeInstance(args);	
	}

	return res;
}

HostRef* JPMethod::invokeInstance(vector<HostRef*>& args)
{
	JPMethodOverload* currentMatch = findOverload(args, false);
	
	if (currentMatch->isStatic())
	{	
		RAISE(JPypeException, "No matching overloads found.");
	}
	else
	{
		return currentMatch->invokeInstance(args);	
	}
}

HostRef* JPMethod::invokeStatic(vector<HostRef*>& args)
{
	JPMethodOverload* currentMatch = findOverload(args, true);
	return currentMatch->invokeStatic(args);
}

JPObject* JPMethod::invokeConstructor(vector<HostRef*>& arg)
{
	JPMethodOverload* currentMatch = findOverload(arg, false);
	return currentMatch->invokeConstructor(m_Class, arg);	
}

string JPMethod::describe(string prefix)
{
	string name = m_Name;
	if (name == "[init")
	{
		name = "__init__";
	}
	stringstream str;
	for (map<string, JPMethodOverload>::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
	{
		str << prefix << "public ";
		if (! m_IsConstructor)
		{
			if (cur->second.isStatic())
			{
				str << "static ";
			}
			else if (cur->second.isFinal())
			{
				str << "final ";
			}

			str << cur->second.getReturnType().getSimpleName() << " ";
		}
		str << name << cur->second.getArgumentString() << ";" << endl;
	}
	return str.str();
}

bool JPMethod::isBeanMutator()
{
	for (map<string, JPMethodOverload>::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
	{
		if ( (! cur->second.isStatic()) && cur->second.getReturnType().getSimpleName() == "void" && cur->second.getArgumentCount() == 2)
		{
			return true;
		}
	}
	return false;
}

bool JPMethod::isBeanAccessor()
{
	for (map<string, JPMethodOverload>::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
	{
		if ( (! cur->second.isStatic()) && cur->second.getReturnType().getSimpleName() != "void" && cur->second.getArgumentCount() == 1)
		{
			return true;
		}
	}
	return false;
}

string JPMethod::matchReport(vector<HostRef*>& args)
{
	stringstream res;
	res << "Match report for method " << m_Name << ", has " << m_Overloads.size() << " overloads." << endl;

	for (map<string, JPMethodOverload>::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
	{
		res << "  " << cur->second.matchReport(args);
	}

	return res.str();
}
