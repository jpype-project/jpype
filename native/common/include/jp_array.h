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

#include "jp_javaframe.h"


#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

class JPArray;

class JPArrayView
{
public:
	JPArrayView(JPArray* array);
	JPArrayView(JPArray* array, int d0, int d1);
	~JPArrayView();
	void reference();
	bool unreference();

public:
	JPArray *array;
	void *memory;
	Py_buffer buffer;
	int refcount;
	Py_ssize_t shape[2];
	Py_ssize_t strides[2];
	jboolean isCopy;
	jboolean owned;
} ;

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPArray
{
	friend class JPArrayView;
public:
	JPArray(const JPValue& array);
	JPArray(JPArray* cls, jsize start, jsize stop, jsize step);
	virtual~ JPArray();

public:

	JPArrayClass* getClass()
	{
		return m_Class;
	}

	jsize     getLength();
	void       setRange(jsize start, jsize length, jsize step, PyObject* val);
	JPPyObject getItem(jsize ndx);
	void       setItem(jsize ndx, PyObject*);
	jarray     clone(JPJavaFrame& frame, PyObject* obj);

	bool       isSlice() const
	{
		return m_Slice;
	}

	jarray     getJava()
	{
		return m_Object.get();
	}

	jobject getObject()
	{
		return m_Object.get();
	}

	int checkIsPrimitive(int &dims);
	int checkRectangular(int &dimsize0, int &dimsize1);

private:
	JPArrayClass* m_Class;
	JPArrayRef    m_Object;
	jsize         m_Start;
	jsize         m_Step;
	jsize         m_Length;
	bool          m_Slice;
} ;


#endif // _JPARRAY_H_
