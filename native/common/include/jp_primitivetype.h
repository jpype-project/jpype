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
#ifndef _JPPRIMITIVETYPE_H_
#define _JPPRIMITIVETYPE_H_
#include "jp_boxedtype.h"

class JPPrimitiveType : public JPClass
{
protected:
	explicit JPPrimitiveType(const string& name);
	~JPPrimitiveType() override;

public:
	bool isPrimitive() const override;

	virtual JPClass* getBoxedClass(JPJavaFrame& frame) const = 0;

	virtual char getTypeCode() = 0;
	virtual jlong getAsLong(jvalue v) = 0;
	virtual jdouble getAsDouble(jvalue v) = 0;

	void setClass(JPJavaFrame& frame, jclass o)
	{
		m_Class = JPClassRef(frame, o);
	}

	virtual void getView(JPArrayView& view) = 0;
	virtual void releaseView(JPArrayView& view) = 0;
	virtual const char* getBufferFormat() = 0;
	virtual Py_ssize_t getItemSize() = 0;
	virtual void copyElements(JPJavaFrame &frame,
			jarray a, jsize start, jsize len,
			void* memory, int offset) = 0;

	virtual PyObject *newMultiArray(JPJavaFrame &frame,
			JPPyBuffer& view, int subs, int base, jobject dims) = 0;

	// Helper for Long types
	PyObject *convertLong(PyTypeObject* wrapper, PyLongObject* tmp);
} ;

#endif
