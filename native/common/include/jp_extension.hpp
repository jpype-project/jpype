#pragma once

#include "jni.h"
#include "jp_class.h"
#include "jp_pythontypes.h"
#include "jp_value.h"
#include "pyjp.h"

#include <vector>


struct JPMethodOverride {
	JPClass *returnType;
	std::vector<JPClass *> paramTypes;
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
	void reset(JPJavaFrame& frame);
	void clearHost() {
		PyJPClass_clearJPClass(m_Host.get());
		m_Host = {};
	}
	bool wasReloaded() const {
		// to be checked before setting overrides
		// if this is true, then we are done in PyJPClass_init
		return m_Instance != nullptr;
	}
private:
	JPMethodOverrideList m_Overrides{};
	jfieldID m_Instance{};
};
