#include <jpype.h>
#include <pyjp.h>

const char *__javavalue__ = "__javavalue__";
const char *__javaproxy__ = "__javaproxy__";
const char *__javaclass__ = "__javaclass__";

/** Construct a Python wrapper for a Java object. */
JPPyObject JPPythonEnv::newJavaObject(const JPValue& value)
{
	JP_TRACE_IN("JPPythonEnv::newJavaObject");
	JPClass *javaClass = value.getClass();
	JPPyObject javaClassWrapper = newJavaClass(javaClass);

	if (javaClassWrapper.isNull())
	{
		JP_TRACE("Convert during initialization");
		return JPPyObject();
	}

	return PyJPValue_create((PyTypeObject*) javaClassWrapper.get(),
			value.getClass()->getContext(),
			value);
	JP_TRACE_OUT;
}

JPValue *JPPythonEnv::getJavaValue(PyObject *obj)
{
	PyJPValue *value = PyJPValue_asValue(obj);
	if (value == NULL)
		return NULL;
	return &(value->m_Value);
}

JPClass *JPPythonEnv::getJavaClass(PyObject *obj)
{
	PyJPModuleState *state = PyJPModuleState_global;
	JPPyObject vobj(JPPyRef::_use, obj);
	if (PyObject_IsInstance(obj, (PyObject*) state->PyJPClass_Type))
		return ((PyJPClass*) obj)->m_Class;
	if (!JPPyObject::hasAttrString(obj, __javaclass__))
		return NULL;
	JPPyObject self(JPPyObject::getAttrString(obj, __javaclass__));
	if (Py_TYPE(self.get()) == (PyTypeObject*) state->PyJPClass_Type)
	{
		return ((PyJPClass*) self.get())->m_Class;
	}
	return NULL;
}

JPProxy *JPPythonEnv::getJavaProxy(PyObject *obj)
{
	PyJPModuleState *state = PyJPModuleState_global;
	if (Py_TYPE(obj) == (PyTypeObject*) state->PyJPProxy_Type)
		return ((PyJPProxy*) obj)->m_Proxy;
	if (!JPPyObject::hasAttrString(obj, __javaproxy__))
		return 0;
	JPPyObject self(JPPyObject::getAttrString(obj, __javaproxy__));
	if (Py_TYPE(self.get()) == (PyTypeObject*) state->PyJPProxy_Type)
	{
		return (((PyJPProxy*) self.get())->m_Proxy);
	}
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

//============================================================
// Factory for Base classes

/**
 * Create a JPValue wrapper with the appropriate type.
 *
 * This method dodges the __new__ method, but does involve
 * __init__.  It is called when returning a Java object back to
 * Python.
 *
 * @param type is the type of the resulting wrapper.
 * @param context is the java virtual machine.
 * @param value is the java value to wrap.
 * @return
 */
JPPyObject PyJPValue_create(PyTypeObject *type, JPContext *context, const JPValue& value)
{
	PyJPModuleState *state = PyJPModuleState_global;
	// dispatch by type so we will create the right wrapper type
	JPPyObject out;
	JPClass* cls = value.getClass();

	if (type == (PyTypeObject *) state->PyJPValue_Type)
		out = PyJPValue_createInstance(type, context, value);
	else if (cls->isThrowable())
		out = PyJPValue_createBase(type, context, value);
	else if (cls == context->_java_lang_Class)
	{
		JPClass *cls2 = context->getTypeManager()->findClass((jclass) value.getValue().l);
		out = PyJPClass_create(type, context, cls2);
	} else if (dynamic_cast<JPBoxedType*> (cls) != 0)
		out = PyJPValue_createBoxed(type, context, value);
	else if (dynamic_cast<JPArrayClass*> (cls) != 0)
		out = PyJPArray_create(type, context, value);
	else
		out = PyJPValue_createInstance(type, context, value);

	return out;
}

JPPyObject PyJPValue_createInstance(PyTypeObject *wrapper, JPContext *context, const JPValue& value)
{
	JP_TRACE_IN("PyJPValue_createInstance");

	if (value.getClass() == NULL && value.getValue().l != NULL)
		JP_RAISE_RUNTIME_ERROR("Value inconsistency");

	PyJPValue *self = (PyJPValue*) ((PyTypeObject*) wrapper)->tp_alloc(wrapper, 0);
	JP_PY_CHECK();

	// If it is not a primitive, we need to reference it
	if (context != NULL && !value.getClass()->isPrimitive())
	{
		JPJavaFrame frame(context);
		jvalue v;
		v.l = frame.NewGlobalRef(value.getValue().l);
		self->m_Value = JPValue(value.getClass(), v);
	} else
	{
		// New value instance
		self->m_Value = value;
	}
	return JPPyObject(JPPyRef::_claim, (PyObject*) self);
	JP_TRACE_OUT;
}

JPPyObject PyJPValue_createBase(PyTypeObject *wrapper, JPContext *context, const JPValue& value)
{
	PyJPModuleState *state = PyJPModuleState_global;
	JP_TRACE_IN("PyJPValue_createbase");
	JPPyObject self(JPPyRef::_claim, ((PyTypeObject*) wrapper)->tp_alloc(wrapper, 0));
	self.setAttrString(__javavalue__,
			PyJPValue_createInstance((PyTypeObject*) state->PyJPValue_Type, context, value).get());
	return self;
	JP_TRACE_OUT;
}

JPPyObject PyJPValue_createBoxed(PyTypeObject *wrapper, JPContext *context, const JPValue& value)
{
	PyJPModuleState *state = PyJPModuleState_global;
	// Find the primitive type
	JPPrimitiveType *primitive = ((JPBoxedType*) value.getClass())->getPrimitive();

	// Convert object to primitive type
	JPValue jcontents = primitive->getValueFromObject(value);
	JPPyObject pycontents;

	// Convert to python based on the type
	if (primitive == context->_boolean)
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong(jcontents.getValue().z));
	else if (primitive == context->_byte)
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong(jcontents.getValue().b));
	else if (primitive == context->_short)
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong(jcontents.getValue().s));
	else if (primitive == context->_int)
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong(jcontents.getValue().i));
	else if (primitive == context->_long)
		pycontents = JPPyObject(JPPyRef::_claim, PyLong_FromLong((long) jcontents.getValue().j));
	else if (primitive == context->_float)
		pycontents = JPPyObject(JPPyRef::_claim, PyFloat_FromDouble(jcontents.getValue().f));
	else if (primitive == context->_double)
		pycontents = JPPyObject(JPPyRef::_claim, PyFloat_FromDouble(jcontents.getValue().d));
	else if (primitive == context->_char)
		return PyJPValue_createInstance(wrapper, context, value);

	JPPyTuple tuple = JPPyTuple::newTuple(1);
	tuple.setItem(0, pycontents.get());
	JPPyObject self(JPPyRef::_call, PyObject_Call((PyObject*) wrapper, tuple.get(), NULL));
	self.setAttrString(__javavalue__, PyJPValue_createInstance((PyTypeObject*) state->PyJPValue_Type, context, value).get());
	return self;
}

