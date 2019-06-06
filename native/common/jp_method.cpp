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
#include <jp_method.h>

JPMethod::JPMethod(JPClass* claz,
		const string& name,
		jobject mth,
		jmethodID mid,
		JPClass *returnType,
		JPClassList parameterTypes,
		JPMethodList& moreSpecific,
		jint modifiers)
: m_Method(claz->getContext(), mth)
{
	this->m_Class = claz;
	this->m_Name = name;
	this->m_MethodID = mid;
	this->m_ReturnType = returnType;
	this->m_ParameterTypes = parameterTypes;
	this->m_MoreSpecificOverloads = moreSpecific;
	this->m_Modifiers = modifiers;
}

JPMethod::~JPMethod()
{
}

string JPMethod::toString() const
{
	return m_Name;
}

JPMatch::Type matchVars(JPPyObjectVector& arg, size_t start, JPClass* vartype)
{
	JP_TRACE_IN("JPMethodOverload::matchVars");
	JPArrayClass* arraytype = (JPArrayClass*) vartype;
	JPClass* type = arraytype->getComponentType();
	size_t len = arg.size();

	JPMatch::Type lastMatch = JPMatch::_exact;
	for (size_t i = start; i < len; i++)
	{
		JPMatch::Type match = type->canConvertToJava(arg[i]);

		if (match < JPMatch::_implicit)
		{
			return JPMatch::_none;
		}
		if (match < lastMatch)
		{
			lastMatch = match;
		}
	}

	return lastMatch;
	JP_TRACE_OUT;
}

JPMatch JPMethod::matches(bool callInstance, JPPyObjectVector& arg)
{
	JP_TRACE_IN("JPMethodOverload::matches");
	JPMatch match;
	match.overload = this;

	size_t len = arg.size();
	size_t tlen = m_ParameterTypes.size();
	JP_TRACE("arguments length", len);
	JP_TRACE("types length", tlen);
	if (callInstance && isStatic())
	{
		len--;
		match.offset = 1;
	}

	if (callInstance || isInstance())
	{
		match.skip = 1;
	}

	JPMatch::Type lastMatch = JPMatch::_exact;
	if (!JPModifier::isVarArgs(m_Modifiers))
	{
		if (len != tlen)
		{
			JP_TRACE("Argument length mismatch", len, tlen);
			return match; // JPMatch::_none
		}
	}
	else
	{
		JP_TRACE("Match vargs");
		JPClass* type = m_ParameterTypes[tlen - 1];
		if (len < tlen - 1)
		{
			return match;
		}

		// Hard, could be direct array or an array.
		if (len == tlen)
		{
			// Try direct
			size_t last = tlen - 1 - match.offset;
			PyObject* obj = arg[last];
			--len;
			lastMatch = type->canConvertToJava(obj);
			if (lastMatch < JPMatch::_implicit)
			{
				// Try indirect
				lastMatch = matchVars(arg, last, type);
				match.isVarIndirect = true;
				JP_TRACE("Match vargs indirect", lastMatch);
			}
			else
			{
				match.isVarDirect = true;
				JP_TRACE("Match vargs direct", lastMatch);
			}
		}

		else if (len > tlen)
		{
			// Must match the array type
			len = tlen - 1;
			lastMatch = matchVars(arg, tlen - 1 + match.offset, type);
			match.isVarIndirect = true;
			JP_TRACE("Match vargs indirect", lastMatch);
		}

		else if (len < tlen)
		{
			match.isVarIndirect = true;
			JP_TRACE("Match vargs empty");
		}

		if (lastMatch < JPMatch::_implicit)
		{
			return match;
		}
	}

	JP_TRACE("Start match");
	for (size_t i = 0; i < len; i++)
	{
		JPClass* type = m_ParameterTypes[i];
		JPMatch::Type ematch = type->canConvertToJava(arg[i + match.offset]);
		JP_TRACE("compare", ematch, type->toString(), JPPyObject::getTypeName(arg[i + match.offset]));
		if (ematch < JPMatch::_implicit)
		{
			return match;
		}
		if (ematch < lastMatch)
		{
			lastMatch = ematch;
		}
	}

	match.type = lastMatch;
	return match;
	JP_TRACE_OUT;
}

