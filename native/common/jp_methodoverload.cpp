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

JPMethodOverload::JPMethodOverload()
{
	m_Method = NULL;
	m_ReturnTypeCache = NULL;
}

JPMethodOverload::JPMethodOverload(const JPMethodOverload& o) :
	m_Class(o.m_Class),
	m_MethodID(o.m_MethodID),
	m_ReturnType(o.m_ReturnType),
	m_Arguments(o.m_Arguments),
	m_IsStatic(o.m_IsStatic),
	m_IsFinal(o.m_IsFinal),
	m_IsVarArgs(o.m_IsVarArgs),
	m_IsConstructor(o.m_IsConstructor)
{
	m_Method = JPEnv::getJava()->NewGlobalRef(o.m_Method);
	m_ReturnTypeCache = NULL;
}

JPMethodOverload::JPMethodOverload(JPClass* claz, jobject mth)
{
	m_Class = claz;
	m_Method = JPEnv::getJava()->NewGlobalRef(mth);
	m_ReturnTypeCache = NULL;

	// static
	m_IsStatic = JPJni::isMemberStatic(m_Method);
	m_IsFinal = JPJni::isMemberFinal(m_Method);
	m_IsVarArgs = JPJni::isVarArgsMethod(m_Method);

	// Method ID
	m_MethodID = JPEnv::getJava()->FromReflectedMethod(m_Method);
	
	m_IsConstructor = JPJni::isConstructor(m_Method);

	// return type
	if (! m_IsConstructor)
	{
		m_ReturnType = JPJni::getReturnType(m_Method);
	}

	// arguments
	m_Arguments = JPJni::getParameterTypes(mth, m_IsConstructor);
	// Add the implicit "this" argument
	if (! m_IsStatic && ! m_IsConstructor)
	{
		m_Arguments.insert(m_Arguments.begin(), 1, claz->getName());
	}
}

JPMethodOverload::~JPMethodOverload()
{
	JPEnv::getJava()->DeleteGlobalRef(m_Method);
}

string JPMethodOverload::getSignature()
{
	stringstream res;
	
	res << "(";
	
	for (vector<JPTypeName>::iterator it = m_Arguments.begin(); it != m_Arguments.end(); it++)
	{
		res << it->getNativeName();
	}
	
	res << ")" ;
	
	return res.str();
}

string JPMethodOverload::getArgumentString()
{
	stringstream res;
	
	res << "(";
	
	bool first = true;
	for (vector<JPTypeName>::iterator it = m_Arguments.begin(); it != m_Arguments.end(); it++)
	{
		if (! first)
		{
			res << ", ";
		}
		else
		{
			first = false;
		}
		res << it->getSimpleName();
	}
	
	res << ")";
	
	return res.str();
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

	TRACE_IN("JPMethodOverload::isSameOverload");
	TRACE2("My sig", getSignature());
	TRACE2("It's sig", o.getSignature());
	int start = 0;
	if (! isStatic())
	{
		start = 1;
	}
	for (unsigned int i = start; i < m_Arguments.size() && i < o.m_Arguments.size(); i++)
	{
		JPTypeName& mine = m_Arguments[i];
		JPTypeName& his = o.m_Arguments[i];
		const string& mineSimple = mine.getSimpleName();
		const string& hisSimple = his.getSimpleName();

		if (mineSimple != hisSimple)
		{
			return false;
		}
	}
	return true;
	TRACE_OUT;
}

EMatchType matchVars(vector<HostRef*>& arg, size_t start, JPType* vartype)
{
	TRACE_IN("JPMethodOverload::matchVars");
	JPArrayClass* arraytype = (JPArrayClass*) vartype;
	JPType* type = arraytype->getComponentType();
	size_t len = arg.size();

	EMatchType lastMatch = _exact;
	for (size_t i = start; i < len; i++)
	{
		HostRef* obj = arg[i];
		EMatchType match = type->canConvertToJava(obj);

		if (match < _implicit)
		{
			return _none;
		}
		if (match < lastMatch)
		{
			lastMatch = match;
		}
	}
	
	return lastMatch;
	TRACE_OUT;
}

