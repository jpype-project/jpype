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
#ifndef _JPPRIMITIVETYPE_H_
#define _JPPRIMITIVETYPE_H_

class JPPrimitiveType : public JPClass
{
protected:
	JPPrimitiveType(JPContext* context, jclass clss,
		const string& name,
		JPBoxedType* boxedClass,
		jint modifiers);
	virtual ~JPPrimitiveType();

protected:
	JPClass* m_BoxedClass;

public:

	JPClass* getBoxedClass() const
	{
		return m_BoxedClass;
	}

	void setBoxedClass(JPBoxedType* boxedClass);

} ;

#endif 
