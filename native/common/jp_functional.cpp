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
		match.conversion = this;
		match.closure = cls;
		return match.type = JPMatch::_implicit;
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