EMatchType JPMethodOverload::matches(bool ignoreFirst, vector<HostRef*>& arg)
{
	TRACE_IN("JPMethodOverload::matches");
	ensureTypeCache();
	size_t len = arg.size();
	size_t tlen = m_Arguments.size();

	EMatchType lastMatch = _exact;
	if (!m_IsVarArgs)
	{
		if (len != tlen)
		{
			return _none;
		}
	}
	else
	{
		JPType* type = m_ArgumentsTypeCache[tlen-1];
		if (len < tlen-1)
		{
			return _none;
		}

		if (len == tlen)
		{
		  // Hard, could be direct array or an array.
			
			// Try direct
			HostRef* obj = arg[len-1];
			len = tlen-1;
		  lastMatch = type->canConvertToJava(obj);
		  if (lastMatch < _implicit)
			{
				// Try indirect
				lastMatch = matchVars(arg, tlen-1, type);
			}
		}
		else if (len > tlen)
		{
			// Must match the array type
			len = tlen-1;
			lastMatch = matchVars(arg, tlen-1, type);
		}
		if (lastMatch < _implicit)
		{
			return _none;
		}
	}
	
	for (unsigned int i = 0; i < len; i++)
	{
		if (i == 0 && ignoreFirst)
		{
			continue;
		}

		HostRef* obj = arg[i];
		JPType* type = m_ArgumentsTypeCache[i];
		
		EMatchType match = type->canConvertToJava(obj);
		if (match < _implicit)
		{
			return _none;
		}
		if (match < lastMatch)
		{
			lastMatch = match;
		}
	}
	
	return lastMatch;
	TRACE_OUT;
}

void JPMethodOverload::packArgs(JPMallocCleaner<jvalue>& v, vector<HostRef*>& arg, size_t skip)
{	
	TRACE_IN("JPMethodOverload::packArgs");
	size_t len = arg.size();
	size_t tlen = m_Arguments.size();
	TRACE2("arguments length",len);
	TRACE2("types length",tlen);
	bool packArray = false;
	if (m_IsVarArgs)
	{ 
		if (len == tlen)
		{
			HostRef* obj = arg[len-1];
			JPType* type = m_ArgumentsTypeCache[tlen-1];
		  if (type->canConvertToJava(obj) < _implicit)
			{
				len = tlen-1;
				packArray = true;
			}
		}
		else
		{
			len = tlen-1;
			packArray = true;
		}
	}

	TRACE2("Pack fixed total=",len-skip);
	for (size_t i = skip; i < len; i++)
	{
		TRACE2("Convert ",i);
		HostRef* obj = arg[i];
		JPType* type = m_ArgumentsTypeCache[i];

		v[i-skip] = type->convertToJava(obj);		
	}

	if (packArray)
	{
		TRACE1("Pack array");
		len = arg.size();
		JPArrayClass* type = (JPArrayClass*) m_ArgumentsTypeCache[tlen-1];
		v[tlen-1-skip] = type->convertToJavaVector(arg, tlen-1, len);
	}
	TRACE_OUT;
}

HostRef* JPMethodOverload::invokeStatic(vector<HostRef*>& arg)
{
	TRACE_IN("JPMethodOverload::invokeStatic");
	ensureTypeCache();
	size_t alen = m_Arguments.size();
	JPLocalFrame frame(8+alen);
	JPMallocCleaner<jvalue> v(alen);
	packArgs(v, arg, 0);
	jclass claz = m_Class->getClass();
	JPType* retType = m_ReturnTypeCache;

	return retType->invokeStatic(claz, m_MethodID, v.borrow());
	TRACE_OUT;
}

