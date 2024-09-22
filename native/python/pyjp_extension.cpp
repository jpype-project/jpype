#include "jp_class.h"
#include "jpype.h"
#include "jp_extension.hpp"
#include <abstract.h>
#include <pyconfig.h>

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

void JPExtensionType::setOverrides(PyObject *args) {
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
