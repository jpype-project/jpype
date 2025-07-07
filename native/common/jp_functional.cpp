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
= default;


class JPConversionFunctional : public JPConversion
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (!PyCallable_Check(match.object))
			return match.type = JPMatch::_none;

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
		if (PyFunction_Check(match.object))
		{
			PyObject* func = match.object; 
			auto* code = (PyCodeObject*) PyFunction_GetCode(func); // borrowed
			Py_ssize_t args = code->co_argcount;
			bool is_varargs = ((code->co_flags&CO_VARARGS)==CO_VARARGS);
			Py_ssize_t optional = 0;
			JPPyObject defaults = JPPyObject::accept(PyObject_GetAttrString(func, "__defaults__"));
			if (!defaults.isNull() && defaults.get() != Py_None)
				optional = PyTuple_Size(defaults.get());
			const int jargs = JPContext_global->getTypeManager()->interfaceParameterCount(cls);
			// Too few arguments
			if (!is_varargs && args < jargs)
				return match.type = JPMatch::_none;
			// Too many arguments
			if (args - optional > jargs) 
				return match.type = JPMatch::_none;
		}
		else if (PyMethod_Check(match.object))
		{
			PyObject* func = PyMethod_Function(match.object); // borrowed
			auto* code = (PyCodeObject*) PyFunction_GetCode(func); // borrowed
			Py_ssize_t args = code->co_argcount;
			bool is_varargs = ((code->co_flags&CO_VARARGS)==CO_VARARGS);
            Py_ssize_t optional = 0;
			JPPyObject defaults = JPPyObject::accept(PyObject_GetAttrString(func, "__defaults__"));
			if (!defaults.isNull() && defaults.get() != Py_None)
				optional = PyTuple_Size(defaults.get());
			const int jargs = JPContext_global->getTypeManager()->interfaceParameterCount(cls);
			// Bound self argument removes one argument
			if ((PyMethod_Self(match.object))!=nullptr) // borrowed
				args--;
			// Too few arguments
			if (!is_varargs && args < jargs)
				return match.type = JPMatch::_none;
			// Too many arguments
			if (args - optional > jargs) 
				return match.type = JPMatch::_none;
		}
		match.conversion = this;
		match.closure = cls;
		return match.type = JPMatch::_implicit;
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		PyObject *typing = PyImport_AddModule("jpype.protocol");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "Callable"));
		PyList_Append(info.implicit, proto.get());
	}

	jvalue convert(JPMatch &match) override
	{
		auto *cls = (JPFunctional*) match.closure;
		JP_TRACE_IN("JPConversionFunctional::convert");
		JPJavaFrame frame = JPJavaFrame::inner();
		auto *self = (PyJPProxy*) PyJPProxy_Type->tp_alloc(PyJPProxy_Type, 0);
		JP_PY_CHECK();
		JPClassList cl;
		cl.push_back(cls);
		self->m_Proxy = new JPProxyFunctional(self, cl);
		self->m_Target = match.object;
		self->m_Dispatch = match.object;
		self->m_Convert = true;
		Py_INCREF(self->m_Target);
		Py_INCREF(self->m_Dispatch);
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
