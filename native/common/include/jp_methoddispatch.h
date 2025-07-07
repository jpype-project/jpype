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
#ifndef _JPMETHODDISPATCH_H_
#define _JPMETHODDISPATCH_H_

#include "jp_class.h"

class JPMethodDispatch : public JPResource
{
public:

	/**
	 * Create a new method based on class and a name;
	 */
	JPMethodDispatch(JPClass *clazz,
			const string& name,
			JPMethodList& overloads,
			jint modifiers);

	~JPMethodDispatch() override;
    JPMethodDispatch(const JPMethodDispatch& method) = delete;
	JPMethodDispatch& operator=(const JPMethodDispatch& method) = delete;

public:

	JPClass* getClass()
	{
		return m_Class;
	}

	const string& getName() const;

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

	JPPyObject invoke(JPJavaFrame& frame, JPPyObjectVector& vargs, bool instance);
	JPValue invokeConstructor(JPJavaFrame& frame, JPPyObjectVector& vargs);
	bool matches(JPJavaFrame& frame, JPPyObjectVector& args, bool instance);

	string matchReport(JPPyObjectVector& sequence);

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
	bool findOverload(JPJavaFrame& frame, JPMethodMatch &bestMatch, JPPyObjectVector& vargs, bool searchInstance, bool raise);

	JPClass*      m_Class;
	string        m_Name;
	JPMethodList  m_Overloads;
	jlong         m_Modifiers;
	JPMethodCache m_LastCache{};
} ;

#endif // _JPMETHODDISPATCH_H_
