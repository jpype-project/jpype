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
#ifndef _JPMETHOD_H_
#define _JPMETHOD_H_
#include "jp_modifier.h"
class JPMethod;

class JPMethod : public JPResource
{
	friend class JPMethodDispatch;
public:
	JPMethod() = default;
	JPMethod(JPJavaFrame& frame,
			JPClass* claz,
			const string& name,
			jobject mth,
			jmethodID mid,
			JPMethodList& moreSpecific,
			jint modifiers);

	~JPMethod() override;

	void setParameters(
			JPClass *returnType,
			JPClassList&& parameterTypes);

	/** Check to see if this overload matches the argument list.
	 *
	 * @param frame is the scope to hold Java local variables.
	 * @param match holds the details of the match that is found.
	 * @param isInstance is true if the first argument is an instance object.
	 * @param args is a list of arguments including the instance.
	 *
	 * @return the quality of the match.
	 *
	 */
	JPMatch::Type matches(JPJavaFrame &frame, JPMethodMatch& match, bool isInstance, JPPyObjectVector& args);
	JPPyObject invoke(JPJavaFrame &frame, JPMethodMatch& match, JPPyObjectVector& arg, bool instance);
	JPPyObject invokeCallerSensitive(JPMethodMatch& match, JPPyObjectVector& arg, bool instance);
	JPValue invokeConstructor(JPJavaFrame &frame, JPMethodMatch& match, JPPyObjectVector& arg);

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

	bool isCallerSensitive() const
	{
		return JPModifier::isCallerSensitive(m_Modifiers);
	}

	string toString() const;

	string matchReport(JPPyObjectVector& args);
	bool checkMoreSpecificThan(JPMethod* other) const;

	jobject getJava()
	{
		return m_Method.get();
	}

    JPMethod& operator=(const JPMethod&) = delete;

private:
	void packArgs(JPJavaFrame &frame, JPMethodMatch &match, vector<jvalue> &v, JPPyObjectVector &arg);
	void ensureTypeCache();

	JPMethod(const JPMethod& o);

private:
	JPClass*                 m_Class{};
	string                   m_Name;
	JPObjectRef              m_Method;
	jmethodID                m_MethodID{};
	JPClass*                 m_ReturnType{};
	JPClassList              m_ParameterTypes;
	JPMethodList             m_MoreSpecificOverloads;
	jint                     m_Modifiers{};
} ;

#endif // _JPMETHOD_H_