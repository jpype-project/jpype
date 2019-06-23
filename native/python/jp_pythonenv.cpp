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
	JPPyObject s_GetMethodDoc;
	JPPyObject s_GetMethodAnnotations;
	JPPyObject s_GetMethodCode;
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
	JP_TRACE_IN_C("JPPythonEnv::setResource");
	JP_TRACE(name);
	JP_TRACE_PY("hold", resource);
	if (name == "GetClassMethod")
		s_Resources->s_GetClassMethod = JPPyObject(JPPyRef::_use, resource);
	else if (name == "GetMethodDoc")
		s_Resources->s_GetMethodDoc = JPPyObject(JPPyRef::_use, resource);
	else if (name == "GetMethodAnnotations")
		s_Resources->s_GetMethodAnnotations = JPPyObject(JPPyRef::_use, resource);
	else if (name == "GetMethodCode")
		s_Resources->s_GetMethodCode = JPPyObject(JPPyRef::_use, resource);
	else
	{
		stringstream ss;
		ss << "Unknown jpype resource " << name;
		JP_RAISE_RUNTIME_ERROR(ss.str());
	}
	JP_TRACE_OUT_C;
}

/** Construct a Python wrapper for a Java object. */
JPPyObject JPPythonEnv::newJavaObject(const JPValue& value)
{
	JP_TRACE_IN_C("JPPythonEnv::newJavaObject");
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
	JP_TRACE_OUT_C;
}

JPPyObject JPPythonEnv::newJavaClass(JPClass* javaClass)
{
	JP_TRACE_IN_C("JPPythonEnv::newJavaClass");
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
	JP_TRACE_OUT_C;
}

JPValue* JPPythonEnv::getJavaValue(PyObject* obj)
{
	JP_TRACE_IN_C("JPPythonEnv::getJavaValue");
	//	JPPyObject vobj(JPPyRef::_use, obj);
	if (Py_TYPE(obj) == &PyJPValue::Type)
		return &((PyJPValue*) obj)->m_Value;
	if (!JPPyObject::hasAttrString(obj, __javavalue__))
		return 0;

	JPPyObject self(JPPyObject::getAttrString(obj, __javavalue__));
	if (Py_TYPE(self.get()) == &PyJPValue::Type)
	{
		return &(((PyJPValue*) self.get())->m_Value);
	}
	return NULL;
	JP_TRACE_OUT_C;
}

JPClass* JPPythonEnv::getJavaClass(PyObject* obj)
{
	JPPyObject vobj(JPPyRef::_use, obj);
	if (Py_TYPE(obj) == &PyJPClass::Type)
		return ((PyJPClass*) obj)->m_Class;
	if (!JPPyObject::hasAttrString(obj, __javaclass__))
		return NULL;
	JPPyObject self(JPPyObject::getAttrString(obj, __javaclass__));
	if (Py_TYPE(self.get()) == &PyJPClass::Type)
	{
		return ((PyJPClass*) self.get())->m_Class;
	}
	return NULL;
}

JPProxy* JPPythonEnv::getJavaProxy(PyObject* obj)
{
	if (Py_TYPE(obj) == &PyJPProxy::Type)
		return ((PyJPProxy*) obj)->m_Proxy;
	if (!JPPyObject::hasAttrString(obj, __javaproxy__))
		return 0;
	JPPyObject self(JPPyObject::getAttrString(obj, __javaproxy__));
	if (Py_TYPE(self.get()) == &PyJPProxy::Type)
	{
		return (((PyJPProxy*) self.get())->m_Proxy);
	}
	return NULL;
}

JPPyObject JPPythonEnv::getJavaProxyCallable(PyObject* obj, const string& name)
{
	JP_TRACE_IN_C("JPythonEnv::getJavaProxyCallable");
	PyObject* target = obj;
	JP_TRACE("Target", target);
	return JPPyObject(JPPyRef::_accept, PyObject_GetAttrString(target, name.c_str()));
	JP_TRACE_OUT_C;
}

void JPPythonEnv::rethrow(const JPStackInfo& info)
{
	JP_TRACE_IN_C("JPythonEnv::rethrow");
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
	JP_TRACE_OUT_C;
}

