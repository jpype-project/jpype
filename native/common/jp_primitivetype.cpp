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
{
}

bool JPPrimitiveType::isPrimitive() const
{
	return true;
}


// equivalent of long_subtype_new as it isn't exposed

PyObject *JPPrimitiveType::convertLong(PyTypeObject* wrapper, PyLongObject* tmp)
{
	if (wrapper == NULL)
		JP_RAISE(PyExc_SystemError, "bad wrapper");

	// Determine number of bytes to copy
	Py_ssize_t n = Py_SIZE(tmp);
	if (n < 0)
		n = -n;

	PyLongObject *newobj = (PyLongObject *) wrapper->tp_alloc(wrapper, n);
	if (newobj == NULL)
		return NULL;

	// Size is in units of digits

	((PyVarObject*) newobj)->ob_size = Py_SIZE(tmp);
#if PY_VERSION_HEX<0x030c0000
	digit *p1 = (digit*)&(newobj->ob_digit);
	digit *p2 = (digit*)&(tmp->ob_digit);
#else
	newobj->long_value.lv_tag = tmp->long_value.lv_tag;
	digit *p1 = (digit*)&(newobj->long_value.ob_digit);
	digit *p2 = (digit*)&(tmp->long_value.ob_digit);
#endif
	for (Py_ssize_t i = 0; i < n; i++)
	{
		*p1 = *p2;
		p1++;
		p2++;
	}
	return (PyObject*) newobj;
}

