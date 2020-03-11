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
	virtual ~JPBoxedType();

	virtual JPMatch::Type getJavaConversion(JPJavaFrame *frame, JPMatch &match, PyObject *pyobj) override;

	JPPrimitiveType* getPrimitive()
	{
		return m_PrimitiveType;
	}

protected:
	JPPrimitiveType* m_PrimitiveType;
} ;

#endif // _JPBOXEDCLASS_H_