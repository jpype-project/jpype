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
#include <algorithm>
#include "jpype.h"
#include "jp_method.h"
#include "jp_methoddispatch.h"

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

void JPMethodDispatch::findOverload(JPJavaFrame& frame, JPMethodMatch &bestMatch, JPPyObjectVector& arg, bool callInstance)
{
	JP_TRACE_IN("JPMethodDispatch::findOverload");
	JP_TRACE("Checking overload", m_Name);
	JP_TRACE("Got overloads to check", m_Overloads.size());
	JPMethodList ambiguous;
	JPMethodMatch match(frame, arg);
	for (JPMethodList::iterator it = m_Overloads.begin(); it != m_Overloads.end(); ++it)
	{
		JPMethod* current = *it;

		JP_TRACE("Trying to match", current->toString());
		current->matches(frame, match, callInstance, arg);

		JP_TRACE("  match ended", match.type);
		if (match.type == JPMatch::_exact)
		{
			bestMatch = match;
			return;
		}
		if (match.type < JPMatch::_implicit)
			continue;

		// If this is the first match then make it the best.
		if (bestMatch.overload == 0)
		{
			bestMatch = match;
			continue;
		}

		// If the best does not hide the other, than we have ambiguity.
		if (!(bestMatch.overload->checkMoreSpecificThan(current)))
		{
			// See if we can match based on instance
			if (callInstance == !current->isStatic())
			{
				// if current matches instance and best does not switch
				if (callInstance == bestMatch.overload->isStatic())
				{
					bestMatch = match;
					continue;
				}
			} else
			{
				// if best matches instance and current does not, no ambiguity
				if (callInstance == !bestMatch.overload->isStatic())
				{
					continue;
				}
			}

			JP_TRACE("Adding to ambiguous list");
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
		JP_RAISE(PyExc_TypeError, ss.str());
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
		JP_RAISE(PyExc_TypeError, ss.str());
	}

	JP_TRACE("Best match", bestMatch.overload->toString());
	return;
	JP_TRACE_OUT;
}

JPPyObject JPMethodDispatch::invoke(JPJavaFrame& frame, JPPyObjectVector& args, bool instance)
{
	JP_TRACE_IN("JPMethodDispatch::invoke");
	JPMethodMatch match(frame, args);
	findOverload(frame, match, args, instance);
	return match.overload->invoke(frame, match, args, instance);
	JP_TRACE_OUT;
}

JPValue JPMethodDispatch::invokeConstructor(JPJavaFrame& frame, JPPyObjectVector& args)
{
	JP_TRACE_IN("JPMethodDispatch::invokeConstructor");
	JPMethodMatch match(frame, args);
	findOverload(frame, match, args, false);
	return match.overload->invokeConstructor(frame, match, args);
	JP_TRACE_OUT;
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

