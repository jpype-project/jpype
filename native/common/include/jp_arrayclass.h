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
#ifndef _JPARRAYCLASS_H_
#define _JPARRAYCLASS_H_

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPArrayClass : public JPClass
{
public:
	JPArrayClass(JPJavaFrame& frame,
			jclass cls,
			const string& name,
			JPClass* superClass,
			JPClass* componentType,
			jint modifiers);
	~ JPArrayClass() override;

	JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast) override;
	JPMatch::Type findJavaConversion(JPMatch &match) override;
	void getConversionInfo(JPConversionInfo &info) override;

	JPValue newArray(JPJavaFrame& frame, int length);

	/**
	 * Create a new java array containing a set of items take from
	 * a range.
	 *
	 * This is used to support variable arguments.
	 *
	 * @param refs contains a vector of python objects.
	 * @param start is the start of the range inclusive.
	 * @param end is the end of the range exclusive.
	 * @return a jvalue containing a java vector.
	 */
	jvalue convertToJavaVector(JPJavaFrame& frame, JPPyObjectVector& refs, jsize start, jsize end);

	virtual JPClass* getComponentType()
	{
		return m_ComponentType;
	}

	bool isArray() const override
	{
		return true;
	}

private:
	JPClass* m_ComponentType;
} ;

#endif // _JPARRAYCLASS_H_
