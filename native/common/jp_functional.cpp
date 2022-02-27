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
#include "jp_functional.h"
#include "jp_proxy.h"

JPFunctional::JPFunctional(JPJavaFrame& frame, jclass clss,
		const string& name,
		JPClass* super,
		JPClassList& interfaces,
		jint modifiers)
: JPClass(frame, clss, name, super, interfaces, modifiers)
{
	m_Method = frame.getFunctional(clss);
}

JPFunctional::~JPFunctional()
{
}

class JPConversionFunctional : public JPConversion
{
public:

	virtual JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (!PyCallable_Check(match.object))
			return match.type = JPMatch::_none;

		// Modified from https://stackoverflow.com/a/1117735
		bool has_parameter_count = false;
		// Get the __code__ attribute of the function, which contains details about the function
		// Note: __code__ is func_code in Python 2.x
		PyObject* function_code_object = PyObject_GetAttrString(match.object, "__code__");
		if(function_code_object) {
			// get the argument count
			PyObject* parameter_count_obj = PyObject_GetAttrString(function_code_object, "co_argcount");
			if(parameter_count_obj) {
				has_parameter_count = true;
				int optional_parameter_count = 0;
				int flags = 0;
				const int ACCEPT_VARGS_MASK = 0x04; // From https://docs.python.org/3/reference/datamodel.html
				PyObject* flags_obj = PyObject_GetAttrString(function_code_object, "co_flags");
				if (flags_obj) {
					flags = PyLong_AsLong(flags_obj);
					Py_DECREF(flags_obj);
				}
				PyObject* optional_parameter_tuple = PyObject_GetAttrString(match.object, "__defaults__");
				if (optional_parameter_tuple && optional_parameter_tuple != Py_None) {
					optional_parameter_count = PyTuple_Size(optional_parameter_tuple);
					Py_DECREF(optional_parameter_tuple);
				}
				const int parameter_count = PyLong_AsLong(parameter_count_obj);
				const int java_parameter_count = cls->getContext()->getTypeManager()->interfaceParameterCount(cls);
				const bool is_varargs = (flags & ACCEPT_VARGS_MASK) == ACCEPT_VARGS_MASK;

				// def my_func(x, y=None) should be both a Function and a BiFunction
				// i.e. the number of parameters accepted by the interface MUST
				// 1. Be at most the maximum number of parameters accepted by the python function (parameter_count)
				//    (Unless the function accept a variable number of arguments, then this restriction does not
				//     apply).
				// 2. Be at least the minumum number of parameters accepted by the python function
				// (parameter_count - optional_parameter_count = number of required parameters).
				// Notes:
				// - keywords vargs does not remove restriction 1
				// - keyword only arguments are not counted.
				if ((!is_varargs && parameter_count < java_parameter_count) ||
					parameter_count - optional_parameter_count > java_parameter_count) {
					match.type = JPMatch::_none;
				} else {
					match.conversion = this;
					match.closure = cls;
					match.type = JPMatch::_implicit;
				}

				Py_DECREF(parameter_count_obj);
			}
			Py_DECREF(function_code_object);
		}
		PyErr_Clear();
		if (!has_parameter_count) {
			match.conversion = this;
			match.closure = cls;
			return match.type = JPMatch::_implicit;
		} else {
			return match.type;
		}
	}

	virtual void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyObject *typing = PyImport_AddModule("jpype.protocol");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "Callable"));
		PyList_Append(info.implicit, proto.get());
	}

	virtual jvalue convert(JPMatch &match) override
	{
		JPFunctional *cls = (JPFunctional*) match.closure;
		JP_TRACE_IN("JPConversionFunctional::convert");
		JPContext *context = PyJPModule_getContext();
		JPJavaFrame frame = JPJavaFrame::inner(context);
		PyJPProxy *self = (PyJPProxy*) PyJPProxy_Type->tp_alloc(PyJPProxy_Type, 0);
		JP_PY_CHECK();
		JPClassList cl;
		cl.push_back(cls);
		self->m_Proxy = new JPProxyFunctional(context, self, cl);
		self->m_Target = match.object;
		self->m_Convert = true;
		Py_INCREF(match.object);
		jvalue v = self->m_Proxy->getProxy();
		v.l = frame.keep(v.l);
		Py_DECREF(self);
		return v;
		JP_TRACE_OUT;  // GCOVR_EXCL_LINE
	}
} functional_conversion;

JPMatch::Type JPFunctional::findJavaConversion(JPMatch &match)
{
	JP_TRACE_IN("JPJPFunctional::findJavaConversiocdn");
	JPClass::findJavaConversion(match);
	if (match.type != JPMatch::_none)
		return match.type;
	if (functional_conversion.matches(this, match))
		return match.type;
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}

void JPFunctional::getConversionInfo(JPConversionInfo &info)
{
	JP_TRACE_IN("JPJPFunctional::getConversionInfo");
	JPClass::getConversionInfo(info);
	functional_conversion.getInfo(this, info);
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}
