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

JPMethodDispatch::JPMethodDispatch(JPClass* clazz,
		const string& name,
		JPMethodList& overloads,
		jint modifiers)
: m_Name(name)
{
	m_Class = clazz;
	m_Overloads = overloads;
	m_Modifiers = modifiers;
}

JPMethodDispatch::~JPMethodDispatch()
{
}

const string& JPMethodDispatch::getName() const
{
	return m_Name;
}

string JPMethodDispatch::getClassName() const
{
	return m_Class->getCanonicalName();
}

JPMatch JPMethodDispatch::findOverload(JPPyObjectVector& arg, bool callInstance)
{
	JP_TRACE_IN("JPMethod::findOverload");
	JP_TRACE("Checking overload", m_Name);
	JP_TRACE("Got overloads to check", m_Overloads.size());
	JPMethodList ambiguous;
	JPMatch bestMatch;
	for (JPMethodList::iterator it = m_Overloads.begin(); it != m_Overloads.end(); ++it)
	{
		JPMethod* current = *it;

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
		if (JPModifier::isConstructor(m_Modifiers))
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
		for (vector<JPMethod*>::iterator it = ambiguous.begin(); it != ambiguous.end(); ++it)
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
		if (JPModifier::isConstructor(m_Modifiers))
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
		for (JPMethodList::iterator it = m_Overloads.begin();
				it != m_Overloads.end();
				++it)
		{
			JPMethod* current = *it;
			ss << "\t" << current->toString();
			ss << std::endl;
		}
		JP_RAISE_TYPE_ERROR(ss.str());
	}
	return bestMatch;
	JP_TRACE_OUT;
}

JPPyObject JPMethodDispatch::invoke(JPPyObjectVector& args, bool instance)
{
	JP_TRACE_IN("JPMethod::invoke");
	JPMatch match = findOverload(args, instance);
	return match.overload->invoke(match, args, instance);
	JP_TRACE_OUT;
}

JPValue JPMethodDispatch::invokeConstructor(JPPyObjectVector& arg)
{
	JPMatch currentMatch = findOverload(arg, false);
	return currentMatch.overload->invokeConstructor(currentMatch, arg);
}

string JPMethodDispatch::matchReport(JPPyObjectVector& args)
{
	stringstream res;
	res << "Match report for method " << m_Name << ", has " << m_Overloads.size() << " overloads." << endl;

	for (JPMethodList::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
	{
		JPMethod* current = *cur;
		res << "  " << current->matchReport(args);
	}
	return res.str();
}

string JPMethodDispatch::dump()
{
	stringstream res;
	for (JPMethodList::iterator cur = m_Overloads.begin(); cur != m_Overloads.end(); cur++)
	{
		JPMethod *u = *cur;
		res << u->toString() << std::endl;
		for (JPMethodList::iterator iter = u->m_MoreSpecificOverloads.begin();
				iter != u->m_MoreSpecificOverloads.end(); ++iter)
		{
			//			res << "   " << (*iter)->toString() << std::endl;
			res << "   " << *iter << std::endl;
		}
	}
	return res.str();
}