void JPMethod::packArgs(JPMatch& match, vector<jvalue>& v, JPPyObjectVector& arg)
{
	JP_TRACE_IN("JPMethodOverload::packArgs");
	size_t len = arg.size();
	size_t tlen = m_ParameterTypes.size();
	JP_TRACE("skip", match.skip == 1);
	JP_TRACE("offset", match.offset == 1);
	JP_TRACE("arguments length", len);
	JP_TRACE("types length", tlen);
	if (match.isVarIndirect)
	{
		JP_TRACE("Pack varargs");
		len = tlen - 1;
		JPArrayClass* type = (JPArrayClass*) m_ParameterTypes[tlen - 1];
		v[tlen - 1 - match.skip] = type->convertToJavaVector(arg, tlen - 1, arg.size());
	}

	JP_TRACE("Pack fixed total=", len - match.offset);
	for (size_t i = match.skip; i < len; i++)
	{
		JPClass* type = m_ParameterTypes[i - match.offset];
		JP_TRACE("Convert", i - match.offset, i, type->getCanonicalName());
		v[i - match.skip] = type->convertToJava(arg[i]);
	}
	JP_TRACE_OUT;
}

JPPyObject JPMethod::invoke(JPMatch& match, JPPyObjectVector& arg, bool instance)
{
	JP_TRACE_IN("JPMethodOverload::invoke");
	size_t alen = m_ParameterTypes.size();
	JPJavaFrame frame(m_Class->getContext(), 8 + alen);

	JPClass* retType = m_ReturnType;

	// Pack the arguments
	vector<jvalue> v(alen + 1);
	packArgs(match, v, arg);

	// Invoke the method (arg[0] = this)
	if (JPModifier::isStatic(m_Modifiers))
	{
		jclass claz = m_Class->getJavaClass();
		return retType->invokeStatic(frame, claz, m_MethodID, &v[0]);
	}
	else
	{
		JPValue* selfObj = JPPythonEnv::getJavaValue(arg[0]);
		jobject c = selfObj->getJavaObject();
		jclass clazz = NULL;
		if (!m_Class->isAbstract() && !instance)
			clazz = m_Class->getJavaClass();
		return retType->invoke(frame, c, clazz, m_MethodID, &v[0]);
	}
	JP_TRACE_OUT;
}

JPValue JPMethod::invokeConstructor(JPMatch& match, JPPyObjectVector& arg)
{
	JP_TRACE_IN("JPMethodOverload::invokeConstructor");
	size_t alen = m_ParameterTypes.size();
	JPJavaFrame frame(m_Class->getContext(), 8 + alen);

	vector<jvalue> v(alen + 1);
	packArgs(match, v, arg);

	jvalue val;
	{
		JPPyCallRelease call;
		val.l = frame.keep(frame.NewObjectA(m_Class->getJavaClass(), m_MethodID, &v[0]));
	}
	return JPValue(m_Class, val);

	JP_TRACE_OUT;
}

string JPMethod::matchReport(JPPyObjectVector& sequence)
{
	stringstream res;

	res << m_ReturnType->getCanonicalName() << " (";

	bool isFirst = true;
	for (vector<JPClass*>::iterator it = m_ParameterTypes.begin(); it != m_ParameterTypes.end(); it++)
	{
		if (isFirst && !isStatic())
		{
			isFirst = false;
			continue;
		}
		isFirst = false;
		res << (*it)->getCanonicalName();
	}

	res << ") ==> ";

	JPMatch match = matches(!isStatic(), sequence);
	switch (match.type)
	{
		case JPMatch::_none:
			res << "NONE";
			break;
		case JPMatch::_explicit:
			res << "EXPLICIT";
			break;
		case JPMatch::_implicit:
			res << "IMPLICIT";
			break;
		case JPMatch::_exact:
			res << "EXACT";
			break;
		default:
			res << "UNKNOWN";
			break;
	}

	res << endl;

	return res.str();
}

bool JPMethod::checkMoreSpecificThan(JPMethod* other) const
{
	for (JPMethodList::const_iterator it = m_MoreSpecificOverloads.begin();
			it != m_MoreSpecificOverloads.end();
			++it)
	{
		if (other == *it)
			return true;
	}
	return false;
}
