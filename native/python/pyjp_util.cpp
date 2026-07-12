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

// GCOVR_EXCL_START
// This is used exclusively during startup

void PyJP_SetStringWithCause(PyObject *exception,
		const char *str)
{
	// See _PyErr_TrySetFromCause
	PyObject *exc1, *val1, *tb1;
	PyErr_Fetch(&exc1, &val1, &tb1);
	PyErr_NormalizeException(&exc1, &val1, &tb1);
	if (tb1 != nullptr)
	{
		PyException_SetTraceback(val1, tb1);
		Py_DECREF(tb1);
	}
	Py_DECREF(exc1);
	PyErr_SetString(exception, str);
	PyObject *exc2, *val2, *tb2;
	PyErr_Fetch(&exc2, &val2, &tb2);
	PyErr_NormalizeException(&exc2, &val2, &tb2);
	PyException_SetCause(val2, val1);
	PyErr_Restore(exc2, val2, tb2);
}
// GCOVR_EXCL_STOP

PyObject* PyJP_GetAttrDescriptor(PyTypeObject *type, PyObject *attr_name)
{
	JP_PY_TRY("Py_GetAttrDescriptor");
	if (type->tp_mro == nullptr)
		return nullptr;  // GCOVR_EXCL_LINE

	// Grab the mro
	PyObject *mro = type->tp_mro;

	// mro should be a tuple
	Py_ssize_t n = PyTuple_Size(mro);

	// Search the tuple for the attribute
	for (Py_ssize_t i = 0; i < n; ++i)
	{
		auto *type2 = (PyTypeObject*) PyTuple_GetItem(mro, i);

		// Skip objects without a functioning dictionary
		if (type2->tp_dict == NULL)
			continue;

		PyObject *res = PyDict_GetItem(type2->tp_dict, attr_name);
		if (res)
		{
			Py_INCREF(res);
			return res;
		}
	}

	// Last check is id in the parent
	{
		PyObject *res = PyDict_GetItem(Py_TYPE(type)->tp_dict, attr_name);
		if (res)
		{
			Py_INCREF(res);
			return res;
		}
	}

	return nullptr;
	JP_PY_CATCH(nullptr); // GCOVR_EXCL_LINE
}

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
