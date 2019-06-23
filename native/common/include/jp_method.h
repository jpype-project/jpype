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
#ifndef _JPMETHOD_H_
#define _JPMETHOD_H_
#include "jp_modifier.h"
class JPMethod;

class JPMatch
{
public:

	enum Type
	{
		_none = 0,
		_explicit = 1,
		_implicit = 2,
		_exact = 3
	} ;

	Type type;
	bool isVarDirect;
	bool isVarIndirect;
	JPMethod* overload;
	char offset;
	char skip;

	JPMatch()
	{
		type = JPMatch::_none;
		isVarDirect = false;
		isVarIndirect = false;
		overload = NULL;
		offset = 0;
		skip = 0;
	}
} ;

class JPMethod : public JPResource
{
	friend class JPMethodDispatch;
public:
	JPMethod();
	JPMethod(JPClass* claz,
			const string& name,
			jobject mth,
			jmethodID mid,
			JPClass *returnType,
			JPClassList parameterTypes,
			JPMethodList& moreSpecific,
			jint modifiers);

	virtual ~JPMethod();

	/** Check to see if this overload matches the argument list.
	 *
	 * @param isInstance is true if the first argument is an instance object.
	 * @param args is a list of arguments including the instance.
	 *
	 */
	JPMatch matches(bool isInstance, JPPyObjectVector& args);
	JPPyObject invoke(JPMatch& match, JPPyObjectVector&  arg, bool instance);
	JPValue  invokeConstructor(JPMatch& match, JPPyObjectVector& arg);

	bool isAbstract() const
	{
		return JPModifier::isAbstract(m_Modifiers);
	}

	bool isConstructor() const
	{
		return JPModifier::isConstructor(m_Modifiers);
	}

	bool isInstance() const
	{
		return !JPModifier::isStatic(m_Modifiers) && !JPModifier::isConstructor(m_Modifiers);
	}

	bool isFinal() const
	{
		return JPModifier::isFinal(m_Modifiers);
	}

	bool isStatic() const
	{
		return JPModifier::isStatic(m_Modifiers);
	}

	bool isVarArgs() const
	{
		return JPModifier::isVarArgs(m_Modifiers);
	}

	string toString() const;

	string matchReport(JPPyObjectVector& args);
	bool checkMoreSpecificThan(JPMethod* other) const;

	jobject getJava()
	{
		return m_Method.get();
	}

private:
	void packArgs(JPMatch& match, vector<jvalue>& v, JPPyObjectVector& arg);

	JPMethod(const JPMethod& o);
	JPMethod& operator=(const JPMethod&) ;

private:
	JPClass*                 m_Class;
	string                   m_Name;
	JPObjectRef              m_Method;
	jmethodID                m_MethodID;
	JPClass*                 m_ReturnType;
	JPClassList              m_ParameterTypes;
	JPMethodList             m_MoreSpecificOverloads;
	jint                     m_Modifiers;
} ;


#endif // _JPMETHODOVERLOAD_H_
