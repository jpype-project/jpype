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
#ifndef _JPBOXEDCLASS_H_
#define _JPBOXEDCLASS_H_

// Boxed types have special conversion rules so that they can convert
// from python primitives.  This code specializes the class wrappers
// to make that happen.

/**
 * Class to wrap for Boxed types.
 *
 * These are linked to primitives.
 * They specialize the conversion rules to set up our table for conversion.
 */
class JPBoxedType : public JPClass
{
public:
	JPBoxedType(JPJavaFrame& frame,
			jclass clss,
			const string& name,
			JPClass* super,
			JPClassList& interfaces,
			jint modifiers,
			JPPrimitiveType* primitiveType);
	~JPBoxedType() override;

	JPMatch::Type findJavaConversion(JPMatch &match) override;
	void getConversionInfo(JPConversionInfo &info) override;

	JPPrimitiveType* getPrimitive()
	{
		return m_PrimitiveType;
	}

	jobject box(JPJavaFrame &frame, jvalue v);
	JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast) override;

protected:
	JPPrimitiveType* m_PrimitiveType;
public:
	jmethodID        m_CtorID;
	jmethodID        m_DoubleValueID;
	jmethodID        m_FloatValueID;
	jmethodID        m_IntValueID;
	jmethodID        m_LongValueID;
	jmethodID        m_BooleanValueID;
	jmethodID        m_CharValueID;
} ;

#endif // _JPBOXEDCLASS_H_