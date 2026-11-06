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
#ifndef _JPFIELD_H_
#define _JPFIELD_H_
#include <atomic>

/**
 * Field object
 */
class JPField
{
public:
	/**
	 * Create a new field based on class and java.lang.Field object
	 */
	JPField(JPJavaFrame& frame,
			JPClass *cls,
			const string& name,
			jobject field,
			JPClass *fieldType,
			jint modifiers);

	/**
	 * destructor
	 */
	virtual ~JPField();

    // disallow copying.
    JPField(const JPField&) = delete;
    JPField& operator=(const JPField&) = delete;

	jobject getJavaObject()
	{
		return this->m_Field.get();
	}

	const string& getName() const
	{
		return m_Name;
	}

	JPPyObject getStaticField();
	void     setStaticField(PyObject *pyobj);

	JPPyObject getField(jobject inst);
	void     setField(jobject inst, PyObject *pyobj);

	bool isFinal() const
	{
		return JPModifier::isFinal(m_Modifiers);
	}

	bool isStatic() const
	{
		return JPModifier::isStatic(m_Modifiers);
	}

	JPClass *getClass() const
	{
		return m_Class;
	}

private:
	// The field ID is resolved on first use rather than at wrapper creation:
	// JNI FromReflectedField initializes the declaring class, which would
	// run its static initializer for JClass(name, initialize=False).  The
	// lookup is idempotent, so concurrent first uses may both resolve it.
	jfieldID getFieldID(JPJavaFrame& frame);

	string           m_Name;
	JPClass*         m_Class;
	JPObjectRef      m_Field;
	std::atomic<jfieldID> m_FieldID{nullptr};
	JPClass*         m_Type;
	jint             m_Modifiers;
} ;

#endif // _JPFIELD_H_
