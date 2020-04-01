#include "jpype.h"
#include "pyjp.h"
#include "jp_functional.h"

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

	virtual JPMatch::Type matches(JPMatch &match, JPClass *cls) override
	{
		if (!PyCallable_Check(match.object))
			return match.type = JPMatch::_none;
		match.conversion = this;
		match.closure = cls;
		return match.type = JPMatch::_implicit;
	}

	virtual jvalue convert(JPMatch &match) override
	{
		JPFunctional *cls = (JPFunctional*) match.closure;
		JP_TRACE_IN("JPConversionFunctional::convert");
		JPContext *context = PyJPModule_getContext();
		JPJavaFrame frame(context);
		PyJPProxy *self = (PyJPProxy*) PyJPProxy_Type->tp_alloc(PyJPProxy_Type, 0);
		JP_PY_CHECK();
		JPClassList cl;
		cl.push_back(cls);
		self->m_Proxy = new JPProxyFunctional(context, self, cl);
		self->m_Target = match.object;
		self->m_Convert = true;
		Py_INCREF(match.object);
		jvalue v = self->m_Proxy->getProxy();
		Py_DECREF(self);
		return v;
		JP_TRACE_OUT;
	}
} functional_conversion;

JPMatch::Type JPFunctional::findJavaConversion(JPMatch &match)
{
	JP_TRACE_IN("JPJPFunctional::getJavaConversion");
	JPClass::findJavaConversion(match);
	if (match.type != JPMatch::_none)
		return match.type;
	if (functional_conversion.matches(match, this))
		return match.type;
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}