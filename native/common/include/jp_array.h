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
#ifndef _JPARRAY_H_
#define _JPARRAY_H_

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPArray
{
public:
	JPArray(JPClass* cls, jarray inst);
	virtual~ JPArray();

	JPArrayClass* getClass()
	{
		return m_Class;
	}

	jsize     getLength();
	JPPyObject getRange(jsize start, jsize stop);
	void       setRange(jsize start, jsize stop, PyObject* val);
	JPPyObject getItem(jsize ndx);
	void       setItem(jsize ndx, PyObject*);

	jobject getObject()
	{
		return m_Object.get();
	}

	virtual JPClass* getType();
	virtual jvalue  getValue();

private:
	JPArrayClass* m_Class;
	JPArrayRef    m_Object;
	jsize        m_Length;
} ;


#endif // _JPARRAY_H_
