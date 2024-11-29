/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#include <algorithm>
#include <utility>
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
	m_LastCache.m_Hash = -1;
}

JPMethodDispatch::~JPMethodDispatch()
= default;

const string& JPMethodDispatch::getName() const
{
	return m_Name;
}

bool JPMethodDispatch::findOverload(JPJavaFrame& frame, JPMethodMatch &bestMatch, JPPyObjectVector& arg,
		bool callInstance, bool raise)
{
	JP_TRACE_IN("JPMethodDispatch::findOverload");
	JP_TRACE("Checking overload", m_Name);
	JP_TRACE("Got overloads to check", m_Overloads.size());
	JPMethodList ambiguous;

	// Check cache to see if we already resolved this overload.
	//   First we need to see if the hash matches for the last set of arguments.
	//   Then make sure we don't hit the rare case that the hash was -1 by chance.
	//   Then make sure it isn't variadic list match, as the hash of an opaque list
	//   element can't be resolved without going through the resolution process.
	if (m_LastCache.m_Hash == bestMatch.m_Hash && m_LastCache.m_Overload != nullptr
			&& !m_LastCache.m_Overload->isVarArgs())
	{
		bestMatch.m_Overload = m_LastCache.m_Overload;
		bestMatch.m_Overload->matches(frame, bestMatch, callInstance, arg);

		// Anything better than explicit constitutes a hit on the cache
		if (bestMatch.m_Type > JPMatch::_explicit)
			return true;
		else
			// bad match so forget the overload.
			bestMatch.m_Overload = nullptr;
	}

	// We need two copies of the match.  One to hold the best match we have
	// found, and one to hold the test of the next overload.
	JPMethodMatch match = bestMatch;

	// Check each overload in order (the TypeManager orders them by priority
	// according to Java overload rules).
	for (auto & current : m_Overloads)
	{
		JP_TRACE("Trying to match", current->toString());
		current->matches(frame, match, callInstance, arg);

		JP_TRACE("  match ended", match.m_Type);
		if (match.m_Type == JPMatch::_exact)
		{
			// We can bypass the process here as there is no better match than exact.
			bestMatch = match;
			m_LastCache = (JPMethodCache&) match; // lgtm [cpp/slicing]
			return true;
		}
		if (match.m_Type < JPMatch::_implicit)
			continue;

		// If this is the first match then make it the best.
		if (bestMatch.m_Overload == nullptr)
		{
			bestMatch = match;
			continue;
		}

		// If the best does not hide the other, than we have ambiguity.
		if (!(bestMatch.m_Overload->checkMoreSpecificThan(current)))
		{
			// See if we can match based on instance
			if (callInstance == !current->isStatic())
			{
				// if current matches instance and best does not switch
				if (callInstance == bestMatch.m_Overload->isStatic())
				{
					bestMatch = match;
					continue;
				}
			} else
			{
				// if best matches instance and current does not, no ambiguity
				if (callInstance == !bestMatch.m_Overload->isStatic())
				{
					continue;
				}
			}

			// If the best match is worse than current, switch
			if (bestMatch.m_Type < match.m_Type)
			{
				bestMatch = match;
				// Remove any prior ambiguity
				ambiguous.clear();
				continue;
			}

			// If the best match is still better than current, keep it
			if (bestMatch.m_Type > match.m_Type)
			{
				continue;
			}

			JP_TRACE("Adding to ambiguous list");

			// Keep trace of ambiguous overloads for the error report.
			ambiguous.push_back(current);
		}
	}

	// If we have an ambiguous overload report an error.
	if (!ambiguous.empty())
	{
		if (!raise)
			return false;
		ambiguous.push_back(bestMatch.m_Overload);

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
			ss << Py_TYPE(arg[i])->tp_name;
		}
		ss << ")" << " between:" << std::endl;
		for (auto & ambiguous_mthd : ambiguous)
		{
			ss << "\t" << ambiguous_mthd->toString() << std::endl;
		}
		JP_RAISE(PyExc_TypeError, ss.str());
		JP_TRACE(ss.str());
	}

	// If we can't find a matching overload throw an error.
	if (!bestMatch.m_Overload)
	{
		if (!raise)
			return false;
		std::stringstream ss;
		if (JPModifier::isConstructor(m_Modifiers))
			ss << "No matching overloads found for constructor " << m_Class->getCanonicalName() << "(";
		else
		{
			ss << "No matching overloads found for ";
			if (!callInstance)
				ss << "*static* ";
			ss << m_Class->getCanonicalName() << "." << getName() << "(";
		}
		size_t start = callInstance ? 1 : 0;
		for (size_t i = start; i < arg.size(); ++i)
		{
			if (i != start)
				ss << ",";
			ss << Py_TYPE(arg[i])->tp_name;
		}
		ss << ")" << ", options are:" << std::endl;
		for (auto current : m_Overloads)
		{
				ss << "\t" << current->toString();
			ss << std::endl;
		}
		JP_RAISE(PyExc_TypeError, ss.str());
	}

	// Set up a cache to bypass repeated calls.
	if (bestMatch.m_Type == JPMatch::_implicit)
	{
		m_LastCache = (JPMethodCache&) bestMatch; // lgtm [cpp/slicing]
	}

	JP_TRACE("Best match", bestMatch.m_Overload->toString());
	return true;
	JP_TRACE_OUT;
}

JPPyObject JPMethodDispatch::invoke(JPJavaFrame& frame, JPPyObjectVector& args, bool instance)
{
	JP_TRACE_IN("JPMethodDispatch::invoke");
	JPMethodMatch match(frame, args, instance);
	findOverload(frame, match, args, instance, true);
	return match.m_Overload->invoke(frame, match, args, instance);
	JP_TRACE_OUT;
}

JPValue JPMethodDispatch::invokeConstructor(JPJavaFrame& frame, JPPyObjectVector& args)
{
	JP_TRACE_IN("JPMethodDispatch::invokeConstructor");
	JPMethodMatch match(frame, args, false);
	findOverload(frame, match, args, false, true);
	return match.m_Overload->invokeConstructor(frame, match, args);
	JP_TRACE_OUT;
}

bool JPMethodDispatch::matches(JPJavaFrame& frame, JPPyObjectVector& args, bool instance)
{
	JP_TRACE_IN("JPMethodDispatch::invoke");
	JPMethodMatch match(frame, args, instance);
	return findOverload(frame, match, args, instance, false);
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}

string JPMethodDispatch::matchReport(JPPyObjectVector& args)
{
	std::stringstream res;
	res << "Match report for method " << m_Name << ", has " << m_Overloads.size() << " overloads." << std::endl;

	for (auto current : m_Overloads)
	{
			res << "  " << current->matchReport(args);
	}
	return res.str();
}

