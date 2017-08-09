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

#include <stack>
#include <set>
#include <algorithm>

JPMethod::JPMethod(jclass clazz, const string& name, bool isConstructor) :
	m_Name(name),
	m_IsConstructor(isConstructor)
{
	m_Class = (jclass)JPEnv::getJava()->NewGlobalRef(clazz);
}

const string& JPMethod::getName() const
{
	return m_Name;
}

string JPMethod::getClassName() const
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
	TRACE2("Checking overload", m_Name);
	TRACE2("Got overloads to check", m_Overloads.size());
	ensureOverloadOrderCache();
	JPMethodOverload* maximallySpecificOverload = 0;
	for (std::vector<OverloadData>::iterator it = m_OverloadOrderCache.begin(); it != m_OverloadOrderCache.end(); ++it)	
	{
		if ((! needStatic) || it->m_Overload->isStatic()) 
		{
			TRACE2("Trying to match", it->m_Overload->getSignature());
			EMatchType match = it->m_Overload->matches(false, arg);
			TRACE2("  match ended", match);
			if(match == _exact) 
			{
				return it->m_Overload;
			}
			if (match >= _implicit) 
			{
				if(!maximallySpecificOverload) 
				{
					maximallySpecificOverload = it->m_Overload;
				} 
				else 
				{
					bool isAmbiguous = std::find(it->m_MoreSpecificOverloads.begin(),
							it->m_MoreSpecificOverloads.end(),
							maximallySpecificOverload) == it->m_MoreSpecificOverloads.end();
					if(isAmbiguous) 
					{
						TRACE3("ambiguous overload", maximallySpecificOverload->getSignature(), it->m_Overload->getSignature());
						RAISE(JPypeException, "Ambiguous overloads found: " + maximallySpecificOverload->getSignature() + " vs " + it->m_Overload->getSignature());
					}
				}
			}
		}
	}
	if (!maximallySpecificOverload)
	{
		std::stringstream ss;
		ss << "No matching overloads found for " << getName() << " in find.";
		RAISE(JPypeException, ss.str());
	}
	return maximallySpecificOverload;
	TRACE_OUT;
}
void JPMethod::ensureOverloadOrderCache()
{
	if(m_Overloads.size() == m_OverloadOrderCache.size()) { return; }
	m_OverloadOrderCache.clear();
	std::vector<JPMethodOverload*> overloads;
	for (std::map<string, JPMethodOverload>::iterator it = m_Overloads.begin(); it != m_Overloads.end(); ++it) {
		overloads.push_back(&it->second);
	}


	std::vector<char> seen(overloads.size(), false);
	for (size_t i = 0; i < overloads.size(); ++i) {
		if (seen[i]) { continue; }
		std::stack<size_t> st;
		st.push(i);
		while(!st.empty()) {
			size_t curr = st.top();
			for (size_t j = 0; j < overloads.size(); ++j) {
				if (j != curr && !seen[j] &&
						overloads[j]->isMoreSpecificThan(*overloads[curr])) {
					seen[j] = true;
					st.push(j);
				}
			}
			if (curr == st.top()) {
				st.pop();
				m_OverloadOrderCache.push_back(OverloadData(overloads[curr]));
			}
		}
	}
//	std::cout << "overload order for " << m_Name << std::endl;
//	for (std::vector<OverloadData>::iterator it = m_OverloadOrderCache.begin(); it != m_OverloadOrderCache.end(); ++it) {
//		std::cout << it->m_Overload->getSignature() << std::endl;
//	}
	for (std::vector<OverloadData>::iterator it = m_OverloadOrderCache.begin(); it != m_OverloadOrderCache.end(); ++it) {
		for (std::vector<OverloadData>::iterator jt = m_OverloadOrderCache.begin(); jt != m_OverloadOrderCache.end(); ++jt) {
			if (jt != it && jt->m_Overload->isMoreSpecificThan(*it->m_Overload)) {
				it->m_MoreSpecificOverloads.push_back(jt->m_Overload);
			}
		}
	}

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
		std::stringstream ss;
		ss << "No matching member overloads found for " << getName() << ".";
		RAISE(JPypeException, ss.str());
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
