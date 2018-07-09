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
#ifndef _JPARRAYCLASS_H_
#define _JPARRAYCLASS_H_

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPArrayClass : public JPClass
{
public:
	JPArrayClass(jclass c);
	virtual~ JPArrayClass();

public:
	virtual JPPyObject convertToPythonObject(jvalue val) override;
	virtual JPMatch::Type canConvertToJava(PyObject* obj) override;
	virtual jvalue convertToJava(PyObject* obj) override;

	JPValue newInstance(int length);

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
	jvalue convertToJavaVector(JPPyObjectVector& refs, jsize start, jsize end);

	virtual JPClass* getComponentType()
	{
		return m_ComponentType;
	}


public:
	JPClass* m_ComponentType;
} ;


#endif // _JPARRAYCLASS_H_
