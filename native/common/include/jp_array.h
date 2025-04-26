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
#ifndef _JPARRAY_H_
#define _JPARRAY_H_

#include "jp_javaframe.h"

class JPArray;

class JPArrayView
{
public:
	explicit JPArrayView(JPArray* array);
	JPArrayView(JPArray* array, jobject collection);
	~JPArrayView();
	void reference();
	bool unreference();
public:
	JPArray *m_Array;
	void *m_Memory{};
	Py_buffer m_Buffer{};
	int m_RefCount;
	Py_ssize_t m_Shape[5]{};
	Py_ssize_t m_Strides[5]{};
	jboolean m_IsCopy{};
	jboolean m_Owned{};
} ;

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPArray
{
	friend class JPArrayView;
public:
	explicit JPArray(const JPValue& array);
	JPArray(JPArray* cls, jsize start, jsize stop, jsize step);
	virtual~ JPArray();

	JPArrayClass* getClass()
	{
		return m_Class;
	}

	jsize     getLength() const;
	void       setRange(jsize start, jsize length, jsize step, PyObject* val);
	JPPyObject getItem(jsize ndx);
	void       setItem(jsize ndx, PyObject*);

	/**
	 *  Create a shallow copy of an array.
	 *
	 * This is used to extract a slice before calling or casting operations.
	 *
	 * @param frame
	 * @param obj
	 * @return
	 */
	jarray     clone(JPJavaFrame& frame, PyObject* obj);

	bool       isSlice() const
	{
		return m_Slice;
	}

	jarray     getJava()
	{
		return m_Object.get();
	}

private:
	JPArrayClass* m_Class;
	JPArrayRef    m_Object;
	jsize         m_Start;
	jsize         m_Step;
	jsize         m_Length;
	bool          m_Slice;
} ;

#endif // _JPARRAY_H_
