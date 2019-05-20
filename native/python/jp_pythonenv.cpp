#include <pyjp.h>
#include <jpype.h>

/** Python seems to delete static variables after the Python resources
 * have already been claimed, so we need to make sure these objects 
 * never get deleted.
 *
 * FIXME figure out how to connect to module unloading.
 */
class JPResources
{
public:
	JPPyObject s_GetClassMethod;
};

namespace
{
	JPResources* s_Resources = NULL;
	const char* __javavalue__ = "__javavalue__";
	const char* __javaproxy__ = "__javaproxy__";
	const char* __javaclass__ = "__javaclass__";
}

void JPPythonEnv::init()
{
	// Nothing frees this currently.  We lack a way to shutdown or reload 
	// this module.
	s_Resources = new JPResources();
}

void JPPythonEnv::setResource(const string& name, PyObject* resource)
{
	JP_TRACE_IN("JPPythonEnv::setResource");
	JP_TRACE(name);
	JP_TRACE_PY("hold", resource);
	if (name == "GetClassMethod")
		s_Resources->s_GetClassMethod = JPPyObject(JPPyRef::_use, resource);
	else
	{
		stringstream ss;
		ss << "Unknown jpype resource " << name;
		JP_RAISE_RUNTIME_ERROR(ss.str());
	}
	JP_TRACE_OUT;
}

/** Construct a Python wrapper for a Java object. */
JPPyObject JPPythonEnv::newJavaObject(const JPValue& value)
{
	JP_TRACE_IN("JPPythonEnv::newJavaObject");
	JPClass* javaClass = value.getClass();
	JPPyObject javaClassWrapper = newJavaClass(javaClass);

	if (javaClassWrapper.isNull())
	{
		JP_TRACE("Convert during initialization");
		return JPPyObject();
	}

	JP_TRACE("Pack args");
	JPPyTuple args(JPPyTuple::newTuple(1));
	args.setItem(0, PyJPValue::alloc(value).get());

	JP_TRACE("Call python");
	return javaClassWrapper.call(args.get(), NULL);
	JP_TRACE_OUT;
}

JPPyObject JPPythonEnv::newJavaClass(JPClass* javaClass)
{
	JP_TRACE_IN("JPPythonEnv::newJavaClass");
	ASSERT_NOT_NULL(javaClass);

	JP_TRACE(javaClass->toString());
	JPPyTuple args(JPPyTuple::newTuple(1));
	args.setItem(0, PyJPClass::alloc(javaClass).get());

	// calls jpype._jclass._getClassFor(_jpype.PyJPClass)
	if (s_Resources->s_GetClassMethod.isNull())
	{
		JP_TRACE("Resource not set.");
		return JPPyObject();
	}
	return s_Resources->s_GetClassMethod.call(args.get(), NULL);
	JP_TRACE_OUT;
}

JPValue* JPPythonEnv::getJavaValue(PyObject* obj)
{
	JPPyObject vobj(JPPyRef::_use, obj);
	if (obj->ob_type == &PyJPValue::Type)
		return &((PyJPValue*) obj)->m_Value;
	if (!JPPyObject::hasAttrString(obj, __javavalue__))
		return 0;
	JPPyObject self(JPPyObject::getAttrString(obj, __javavalue__));
	if (self.get()->ob_type == &PyJPValue::Type)
	{
		return &(((PyJPValue*) self.get())->m_Value);
	}
	return NULL;
}

JPClass* JPPythonEnv::getJavaClass(PyObject* obj)
{
	JPPyObject vobj(JPPyRef::_use, obj);
	if (obj->ob_type == &PyJPClass::Type)
		return ((PyJPClass*) obj)->m_Class;
	if (!JPPyObject::hasAttrString(obj, __javaclass__))
		return NULL;
	JPPyObject self(JPPyObject::getAttrString(obj, __javaclass__));
	if (self.get()->ob_type == &PyJPClass::Type)
	{
		return ((PyJPClass*) self.get())->m_Class;
	}
	return NULL;
}

JPProxy* JPPythonEnv::getJavaProxy(PyObject* obj)
{
	if (obj->ob_type == &PyJPProxy::Type)
		return ((PyJPProxy*) obj)->m_Proxy;
	if (!JPPyObject::hasAttrString(obj, __javaproxy__))
		return 0;
	JPPyObject self(JPPyObject::getAttrString(obj, __javaproxy__));
	if (self.get()->ob_type == &PyJPProxy::Type)
	{
		return (((PyJPProxy*) self.get())->m_Proxy);
	}
	return NULL;
}

JPPyObject JPPythonEnv::getJavaProxyCallable(PyObject* obj, const string& name)
{
	JP_TRACE_IN("JPythonEnv::getJavaProxyCallable");
	PyJPProxy* proxy = (PyJPProxy*) obj;
	PyObject* callable = proxy->m_Callable;
	PyObject* target = proxy->m_Target;
	JP_TRACE("Proxy", proxy);
	JP_TRACE("Callable", callable);
	JP_TRACE("Target", target);

	// Pack arguments
	JPPyTuple args(JPPyTuple::newTuple(2));
	args.setItem(0, target);
	args.setItem(1, JPPyString::fromStringUTF8(name).get());

	// Lookup function must be "def lookup(target, name)"
	return JPPyObject(JPPyRef::_call, PyObject_Call(callable, args.get(), NULL));
	JP_TRACE_OUT;
}

void JPPythonEnv::rethrow(const JPStackInfo& info)
{
	JP_TRACE_IN("JPythonEnv::rethrow");
	JP_TRACE(info.getFile(), info.getLine());
	try
	{
		throw;
	} catch (JPypeException& ex)
	{
		ex.from(info); // this likely wont be necessary, but for now we will add the entry point.
		ex.toPython();
		return;
	}
	JP_TRACE_OUT;
}

