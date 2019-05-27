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
#ifndef _JPMETHODDISPATCH_H_
#define _JPMETHODDISPATCH_H_

class JPMethodDispatch: public JPResource
{
public:

	/**
	 * Create a new method based on class and a name;
	 */
	JPMethodDispatch(JPClass *clazz, const string& name, bool isConstructor);
	virtual ~JPMethodDispatch();

private:
	JPMethodDispatch(const JPMethodDispatch& method);
	JPMethodDispatch& operator=(const JPMethodDispatch& method);

public:
	const string& getName() const;
	string getClassName() const;

	bool hasStatic() const
	{
		return JPModifier::isStatic(m_Modifiers);
	}

	bool isBeanMutator() const
	{
		return JPModifier::isBeanMutator(m_Modifiers);
	}

	bool isBeanAccessor() const
	{
		return JPModifier::isBeanAccessor(m_Modifiers);
	}

	JPPyObject invoke(JPPyObjectVector& vargs, bool instance);
	JPValue invokeConstructor(JPPyObjectVector& vargs);

	string matchReport(JPPyObjectVector& sequence);
	string dump();

	const JPMethodList& getMethodOverloads()
	{
		return m_Overloads;
	}

private:
	/** Search for a matching overload.
	 * 
	 * @param searchInstance is true if the first argument is to be skipped
	 * when matching with a non-static.
	 */
	JPMatch findOverload(JPPyObjectVector& vargs, bool searchInstance);
	void dumpOverloads();

	JPClass*      m_Class;
	string        m_Name;
	JPMethodList  m_Overloads;
	jlong         m_Modifiers;
} ;

#endif // _JPMETHODDISPATCH_H_
