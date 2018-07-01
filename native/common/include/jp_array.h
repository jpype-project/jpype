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

#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPArray
{
public:
	JPArray(JPClass* cls, jarray inst);
	virtual~ JPArray();

public:

	JPArrayClass* getClass()
	{
		return m_Class;
	}

	int        getLength();
	JPPyObject getRange(size_t start, size_t stop);
	void       setRange(size_t start, size_t stop, PyObject* val);
	JPPyObject getItem(size_t ndx);
	void       setItem(size_t ndx, PyObject*);

	jobject getObject()
	{
		return m_Object.get();
	}

public: // Wrapper
	virtual JPClass* getType();
	virtual jvalue  getValue();

private:
	JPArrayClass* m_Class;
	JPArrayRef    m_Object;
	size_t        m_Length;
} ;


#endif // _JPARRAY_H_
