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

#include <algorithm>

JPMethod::JPMethod(jclass clazz, const string& name, bool isConstructor) :
	m_Name(name),
	m_IsConstructor(isConstructor),
	m_hasStatic(false),
	m_Cached(false)
{
	JPJavaFrame frame;
	m_Class = (jclass)frame.NewGlobalRef(clazz);
}

JPMethod::~JPMethod()
{
	JPJavaFrame::ReleaseGlobalRef(m_Class);
	for (OverloadList::iterator it = m_Overloads.begin(); it != m_Overloads.end(); ++it)
		delete *it;
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

void JPMethod::addOverload(JPClass* claz, jobject mth)
{
//	printf("JPMethodOverload %s\n", JPJni::toStringC(mth));
	JPMethodOverload* over = new JPMethodOverload(claz, mth);

	// The same overload can be repeated each time it is overriden. 
	bool found = false;
  for (OverloadList::iterator it = m_Overloads.begin(); it != m_Overloads.end(); ++it)
	{
		if (over->isSameOverload(**it))
		{
			found = true;
			break;
		}
	}
	if (found)
	{
		delete over;
		return;
}
	m_Overloads.push_back(over);
	m_Cached = false;

	// Cache if this method has a static method
	if (over->isStatic())
{
		m_hasStatic = true;
	}

	// We can't check the order of specificity here because we do not want to 
	// load the argument types here.  Thus we need to wait for the first
	// used. 
}

void JPMethod::ensureOverloadCache()
{
  if (m_Cached)
		return;
	m_Cached = true;

  for (OverloadList::iterator iter2 = m_Overloads.begin(); iter2 != m_Overloads.end(); ++iter2)
	{
		JPMethodOverload* over = *iter2;
		over->m_Ordered = false;
		for (OverloadList::iterator it = m_Overloads.begin(); it != m_Overloads.end(); ++it)
		{
			if (it == iter2)
				continue;

			if (over->isMoreSpecificThan(**it) && !(*it)->checkMoreSpecificThan(over))
			{
				over->m_MoreSpecificOverloads.push_back(*it);
			}
		}
	}

	// Execute a graph sort problem so that the most specific are always on the front
	OverloadList unsorted(m_Overloads);
	m_Overloads.clear();
	while (!unsorted.empty())
	{
		// Remove the first unsorted element
		JPMethodOverload* front = unsorted.front();
		unsorted.pop_front();

		// Check to see if all dependencies are already ordered
		int good = true;
		for (OverloadList::iterator it = front->m_MoreSpecificOverloads.begin(); 
				it != front->m_MoreSpecificOverloads.end(); ++it)
		{
			if (!(*it)->m_Ordered)
			{
				good = false;
				break;
			}
		}

		// If all dependencies are included
		if (good)
		{
			// We can put it at the front of the ordered list
			front->m_Ordered = true;
			m_Overloads.push_front(front);
		}
		else
		{
			// Otherwsie, we will defer it
			unsorted.push_back(front);
		}
	}
}

JPMethodOverload* JPMethod::findOverload(vector<HostRef*>& arg, bool searchStatic, bool searchInstance)
{
	TRACE_IN("JPMethod::findOverload");
	TRACE2("Checking overload", m_Name);
	TRACE2("Got overloads to check", m_Overloads.size());
	ensureOverloadCache();
	JPMethodOverload* maximallySpecificOverload = NULL;
	OverloadList ambiguous;
	for (OverloadList::iterator it = m_Overloads.begin(); it != m_Overloads.end(); ++it)
	{
		if ( (!searchStatic && (*it)->isStatic() ) || (!searchInstance && !(*it)->isStatic()) )
			continue;

		TRACE2("Trying to match", (*it)->toString());
		EMatchType match = (*it)->matches(false, arg);
			TRACE2("  match ended", match);
			if(match == _exact)
			{
			return *it;
			}
		if (match < _implicit)
			continue;

		if (maximallySpecificOverload==0)
			{
			maximallySpecificOverload = *it;
		}
		else if (maximallySpecificOverload->checkMoreSpecificThan(*it))
				{
			// We already had a more specific overload
				}
				else
				{
			ambiguous.push_back(*it);
		}
	}

	// If we have an ambiguous overload report an error.
	if (!ambiguous.empty())
	{
		ambiguous.push_front(maximallySpecificOverload);

			// We have two possible overloads so we declare an error
			std::stringstream ss;
			if (m_IsConstructor)
				ss << "Ambiguous overloads found for constructor " << JPJni::toStringC(m_Class) <<"(";
			else
				ss << "Ambiguous overloads found for " << JPJni::toStringC(m_Class)<<"."<< getName() <<"(";
			size_t start = searchStatic?0:1;
			for (size_t i=start; i<arg.size(); ++i)
					{
				if (i!=start)
					ss<< ",";
				ss << JPEnv::getHost()->getTypeName(arg[i]);
			}
	    ss <<")"<< " between:" << std::endl;
		for (OverloadList::iterator it = ambiguous.begin(); it != ambiguous.end(); ++it)
		{
			ss << "\t" << (*it)->toString()<< std::endl;
		}
		RAISE(JPypeException, ss.str());
		TRACE1(ss.str());
	}

	// If we can't find a matching overload throw an error.
	if (!maximallySpecificOverload)
	{
		std::stringstream ss;
		if (m_IsConstructor)
			ss << "No matching overloads found for constructor " << JPJni::toStringC(m_Class) <<"(";
		else
			ss << "No matching overloads found for " << JPJni::toStringC(m_Class)<<"."<< getName() <<"(";
		size_t start = searchStatic?0:1;
		for (size_t i=start; i<arg.size(); ++i)
		{
			if (i!=start)
				ss<< ",";
			ss << JPEnv::getHost()->getTypeName(arg[i]);
		}
	        ss <<")"<< ", options are:" << std::endl;
		for (OverloadList::iterator it = m_Overloads.begin();
				it != m_Overloads.end();
				++it)
		{
				ss << "\t" << (*it)->toString();
				ss << std::endl;
		}
		RAISE(JPypeException, ss.str());
	}
	return maximallySpecificOverload;
	TRACE_OUT;
}

HostRef* JPMethod::invoke(vector<HostRef*>& args)
{
	JPMethodOverload* currentMatch = findOverload(args, true, true);
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
	JPMethodOverload* currentMatch = findOverload(args, false, true);

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
	JPMethodOverload* currentMatch = findOverload(args, true, false);
	return currentMatch->invokeStatic(args);
}

JPObject* JPMethod::invokeConstructor(vector<HostRef*>& arg)
{
	JPMethodOverload* currentMatch = findOverload(arg, true, true);
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
	for (OverloadList::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
			{
		str << prefix << (*cur)->toString() << endl;
	}
	return str.str();
}

bool JPMethod::isBeanMutator()
{
	for (OverloadList::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
	{
		if ( (! (*cur)->isStatic()) && (*cur)->getReturnType().getSimpleName() == "void" && (*cur)->getArgumentCount() == 2)
		{
			return true;
		}
	}
	return false;
}

bool JPMethod::isBeanAccessor()
{
	for (OverloadList::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
	{
		if ( (! (*cur)->isStatic()) && (*cur)->getReturnType().getSimpleName() != "void" && (*cur)->getArgumentCount() == 1)
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

	for (OverloadList::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
	{
		res << "  " << (*cur)->matchReport(args);
	}

	return res.str();
}
