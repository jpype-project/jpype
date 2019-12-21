#include <jpype.h>
#include <pyjp.h>

const char *__javavalue__ = "__javavalue__";
const char *__javaproxy__ = "__javaproxy__";
const char *__javaclass__ = "__javaclass__";
const char *__javaclasshints__ = "__javaclasshints__";

/** Construct a Python wrapper for a Java object. */
JPPyObject JPPythonEnv::newJavaObject(const JPValue& value)
{
	JP_TRACE_IN("JPPythonEnv::newJavaObject");
	JPClass *javaClass = value.getClass();
	JPPyObject javaClassWrapper = newJavaClass(javaClass);
	return PyJPValue_create((PyTypeObject*) javaClassWrapper.get(),
			value.getClass()->getContext(),
			value);
	JP_TRACE_OUT;
}

/**
 * Construct a new Java class wrapper.
 *
 * @param javaClass is the JPClass to wrap.
 * @return a new python class wrapper.
 * @throws if unable to create wrapper.
 */
JPPyObject JPPythonEnv::newJavaClass(JPClass *javaClass)
{
	JP_TRACE_IN("JPPythonEnv::newJavaClass");
	JPContext *context = javaClass->getContext();
	JPPyObject pycls(PyJPClass_create(
			(PyTypeObject*) PyJPModuleState_global->PyJPClass_Type, context, javaClass));
	PyObject *out = PyJPModule_getClass(NULL, pycls.get());
	if (out == NULL)
		JP_RAISE_PYTHON("Failed");
	return JPPyObject(JPPyRef::_claim, out);
	JP_TRACE_OUT;
}

/** Retrieve the Java value from a Python object. */
JPValue *JPPythonEnv::getJavaValue(PyObject *obj)
{
	PyJPValue *value = PyJPValue_asValue(obj);
	if (value == NULL)
		return NULL;
	return &(value->m_Value);
}

/** Retrieve the Java value from a Python object. */
JPClass *JPPythonEnv::getJavaClass(PyObject *obj)
{
	// Check for Java class instance
	if (PyJPClass_Check(obj))
		return ((PyJPClass*) obj)->m_Class;

	// Check for class wrapper
	JPPyObject self(JPPyRef::_accept, PyObject_GetAttrString(obj, __javaclass__));
	if (self.isNull())
		return NULL;
	if (PyJPClass_Check(self.get()))
		return ((PyJPClass*) self.get())->m_Class;
	return NULL;
}

JPProxy *JPPythonEnv::getJavaProxy(PyObject *obj)
{
	PyJPModuleState *state = PyJPModuleState_global;
	if (Py_TYPE(obj) == (PyTypeObject*) state->PyJPProxy_Type)
		return ((PyJPProxy*) obj)->m_Proxy;
	JPPyObject self(JPPyRef::_accept, PyObject_GetAttrString(obj, __javaproxy__));
	if (!self.isNull() && Py_TYPE(self.get()) == (PyTypeObject*) state->PyJPProxy_Type)
		return (((PyJPProxy*) self.get())->m_Proxy);
	return NULL;
}

JPPyObject JPPythonEnv::getJavaProxyCallable(PyObject *obj, const string& name)
{
	JP_TRACE_IN("JPythonEnv::getJavaProxyCallable");
	PyObject *target = obj;
	JP_TRACE("Target", target);
	return JPPyObject(JPPyRef::_accept, PyObject_GetAttrString(target, name.c_str()));
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

PyObject* PyType_Lookup(PyTypeObject *type, PyObject *attr_name)
{
	if (type->tp_mro == NULL)
		return NULL;

	PyObject *mro = type->tp_mro;
	Py_ssize_t n = PyTuple_Size(mro);
	for (Py_ssize_t i = 0; i < n; ++i)
	{
		type = (PyTypeObject*) PyTuple_GetItem(mro, i);
		PyObject *res = PyDict_GetItem(type->tp_dict, attr_name);
		if (res)
		{
			Py_INCREF(res);
			return res;
		}
	}
	return NULL;
}

