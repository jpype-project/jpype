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
#ifndef _JPBUFFER_H_
#define _JPBUFFER_H_

#include "jp_javaframe.h"

class JPBufferType;

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPBuffer
{
public:
	explicit JPBuffer(const JPValue& array);
	virtual~ JPBuffer();

	JPBufferType* getClass()
	{
		return m_Class;
	}

	jobject getJava()
	{
		return m_Object.get();
	}

	bool isReadOnly() const;

	Py_buffer& getView();

	bool isValid() const;

private:
	JPBufferType* m_Class;
	JPObjectRef m_Object;
	void *m_Address;
	Py_ssize_t m_Capacity;
	Py_buffer m_Buffer{};
	char m_Format[3]{};
} ;

#endif // _JPBUFFER_H_