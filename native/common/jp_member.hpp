#pragma once

#include "jp_field.h"
#include "jp_javaframe.h"
#include "jp_method.h"
#include "pyjp.h"

#include <pyconfig.h>
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

	PyObject *locals = PyEval_GetLocals();
	if (locals == nullptr) {
		// access denied
		return false;
	}

	JPPyObject obj = JPPyObject::call(PyMapping_GetItemString(locals, "self"));
	if (obj.get() != nullptr) {
		obj = JPPyObject::use((PyObject *) Py_TYPE(obj.get()));
	} else {
		obj = JPPyObject::call(PyMapping_GetItemString(locals, "cls"));
	}

	if (obj.isNull()) {
		return false;
	}

	JPClass *cls = PyJPClass_getJPClass(obj.get());
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
