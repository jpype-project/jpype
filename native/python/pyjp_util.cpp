// --- file: python/pyjp_util.cpp ---
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
#include "jp_primitive_accessor.h"
#include "jp_gc.h"
#include "jp_proxy.h"

#ifdef WIN32
#include <Windows.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// PyJP_SetStringWithCause/PyJP_GetAttrDescriptor live in pyjp_module.cpp
// (that file's versions are the ones with the real fixes -- see
// PyJP_SetStringWithCause's null-pending-exception guard -- so this file
// only keeps the two MRO helpers below, which have no other definition).

int PyJP_IsSubClassSingle(PyTypeObject* type, PyTypeObject* obj)
{
	if (type == nullptr || obj == nullptr)
		return 0;  // GCOVR_EXCL_LINE
	PyObject* mro1 = obj->tp_mro;
	Py_ssize_t n1 = PyTuple_Size(mro1);
	Py_ssize_t n2 = PyTuple_Size(type->tp_mro);
	if (n1 < n2)
		return 0;
	return PyTuple_GetItem(mro1, n1 - n2) == (PyObject*) type;
}

int PyJP_IsInstanceSingle(PyObject* obj, PyTypeObject* type)
{
	if (type == nullptr || obj == nullptr)
		return 0; // GCOVR_EXCL_LINE
	return PyJP_IsSubClassSingle(type, Py_TYPE(obj));
}

#ifdef __cplusplus
}
#endif
