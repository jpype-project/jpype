#pragma once

#include "jp_field.h"
#include "jp_javaframe.h"
#include "jp_method.h"
#include "pyjp.h"

#include <type_traits>

/*
#include <concepts>

template <class T>
concept JavaMember = requires(T t) {
	std::same_as<JPField> || std::same_as<JPMethod>;
	{ t.isPublic() } -> std::same_as<bool>;
	{ t.isProtected() } -> std::same_as<bool>;
	{ t.isPrivate() } -> std::same_as<bool>;
	{ t.isStatic() } -> std::same_as<bool>;
	{ t.getClass() } -> std::same_as<JPClass *>;
};

template <JavaMember T>
bool canAccess(JPJavaFrame &jframe, const T &member) { ... }
*/

template <class T>
using IsJavaMember = std::enable_if_t<std::disjunction_v<std::is_same<T, JPField>, std::is_same<T, JPMethod>>, bool>;

namespace {

template <class T, IsJavaMember<T> = true>
bool canAccess(JPJavaFrame &jframe, const T &member) {
	if (member.isPublic()) {
		return true;
	}
#if PY_MINOR_VERSION<11
	// FIXME
	return true;
#endif

	PyThreadState *state = PyThreadState_Get();
	JPPyObject frame = JPPyObject::accept((PyObject*)PyThreadState_GetFrame(state));
	JPPyObject locals = JPPyObject::accept(PyFrame_GetLocals((PyFrameObject*)frame.get()));

	PyObject *obj = PyDict_GetItemString(locals.get(), "self");
	if (obj != nullptr) {
		obj = (PyObject *) Py_TYPE(obj);
	} else {
		obj = PyDict_GetItemString(locals.get(), "cls");
	}

	if (obj == nullptr) {
		return false;
	}

	JPClass *cls = PyJPClass_getJPClass(obj);
	if (cls == nullptr) {
		return false;
	}

	if (member.isPrivate()) {
		return cls == member.getClass();
	}

	if (member.isProtected()) {
		return cls->isAssignableFrom(jframe, member.getClass());
	}

	// package visibility not supported
	return false;
}

}

template <class T, IsJavaMember<T> = true>
void checkAccess(JPJavaFrame &jframe, const T &member) {
	if (canAccess(jframe, member)) {
		return;
	}
	if constexpr(std::is_same_v<T, JPField>) {
		JP_RAISE(PyExc_AttributeError, "Field is not visible");
	}
	JP_RAISE(PyExc_AttributeError, "Method is not visible");
}
