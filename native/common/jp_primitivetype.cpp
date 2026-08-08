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
#include "jpype.h"
#include "pyjp.h"

JPPrimitiveType::JPPrimitiveType(const string& name)
: JPClass(name, 0x411)
{
}

JPPrimitiveType::~JPPrimitiveType()
= default;

bool JPPrimitiveType::isPrimitive() const
{
	return true;
}

PyObject *JPPrimitiveType::convertLong(PyTypeObject* wrapper, PyLongObject* tmp)
{
	if (wrapper == nullptr)
		JP_RAISE(PyExc_SystemError, "bad wrapper");

	// PyLong_AsLongLong can't fail/overflow here -- tmp always represents a
	// genuine Java primitive (byte/short/int/long), which always fits
	// within 64 bits.
	long long value = PyLong_AsLongLong((PyObject*) tmp);
	return PyJPNumber_longFromLongLong(wrapper, value);
}