HostRef* JPMethodOverload::invokeInstance(vector<HostRef*>& arg)
{
	TRACE_IN("JPMethodOverload::invokeInstance");
	ensureTypeCache();
	HostRef* res;
	{
	  size_t alen = m_Arguments.size();
		JPLocalFrame frame(8+alen);
	
		// Arg 0 is "this"
		HostRef* self = arg[0];
		JPObject* selfObj = JPEnv::getHost()->asObject(self);
	
	
		JPMallocCleaner<jvalue> v(alen-1);
		packArgs(v, arg, 1);
		JPType* retType = m_ReturnTypeCache;
	
		jobject c = selfObj->getObject();
		jclass clazz = m_Class->getClass();
	
		res = retType->invoke(c, clazz, m_MethodID, v.borrow());
		TRACE1("Call finished");
	}
	TRACE1("Call successfull");
	
	return res;

	TRACE_OUT;
}

JPObject* JPMethodOverload::invokeConstructor(jclass claz, vector<HostRef*>& arg)
{
	TRACE_IN("JPMethodOverload::invokeConstructor");
	ensureTypeCache();

	size_t alen = m_Arguments.size();
	JPLocalFrame frame(8+alen);
	
	JPMallocCleaner<jvalue> v(alen);
	packArgs(v, arg, 0);
	
	jvalue val;
	val.l = JPEnv::getJava()->NewObjectA(claz, m_MethodID, v.borrow());
	TRACE1("Object created");
	
	JPTypeName name = JPJni::getName(claz);
	return new JPObject(name, val.l);

	TRACE_OUT;
}

string JPMethodOverload::matchReport(vector<HostRef*>& args)
{
	stringstream res;

	res << m_ReturnType.getNativeName() << " (";

	bool isFirst = true;
	for (vector<JPTypeName>::iterator it = m_Arguments.begin(); it != m_Arguments.end(); it++)
	{
		if (isFirst && ! isStatic())
		{
			isFirst = false;
			continue;
		}
		isFirst = false;
		res << it->getNativeName();
	}
	
	res << ") ==> ";

	EMatchType match = matches(! isStatic(), args);
	switch(match)
	{
	case _none :
		res << "NONE";
		break;
	case _explicit :
		res << "EXPLICIT";
		break;
	case _implicit :
		res << "IMPLICIT";
		break;
	case _exact :
		res << "EXACT";
		break;
	default :
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
	size_t startThis = isStatic() || m_IsConstructor  ? 0 : 1;
	size_t startOther = other.isStatic() || m_IsConstructor ? 0 : 1;
	size_t numParametersThis = m_Arguments.size() - startThis;
	size_t numParametersOther = other.m_Arguments.size() - startOther;
	if(numParametersOther != numParametersThis) {
		return false;
	}
	for (size_t i = 0; i < numParametersThis; ++i) {
		const JPType* thisArgType = m_ArgumentsTypeCache[startThis + i];
		const JPType* otherArgType = other.m_ArgumentsTypeCache[startOther + i];
		if (!thisArgType->isSubTypeOf(*otherArgType)) {
			return false;
		}
	}
	return true;
}

void JPMethodOverload::ensureTypeCache() const 
{
//	TRACE_IN("JPMethodOverload::ensureTypeCache");
	if (m_Arguments.size() == m_ArgumentsTypeCache.size() && (m_ReturnTypeCache || m_IsConstructor)) 
	{ 
		return; 
	}
	// There was a bug in the previous condition, best to be safe and clear list
	m_ArgumentsTypeCache.clear(); 
	for (size_t i = 0; i < m_Arguments.size(); ++i) 
	{
		m_ArgumentsTypeCache.push_back(JPTypeManager::getType(m_Arguments[i]));
	}
	if (!m_IsConstructor) 
	{
		m_ReturnTypeCache = JPTypeManager::getType(m_ReturnType);
	}
//	TRACE_OUT;
}

