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
}

JPMethodOverload::JPMethodOverload(const JPMethodOverload& o) :
	m_Class(o.m_Class),
	m_MethodID(o.m_MethodID),
	m_ReturnType(o.m_ReturnType),
	m_Arguments(o.m_Arguments),
	m_IsStatic(o.m_IsStatic),
	m_IsFinal(o.m_IsFinal),
	m_IsConstructor(o.m_IsConstructor)
{
	m_Method = JPEnv::getJava()->NewGlobalRef(o.m_Method);
}

JPMethodOverload::JPMethodOverload(JPClass* claz, jobject mth)
{
	m_Class = claz;
	m_Method = JPEnv::getJava()->NewGlobalRef(mth);

	// static
	m_IsStatic = JPJni::isMemberStatic(mth);
	m_IsFinal = JPJni::isMemberStatic(m_Method);
	
	// Method ID
	m_MethodID = JPEnv::getJava()->FromReflectedMethod(mth);
	
	m_IsConstructor = JPJni::isConstructor(m_Method);

	// return type
	if (! m_IsConstructor)
	{
		m_ReturnType = JPJni::getReturnType(mth);
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
		JPTypeName mine = m_Arguments[i];
		JPTypeName his = o.m_Arguments[i];
		string mineSimple = mine.getSimpleName();
		string hisSimple = his.getSimpleName();

		if (mineSimple != hisSimple)
		{
			return false;
		}
	}
	return true;
	TRACE_OUT;
}

EMatchType JPMethodOverload::matches(bool ignoreFirst, vector<HostRef*>& arg)
{
	TRACE_IN("JPMethodOverload::matches");

	size_t len = arg.size();
	
	if (len != m_Arguments.size())
	{
		return _none;
	}
	
	EMatchType lastMatch = _exact;
	for (unsigned int i = 0; i < len; i++)
	{
		if (i == 0 && ignoreFirst)
		{
			continue;
		}

		HostRef* obj = arg[i];
		JPType* type = JPTypeManager::getType(m_Arguments[i]);
		
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

HostRef* JPMethodOverload::invokeStatic(vector<HostRef*>& arg)
{
	TRACE_IN("JPMethodOverload::invokeStatic");
	JPCleaner cleaner;
	
	
	size_t len = arg.size();
	
	JPMallocCleaner<jvalue> v(len);
	JPMallocCleaner<JPType*> types(len);
	
	for (unsigned int i = 0; i < len; i++)
	{
		HostRef* obj = arg[i];
		types[i] = JPTypeManager::getType(m_Arguments[i]);

		v[i] = types[i]->convertToJava(obj);		
		if (types[i]->isObjectType())
		{
			cleaner.addLocal(v[i].l);
		}
	}
	
	jclass claz = m_Class->getClass();
	cleaner.addLocal(claz);

	JPType* retType = JPTypeManager::getType(m_ReturnType);

	return retType->invokeStatic(claz, m_MethodID, v.borrow());
	TRACE_OUT;
}

HostRef* JPMethodOverload::invokeInstance(vector<HostRef*>& args)
{
	TRACE_IN("JPMethodOverload::invokeInstance");
	HostRef* res;
	{
		JPCleaner cleaner;
	
		// Arg 0 is "this"
		HostRef* self = args[0];
		JPObject* selfObj = JPEnv::getHost()->asObject(self);
	
		size_t len = args.size();
	
		JPMallocCleaner<jvalue> v(len-1);
		
		for (unsigned int i = 1; i < len; i++)
		{
			HostRef* obj = args[i];
	
			JPType* type = JPTypeManager::getType(m_Arguments[i]);
			v[i-1] = type->convertToJava(obj);		
			if (type->isObjectType())
			{
				cleaner.addLocal(v[i-1].l);
			}
		}
		
		JPType* retType = JPTypeManager::getType(m_ReturnType);
	
		jobject c = selfObj->getObject();
		cleaner.addLocal(c);
	
		jclass clazz = m_Class->getClass();
		cleaner.addLocal(clazz);
	
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

	size_t len = arg.size();
	JPCleaner cleaner;
	
	JPMallocCleaner<jvalue> v(len);

	for (unsigned int i = 0; i < len; i++)
	{
		HostRef* obj = arg[i];
		// TODO the following can easily be optimized ... or at least cached
		JPType* t = JPTypeManager::getType(m_Arguments[i]);

		v[i] = t->convertToJava(obj);		
		if (t->isObjectType())
		{
			cleaner.addLocal(v[i].l);
		}
	}
	
	jvalue val;
	val.l = JPEnv::getJava()->NewObjectA(claz, m_MethodID, v.borrow());
	cleaner.addLocal(val.l);
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
