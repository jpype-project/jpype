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

class JPMethod: public JPResource
{
public:
	typedef list<JPMethodOverload*> OverloadList;

	/**
	 * Create a new method based on class and a name;
	 */
	JPMethod(JPClass *clazz, const string& name, bool isConstructor);
	virtual ~JPMethod();

private:
	JPMethod(const JPMethod& method);
	JPMethod& operator=(const JPMethod& method);

public:
	const string& getName() const;
	string getClassName() const;

	void addOverload(JPClass* clazz, jobject mth);

	bool hasStatic()
	{
		return m_hasStatic;
	}

	bool isBeanMutator();
	bool isBeanAccessor();

	JPPyObject invoke(JPPyObjectVector& vargs, bool instance);
	JPValue invokeConstructor(JPPyObjectVector& vargs);

	string matchReport(JPPyObjectVector& sequence);
	string dump();

	const OverloadList& getMethodOverloads()
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
	void ensureOverloadCache();
	void dumpOverloads();

	JPClass*      m_Class;
	string        m_Name;
	OverloadList  m_Overloads;
	bool          m_IsConstructor;
	bool          m_hasStatic;
	bool          m_Cached;
} ;

#endif // _JPMETHOD_H_

