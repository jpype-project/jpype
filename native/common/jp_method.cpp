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
#include <jpype.h>
#include <algorithm>

JPMethod::JPMethod(JPClass* clazz, const string& name, bool isConstructor) : m_Name(name)
{
	m_Class = clazz;
	m_hasStatic = false;
	m_Cached = false;
	m_IsConstructor = isConstructor;
}

JPMethod::~JPMethod()
{
	for (OverloadList::iterator it = m_Overloads.begin(); it != m_Overloads.end(); ++it)
		delete *it;
}

const string& JPMethod::getName() const
{
	return m_Name;
}

string JPMethod::getClassName() const
{
	return m_Class->getCanonicalName();
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

	//	if (over->isBeanAccessor())
	//	{
	//		m_BeanAccessor = true;
	//	}
	//
	//	if (over->isBeanAccessor())
	//	{
	//		m_BeanMutator = true;
	//	}

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
			JPMethodOverload* current = *it;

			if (over->isMoreSpecificThan(*current) && !current->isMoreSpecificThan(*over))
			{
				over->m_MoreSpecificOverloads.push_back(current);
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
			JPMethodOverload* current = *it;
			if (!current->m_Ordered)
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

JPMatch JPMethod::findOverload(JPPyObjectVector& arg, bool callInstance)
{
	JP_TRACE_IN("JPMethod::findOverload");
	JP_TRACE("Checking overload", m_Name);
	JP_TRACE("Got overloads to check", m_Overloads.size());
	ensureOverloadCache();
	vector<JPMethodOverload*> ambiguous;
	JPMatch bestMatch;
	for (OverloadList::iterator it = m_Overloads.begin(); it != m_Overloads.end(); ++it)
	{
		JPMethodOverload* current = *it;

		JP_TRACE("Trying to match", current->toString());
		JPMatch match = current->matches(callInstance, arg);

		JP_TRACE("  match ended", match.type);
		if (match.type == JPMatch::_exact)
		{
			return match;
		}
		if (match.type < JPMatch::_implicit)
			continue;

		if (bestMatch.overload == 0)
		{
			bestMatch = match;
			continue;
		}

		if (callInstance && !current->isStatic() && bestMatch.overload->isStatic())
		{
			bestMatch = match;
			continue;
		}

		if (!callInstance && current->isStatic() && !bestMatch.overload->isStatic())
		{
			bestMatch = match;
			continue;
		}

		if (!(bestMatch.overload->checkMoreSpecificThan(current)))
		{
			ambiguous.push_back(*it);
		}
	}

	// If we have an ambiguous overload report an error.
	if (!ambiguous.empty())
	{
		ambiguous.push_back(bestMatch.overload);

		// We have two possible overloads so we declare an error
		std::stringstream ss;
		if (m_IsConstructor)
			ss << "Ambiguous overloads found for constructor " << m_Class->getCanonicalName() << "(";
		else
			ss << "Ambiguous overloads found for " << m_Class->getCanonicalName() << "." << getName() << "(";
		size_t start = callInstance ? 1 : 0;
		for (size_t i = start; i < arg.size(); ++i)
		{
			if (i != start)
				ss << ",";
			ss << JPPyObject::getTypeName(arg[i]);
		}
		ss << ")" << " between:" << std::endl;
		for (vector<JPMethodOverload*>::iterator it = ambiguous.begin(); it != ambiguous.end(); ++it)
		{
			ss << "\t" << (*it)->toString() << std::endl;
		}
		JP_RAISE_TYPE_ERROR(ss.str());
		JP_TRACE(ss.str());
	}

	// If we can't find a matching overload throw an error.
	if (!bestMatch.overload)
	{
		std::stringstream ss;
		if (m_IsConstructor)
			ss << "No matching overloads found for constructor " << m_Class->getCanonicalName() << "(";
		else
			ss << "No matching overloads found for " << m_Class->getCanonicalName() << "." << getName() << "(";
		size_t start = callInstance ? 1 : 0;
		for (size_t i = start; i < arg.size(); ++i)
		{
			if (i != start)
				ss << ",";
			ss << JPPyObject::getTypeName(arg[i]);
		}
		ss << ")" << ", options are:" << std::endl;
		for (OverloadList::iterator it = m_Overloads.begin();
				it != m_Overloads.end();
				++it)
		{
			JPMethodOverload* current = *it;
			ss << "\t" << current->toString();
			ss << std::endl;
		}
		JP_RAISE_TYPE_ERROR(ss.str());
	}
	return bestMatch;
	JP_TRACE_OUT;
}

JPPyObject JPMethod::invoke(JPPyObjectVector& args, bool instance)
{
	JP_TRACE_IN("JPMethod::invoke");
	JPMatch match = findOverload(args, instance);
	return match.overload->invoke(match, args);
	JP_TRACE_OUT;
}

JPValue JPMethod::invokeConstructor(JPPyObjectVector& arg)
{
	JPMatch currentMatch = findOverload(arg, false);
	return currentMatch.overload->invokeConstructor(currentMatch, arg);
}

bool JPMethod::isBeanMutator()
{
	for (OverloadList::iterator it = m_Overloads.begin(); it != m_Overloads.end(); ++it)
	{
		if ((*it)->isBeanMutator())
			return true;
	}
	return false;
}

bool JPMethod::isBeanAccessor()
{
	for (OverloadList::iterator it = m_Overloads.begin(); it != m_Overloads.end(); ++it)
	{
		if ((*it)->isBeanAccessor())
			return true;
	}
	return false;
}

string JPMethod::matchReport(JPPyObjectVector& args)
{
	stringstream res;
	res << "Match report for method " << m_Name << ", has " << m_Overloads.size() << " overloads." << endl;

	for (OverloadList::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
	{
		JPMethodOverload* current = *cur;
		res << "  " << current->matchReport(args);
	}
	return res.str();
}

string JPMethod::dump()
{
	stringstream res;
	for (OverloadList::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
	{
		JPMethodOverload *u = *cur;
		res << u->toString() << std::endl;
		for (OverloadList::iterator iter = u->m_MoreSpecificOverloads.begin();
				iter != u->m_MoreSpecificOverloads.end(); ++iter)
		{
			res << "   " << (*iter)->toString() << std::endl;
		}
	}
	return res.str();
}