JPPyObject JPPythonEnv::getMethodDoc(PyJPMethod* javaMethod)
{
	JPContext *context = javaMethod->m_Context->m_Context;
	JP_TRACE_IN("JPPythonEnv::getMethodDoc");
	if (s_Resources->s_GetMethodDoc.isNull())
	{
		JP_TRACE("Resource not set.");
		return JPPyObject();
	}

	ASSERT_NOT_NULL(javaMethod);

	// Convert the overloads
	JP_TRACE("Convert overloads");
	const JPMethodList& overloads = javaMethod->m_Method->getMethodOverloads();
	JPPyTuple ov(JPPyTuple::newTuple(overloads.size()));
	int i = 0;
	JPClass* methodClass = context->getTypeManager()->findClassByName("java.lang.reflect.Method");
	for (JPMethodList::const_iterator iter = overloads.begin(); iter != overloads.end(); ++iter)
	{
		JP_TRACE("Set overload", i);
		jvalue v;
		v.l = (*iter)->getJava();
		JPPyObject obj(JPPythonEnv::newJavaObject(JPValue(methodClass, v)));
		ov.setItem(i++, obj.get());
	}

	// Pack the arguments
	{
		JP_TRACE("Pack arguments");
		JPPyTuple args(JPPyTuple::newTuple(3));
		args.setItem(0, (PyObject*) javaMethod);
		jvalue v;
		v.l = (jobject) javaMethod->m_Method->getClass()->getJavaClass();
		JPPyObject obj(JPPythonEnv::newJavaObject(JPValue(context->_java_lang_Class, v)));
		args.setItem(1, obj.get());
		args.setItem(2, ov.get());
		JP_TRACE("Call Python");
		return s_Resources->s_GetMethodDoc.call(args.get(), NULL);
	}

	JP_TRACE_OUT;
}

JPPyObject JPPythonEnv::getMethodAnnotations(PyJPMethod* javaMethod)
{
	JP_TRACE_IN("JPPythonEnv::getMethodAnnotations");
	if (s_Resources->s_GetMethodDoc.isNull())
	{
		JP_TRACE("Resource not set.");
		return JPPyObject();
	}

	ASSERT_NOT_NULL(javaMethod);

	// Convert the overloads
	JP_TRACE("Convert overloads");
	const JPMethod::OverloadList& overloads = javaMethod->m_Method->getMethodOverloads();
	JPPyTuple ov(JPPyTuple::newTuple(overloads.size()));
	int i = 0;
	JPClass* methodClass = JPTypeManager::findClass("java.lang.reflect.Method");
	for (JPMethod::OverloadList::const_iterator iter = overloads.begin(); iter != overloads.end(); ++iter)
	{
		JP_TRACE("Set overload", i);
		jvalue v;
		v.l = (*iter)->getJava();
		JPPyObject obj(JPPythonEnv::newJavaObject(JPValue(methodClass, v)));
		ov.setItem(i++, obj.get());
	}

	// Pack the arguments
	{
		JP_TRACE("Pack arguments");
		JPPyTuple args(JPPyTuple::newTuple(3));
		args.setItem(0, (PyObject*) javaMethod);
		jvalue v;
		v.l = (jobject) javaMethod->m_Method->getClass()->getJavaClass();
		JPPyObject obj(JPPythonEnv::newJavaObject(JPValue(JPTypeManager::_java_lang_Class, v)));
		args.setItem(1, obj.get());
		args.setItem(2, ov.get());
		JP_TRACE("Call Python");
		return s_Resources->s_GetMethodAnnotations.call(args.get(), NULL);
	}

	JP_TRACE_OUT;
}


JPPyObject JPPythonEnv::getMethodCode(PyJPMethod* javaMethod)
{
	JP_TRACE_IN("JPPythonEnv::getMethodCode");
	if (s_Resources->s_GetMethodCode.isNull())
	{
		JP_TRACE("Resource not set.");
		return JPPyObject();
	}

	ASSERT_NOT_NULL(javaMethod);

	// Pack the arguments
	{
		JP_TRACE("Pack arguments");
		JPPyTuple args(JPPyTuple::newTuple(1));
		args.setItem(0, (PyObject*) javaMethod);
		JP_TRACE("Call Python");
		return s_Resources->s_GetMethodCode.call(args.get(), NULL);
	}

	JP_TRACE_OUT;
}
