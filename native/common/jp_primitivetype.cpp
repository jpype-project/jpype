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

extern "C" Py_ssize_t PyJPValue_getJavaSlotOffset(PyObject* self);

// equivalent of long_subtype_new as it isn't exposed

PyObject *JPPrimitiveType::convertLong(PyTypeObject* wrapper, PyLongObject* tmp)
{
	if (wrapper == nullptr)
		JP_RAISE(PyExc_SystemError, "bad wrapper");

#if PY_VERSION_HEX<0x030c0000
	// Determine number of digits to copy
	Py_ssize_t n = Py_SIZE(tmp);
	if (n < 0)
		n = -n;

	auto *newobj = (PyLongObject *) wrapper->tp_alloc(wrapper, n);
	if (newobj == nullptr)
		return nullptr;

	// Size is in units of digits
	((PyVarObject*) newobj)->ob_size = Py_SIZE(tmp);
	for (Py_ssize_t i = 0; i < n; i++)
	{
		newobj->ob_digit[i] = tmp->ob_digit[i];
	}

#else
	// 3.12 completely does away with ob_size field and repurposes it
	
	// Determine the number of digits to copy
	int n = (tmp->long_value.lv_tag >> 3);

	PyLongObject *newobj = (PyLongObject *) wrapper->tp_alloc(wrapper, n);
	if (newobj == NULL)
		return NULL;

	newobj->long_value.lv_tag = tmp->long_value.lv_tag;
	memcpy(&newobj->long_value.ob_digit, &tmp->long_value.ob_digit, n*sizeof(digit));
#endif
	return (PyObject*) newobj;
}