JPPyObject PyJPArray_create(PyTypeObject *wrapper, JPContext *context, const JPValue& value)
{
	JP_TRACE_IN("PyJPArray_alloc");
	JPPyObject self = PyJPValue_createInstance(wrapper, context, value);
	((PyJPArray*) self.get())->m_Array = new JPArray(value.getClass(), (jarray) value.getValue().l);
	return self;
	JP_TRACE_OUT;
}

JPPyObject PyJPClass_create(PyTypeObject *wrapper, JPContext *context, JPClass *cls)
{
	JP_TRACE_IN("PyJPClass_create");
	// Special case for primitives
	if (context == NULL)
	{
		jvalue v;
		v.l = 0;
		JPPyObject self = PyJPValue_createInstance(wrapper, context, JPValue(NULL, v));
		((PyJPClass*) self.get())->m_Class = cls;
		return self;
	}

	JPJavaFrame frame(context);
	jvalue value;
	value.l = (jobject) cls->getJavaClass();
	JPPyObject self = PyJPValue_createInstance(wrapper, context,
			JPValue(context->_java_lang_Class, value));
	((PyJPClass*) self.get())->m_Class = cls;
	return self;
	JP_TRACE_OUT;
}

JPPyObject PyJPField_create(JPField *field)
{
	JP_TRACE_IN("PyJPField_create");
	PyJPModuleState *state = PyJPModuleState_global;
	JPContext *context = field->getContext();
	jvalue v;
	v.l = field->getJavaObject();
	if (state->PyJPField_Type == NULL)
		JP_RAISE_RUNTIME_ERROR("PyJPField type is not defined.");
	if (context->_java_lang_reflect_Field == NULL)
		JP_RAISE_RUNTIME_ERROR("java.lang.reflect.Field not loaded.");
	JPPyObject self = PyJPValue_createInstance((PyTypeObject*) state->PyJPField_Type, field->getContext(),
			JPValue(context->_java_lang_reflect_Field, v));
	((PyJPField*) self.get())->m_Field = field;
	return self;
	JP_TRACE_OUT;
}

JPPyObject PyJPMethod_create(JPMethodDispatch *m, PyObject *instance)
{
	JP_TRACE_IN("PyJPMethod_create");
	PyJPModuleState *state = PyJPModuleState_global;
	PyTypeObject *type = (PyTypeObject*) state->PyJPMethod_Type;
	PyJPMethod *self = (PyJPMethod*) type->tp_alloc(type, 0);
	JP_PY_CHECK();
	self->m_Method = m;
	self->m_Instance = instance;
	if (instance != NULL)
	{
		JP_TRACE_PY("method alloc (inc)", instance);
		Py_INCREF(instance);
	}
	self->m_Doc = NULL;
	self->m_Annotations = NULL;
	self->m_CodeRep = NULL;
	JP_TRACE("self", self);
	return JPPyObject(JPPyRef::_claim, (PyObject*) self);
	JP_TRACE_OUT;
}
