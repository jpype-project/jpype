#include "jp_class.h"
#include "jpype.h"
#include "jp_extension.hpp"
#include "pyjp.h"

static JPClass *getClass(PyObject *ptr) {
	unsigned long long value = PyLong_AsUnsignedLongLong(ptr);
	if (static_cast<long long>(value) <= 0) {
		PyErr_Format(PyExc_TypeError, "Java class required: %s", Py_TYPE(ptr)->tp_name);
		JP_RAISE_PYTHON();
	}
	return reinterpret_cast<JPClass *>(static_cast<uintptr_t>(value));
}

static std::vector<const JPClass *> getParams(PyObject *types) {
	std::vector<const JPClass *> res{};
	Py_ssize_t len = PySequence_Length(types);
	res.reserve(len);
	for (Py_ssize_t i = 0; i < len; i++) {
		JPPyObject type = JPPyObject::call(PySequence_GetItem(types, i));
		res.push_back(getClass(type.get()));
	}
	return res;
}

static JPMethodOverride createOverride(PyObject *def) {
	JPPyObject resType = JPPyObject::call(PySequence_GetItem(def, 0));
	JPPyObject argTypes = JPPyObject::call(PySequence_GetItem(def, 1));

	return {
		getClass(resType.get()),
		getParams(argTypes.get()),
		JPPyObject::call(PySequence_GetItem(def, 2))
	};
}

void JPExtensionType::setOverrides(JPJavaFrame& frame, PyObject *args) {
	if (m_instance == nullptr) {
		// This can't be done in the constructor because it will cause
		// the Java class to be initialized (<cinit>). This will then cause the creation
		// of another JPExtensionType whose overrides never get set and that JPExtensionType
		// ends up cached by jpype. At the point that setOverrides is called, the correct
		// JPExtensionType has already been cached and it is safe to get the field id.
		if (m_SuperClass != nullptr && m_SuperClass->isExtension()) {
			m_instance = static_cast<JPExtensionType*>(m_SuperClass)->m_instance;
		} else {
			m_instance = frame.GetFieldID(m_Class.get(), "$instance", "J");
		}
	}

	Py_ssize_t len = PySequence_Length(args);
	if (len <= 0) {
		return;
	}

	m_Overrides.reserve(len);

	for (Py_ssize_t i = 0; i < len; i++) {
		JPPyObject def = JPPyObject::call(PySequence_GetItem(args, i));
		m_Overrides.emplace_back(createOverride(def.get()));
	}
}

JPPyObject JPExtensionType::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast) {
	(void) cast;

	jobject obj = val.l;
	PyObject *instance = (PyObject *)frame.GetLongField(obj, m_instance);
	if (instance == nullptr) {
		// this is the first access when calling a python implemented constructor
		// this cannot be done in newInstance or we will have to deal with it being
		// null in the python implemented constructor and anything it calls
		PyTypeObject *type = (PyTypeObject *) m_Host.get();
		JPPyObject res = JPPyObject::call(type->tp_alloc(type, 0));
		instance = res.get();

		JPValue jv{this, obj};
		PyJPValue_assignJavaSlot(frame, instance, jv);

		frame.SetLongField(obj, m_instance, (jlong)instance);
		frame.registerRef(obj, instance);
		return res;
	}
	return JPPyObject::use(instance);
}

JPValue JPExtensionType::newInstance(JPJavaFrame& frame, JPPyObjectVector& args) {
	return JPClass::newInstance(frame, args);
}
