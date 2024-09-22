#pragma once

#include "jp_class.h"
#include "jp_pythontypes.h"

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
	const JPMethodOverrideList *getOverrides() const {
		return &m_Overrides;
	}
	void setOverrides(PyObject *args);
private:
	JPMethodOverrideList m_Overrides{};
};
