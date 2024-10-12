#pragma once

#include "jni.h"
#include "jp_class.h"
#include "jp_pythontypes.h"
#include "jp_value.h"

#include <vector>


struct JPMethodOverride {
	const JPClass *returnType;
	std::vector<const JPClass *> paramTypes;
	JPPyObject function;
};

using JPMethodOverrideList = std::vector<JPMethodOverride>;


class JPExtensionType final : public JPClass {

public:
	using JPClass::JPClass;
	virtual ~JPExtensionType() = default;

	JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast) override;
	const JPMethodOverrideList &getOverrides() const {
		return m_Overrides;
	}
	void setOverrides(JPJavaFrame& frame, PyObject *args);
	bool operator==(jobject obj) const {
		return m_Context->getEnv()->IsSameObject(obj, getJavaClass());
	}
	JPValue newInstance(JPJavaFrame& frame, JPPyObjectVector& args) override;
	PyObject *getPythonObject(JPJavaFrame& frame, JPValue &jv) const {
		return (PyObject *)frame.GetLongField(jv.getValue().l, m_Instance);
	}
private:
	JPMethodOverrideList m_Overrides{};
	jfieldID m_Instance{};
};
