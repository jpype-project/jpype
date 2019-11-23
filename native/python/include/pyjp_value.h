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
#ifndef _PYJP_VALUE_H_
#define _PYJP_VALUE_H_

#include "pyjp_context.h"


extern PyObject *PyJPClassMeta_Type;
extern PyObject *PyJPValueBase_Type;
extern PyObject *PyJPValue_Type;
extern PyObject *PyJPValueInt_Type;
extern PyObject *PyJPValueExc_Type;

inline JPContext* PyJPValue_getContext(PyJPValue* value)
{
	JPContext *context = value->m_Context->m_Context;
	if (context == NULL)
	{
		JP_RAISE_RUNTIME_ERROR("Context is null");
	}
	ASSERT_JVM_RUNNING(context);
	return;
}
#define PyJPValue_GET_CONTEXT(X) PyJPValue_getContext((PyJPValue*)X)

struct PyJPValue
{
	PyObject_HEAD
	JPValue m_Value;
	PyJPContext *m_Context;

	static JPPyObject create(PyTypeObject* wrapper, JPContext* context, JPClass* cls, jvalue value);
	static void initType(PyObject *module);
	static PyJPValue* getValue(PyObject *self);
} ;

#endif // _PYJP_VALUE_H_2
