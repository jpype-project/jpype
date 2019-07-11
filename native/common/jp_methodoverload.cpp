/*****************************************************************************
   Copyright 2004 Steve Mï¿½nard

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

JPMethodOverload::JPMethodOverload(JPClass* claz, jobject mth) : m_Method(mth)
{
	JPJavaFrame frame;
	m_Class = claz;
	m_ReturnTypeCache = NULL;

	// static
	m_IsStatic = JPJni::isMemberStatic(mth);
	m_IsFinal = JPJni::isMemberFinal(mth);
	m_IsVarArgs = JPJni::isMethodVarArgs(mth);
	m_IsAbstract = JPJni::isMemberAbstract(mth);

	// Method ID
	m_MethodID = frame.FromReflectedMethod(m_Method.get());

	m_IsConstructor = JPJni::isConstructor(mth);

	// return type
	if (!m_IsConstructor)
	{
		m_ReturnType = JPJni::getMethodReturnType(mth);
	}

	// arguments
	m_Arguments = JPJni::getMethodParameterTypes(mth, m_IsConstructor);
	// Add the implicit "this" argument
	if (!m_IsStatic && !m_IsConstructor)
	{
		m_Arguments.insert(m_Arguments.begin(), 1, claz->getJavaClass());
	}
        
        m_CallerSensitive = JPTypeManager::isCallerSensitive(m_Method.get());

}

JPMethodOverload::~JPMethodOverload()
{
}

string JPMethodOverload::toString() const
{
	return JPJni::toString(m_Method.get());
}

bool JPMethodOverload::isSameOverload(JPMethodOverload& o)
{
	if (isStatic() != o.isStatic())
	{
		return false;
	}

	if (m_Arguments.size() != o.m_Arguments.size())
	{
		return false;
	}

	//	JP_TRACE_IN("JPMethodOverload::isSameOverload");
	int start = 0;
	if (!isStatic() && !m_IsConstructor)
	{
		start = 1;
	}
	for (unsigned int i = start; i < m_Arguments.size() && i < o.m_Arguments.size(); i++)
	{
		if (!JPJni::equalsObject(m_Arguments[i].get(), o.m_Arguments[i].get()))
		{
			return false;
		}
	}
	return true;
	//	JP_TRACE_OUT;
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

JPMatch JPMethodOverload::matches(bool callInstance, JPPyObjectVector& arg)
{
	JP_TRACE_IN("JPMethodOverload::matches");
	JPMatch match;
	match.overload = this;

	ensureTypeCache();
	size_t len = arg.size();
	size_t tlen = m_Arguments.size();
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
	if (!m_IsVarArgs)
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
		JPClass* type = m_ArgumentsTypeCache[tlen - 1];
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
		JPClass* type = m_ArgumentsTypeCache[i];
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

void JPMethodOverload::packArgs(JPMatch& match, vector<jvalue>& v, JPPyObjectVector& arg)
{
	JP_TRACE_IN("JPMethodOverload::packArgs");
	size_t len = arg.size();
	size_t tlen = m_Arguments.size();
	JP_TRACE("skip", match.skip == 1);
	JP_TRACE("offset", match.offset == 1);
	JP_TRACE("arguments length", len);
	JP_TRACE("types length", tlen);
	if (match.isVarIndirect)
	{
		JP_TRACE("Pack varargs");
		len = tlen - 1;
		JPArrayClass* type = (JPArrayClass*) m_ArgumentsTypeCache[tlen - 1];
		v[tlen - 1 - match.skip] = type->convertToJavaVector(arg, tlen - 1, arg.size());
	}

	JP_TRACE("Pack fixed total=", len - match.offset);
	for (size_t i = match.skip; i < len; i++)
	{
		JPClass* type = m_ArgumentsTypeCache[i - match.offset];
		JP_TRACE("Convert", i - match.offset, i, type->getCanonicalName());
		v[i - match.skip] = type->convertToJava(arg[i]);
	}
	JP_TRACE_OUT;
}

JPPyObject JPMethodOverload::invoke(JPMatch& match, JPPyObjectVector& arg, bool instance)
{
	JP_TRACE_IN("JPMethodOverload::invoke");
	ensureTypeCache();
	size_t alen = m_Arguments.size();
	JPJavaFrame frame(8 + alen);
        if (m_CallerSensitive)
        {
            JP_RAISE_TYPE_ERROR("Not supported");
        }

	JPClass* retType = m_ReturnTypeCache;

	// Pack the arguments
	vector<jvalue> v(alen + 1);
	packArgs(match, v, arg);

	// Invoke the method (arg[0] = this)
	if (m_IsStatic)
	{
		jclass claz = m_Class->getJavaClass();
		return retType->invokeStatic(frame, claz, m_MethodID, &v[0]);
	}
	else
	{
		JPValue* selfObj = JPPythonEnv::getJavaValue(arg[0]);
		jobject c = selfObj->getJavaObject();
		jclass clazz = NULL;
		if (!m_IsAbstract && !instance)
			clazz = m_Class->getJavaClass();
		return retType->invoke(frame, c, clazz, m_MethodID, &v[0]);
	}
	JP_TRACE_OUT;
}

JPValue JPMethodOverload::invokeConstructor(JPMatch& match, JPPyObjectVector& arg)
{
	JP_TRACE_IN("JPMethodOverload::invokeConstructor");
	ensureTypeCache();
	size_t alen = m_Arguments.size();
	JPJavaFrame frame(8 + alen);

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

string JPMethodOverload::matchReport(JPPyObjectVector& sequence)
{
	ensureTypeCache();
	stringstream res;

	res << m_ReturnTypeCache->getCanonicalName() << " (";

	bool isFirst = true;
	for (vector<JPClass*>::iterator it = m_ArgumentsTypeCache.begin(); it != m_ArgumentsTypeCache.end(); it++)
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

bool JPMethodOverload::isMoreSpecificThan(JPMethodOverload& other) const
{
	ensureTypeCache();
	other.ensureTypeCache();
	// see http://docs.oracle.com/javase/specs/jls/se7/html/jls-15.html#jls-15.12.2.5

	// fixed-arity methods
	size_t startThis = isStatic() || m_IsConstructor ? 0 : 1;
	size_t startOther = other.isStatic() || m_IsConstructor ? 0 : 1;
	size_t numParametersThis = m_Arguments.size() - startThis;
	size_t numParametersOther = other.m_Arguments.size() - startOther;
	if (numParametersOther != numParametersThis)
	{
		return false;
	}
	for (size_t i = 0; i < numParametersThis; ++i)
	{
		JPClass* thisArgType = m_ArgumentsTypeCache[startThis + i];
		JPClass* otherArgType = other.m_ArgumentsTypeCache[startOther + i];
		if (!thisArgType->isSubTypeOf(otherArgType))
		{
			return false;
		}
	}
	return true;
}

bool JPMethodOverload::checkMoreSpecificThan(JPMethodOverload* other) const
{
	for (OverloadList::const_iterator it = m_MoreSpecificOverloads.begin();
			it != m_MoreSpecificOverloads.end();
			++it)
	{
		if (other == *it)
			return true;
	}
	return false;
}

void JPMethodOverload::ensureTypeCache() const
{
	if (m_Arguments.size() == m_ArgumentsTypeCache.size() && (m_ReturnTypeCache || m_IsConstructor))
	{
		return;
	}
	JP_TRACE_IN("JPMethodOverload::ensureTypeCache");
	// There was a bug in the previous condition, best to be safe and clear list
	m_ArgumentsTypeCache.clear();
	for (size_t i = 0; i < m_Arguments.size(); ++i)
	{
		m_ArgumentsTypeCache.push_back(JPTypeManager::findClass(m_Arguments[i].get()));
	}
	if (!m_IsConstructor)
	{
		m_ReturnTypeCache = JPTypeManager::findClass(m_ReturnType.get());
	}
	JP_TRACE_OUT;
}

bool JPMethodOverload::isBeanAccessor()
{
	ensureTypeCache();
	return !isStatic()
			&& m_ReturnTypeCache != JPTypeManager::_void
			&& getArgumentCount() == 1;
}

bool JPMethodOverload::isBeanMutator()
{
	ensureTypeCache();
	return !isStatic()
			&& m_ReturnTypeCache == JPTypeManager::_void
			&& getArgumentCount() == 2;
}
