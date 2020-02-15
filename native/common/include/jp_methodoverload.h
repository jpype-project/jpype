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
#ifndef _JPMETHODOVERLOAD_H_
#define _JPMETHODOVERLOAD_H_

class JPMethodOverload;

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
	JPMethodOverload* overload;
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

class JPMethodOverload
{
	friend class JPMethod;
public:
	JPMethodOverload();
	JPMethodOverload(JPClass* claz, jobject mth);

	virtual ~JPMethodOverload();

	/** Check to see if this overload matches the argument list.
	 *
	 * @param isInstance is true if the first argument is an instance object.
	 * @param args is a list of arguments including the instance.
	 *
	 */
	JPMatch matches(bool isInstance, JPPyObjectVector& args) ;
	JPPyObject invoke(JPJavaFrame& frame, JPMatch& match, JPPyObjectVector& arg, bool instance);
	JPValue  invokeConstructor(JPJavaFrame& frame, JPMatch& match, JPPyObjectVector& arg);

	jobject getJava() const
	{
		return m_Method.get();
	}

	bool isStatic() const
	{
		return m_IsStatic;
	}

	bool isConstructor() const
	{
		return m_IsConstructor;
	}

	bool isInstance() const
	{
		return !m_IsStatic && !m_IsConstructor;
	}

	bool isFinal() const
	{
		return m_IsFinal;
	}

	bool isVarArgs() const
	{
		return m_IsVarArgs;
	}

	unsigned char getArgumentCount() const
	{
		return (unsigned char) m_Arguments.size();
	}

	string toString() const;

	bool isSameOverload(JPMethodOverload& o);

	/** Determine if a method is more specific than another. */
	bool isMoreSpecificThan(JPMethodOverload& other) const;

	/** Consult the cache to determine if a method is more specific
	 * than another.
	 */
	bool checkMoreSpecificThan(JPMethodOverload* other) const;

	/** Used to determine if a bean get property should be added to the class.
	 *
	 * FIXME This does not check for begins with "get"
	 */
	bool isBeanAccessor();

	/** Used to determine if a bean set property should be added to the class.
	 *
	 * FIXME This does not check for begins with "set" or "is"
	 */
	bool isBeanMutator();

	string matchReport(JPPyObjectVector& args);

private:
	void packArgs(JPMatch& match, vector<jvalue>& v, JPPyObjectVector& arg);
	void ensureTypeCache() const;

	JPMethodOverload(const JPMethodOverload& o);
	JPMethodOverload& operator=(const JPMethodOverload&) ;

private:
	typedef list<JPMethodOverload*> OverloadList;

	JPClass*                 m_Class;
	JPObjectRef              m_Method;
	jmethodID                m_MethodID;
	JPClassRef               m_ReturnType;
	vector<JPClassRef>       m_Arguments;
	mutable JPClass*         m_ReturnTypeCache;
	mutable vector<JPClass*> m_ArgumentsTypeCache;
	bool                     m_IsStatic;
	bool                     m_IsFinal;
	bool                     m_IsVarArgs;
	bool                     m_IsConstructor;
	OverloadList             m_MoreSpecificOverloads;
	bool                     m_Ordered;
	bool                     m_IsAbstract;
	bool                     m_CallerSensitive;
} ;

#endif // _JPMETHODOVERLOAD_H_
