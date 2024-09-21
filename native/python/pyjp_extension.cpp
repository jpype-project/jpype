#include "jp_class.h"
#include "pyjp.h"
#include "jp_extension.hpp"
#include <pyconfig.h>

static JPClass *getClass(PyObject *ptr) {
	unsigned long long value = PyLong_AsUnsignedLongLong(ptr);
	if (static_cast<long long>(value) <= 0) {
		JP_RAISE(PyExc_TypeError, "Java class required");
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

extern "C" PyObject  *PyJPExtension_putOverrides(PyObject *type, PyObject *args) {
	JP_PY_TRY("PyJPExtension_putOverrides");
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);

	Py_ssize_t len = PySequence_Length(args);
	if (len <= 0) {
		return nullptr;
	}

	JPClass* cls = PyJPClass_getJPClass(type);
	if (cls == nullptr) {
		PyErr_SetString(PyExc_TypeError, "Java class required");
		return nullptr;
	}

	JPMethodOverrideList overrides{};
	overrides.reserve(len);

	for (Py_ssize_t i = 0; i < len; i++) {
		JPPyObject def = JPPyObject::call(PySequence_GetItem(args, i));
		overrides.emplace_back(createOverride(def.get()));
	}

	cls->addOverrides(std::move(overrides));
	return nullptr;
	JP_PY_CATCH(nullptr);
}
