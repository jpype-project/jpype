#pragma once

#include "jp_class.h"
#include "jp_pythontypes.h"

#include <type_traits>
#include <vector>


struct JPMethodOverride {
	const JPClass *returnType;
	// when we move to a standard that isn't over 10 years old
	// this can probably become std::span
	std::vector<const JPClass *> paramTypes;
	JPPyObject function;
};


struct JPMethodOverrideList : std::vector<JPMethodOverride> {
	// extending it gets around incomplete type problems in jp_class.h
	using std::vector<JPMethodOverride>::vector;
};


template <class T, typename = std::enable_if_t<std::is_base_of<JPClass, T>::value>>
class JPExtensionType final : public T {

public:
	using T::T;
	virtual ~JPExtensionType() = default;
	virtual const JPMethodOverrideList *getOverrides() const override {
		return &m_Overrides;
	}
	virtual void addOverrides(JPMethodOverrideList &&overrides) override {
		m_Overrides = std::move(overrides);
	}

private:
	JPMethodOverrideList m_Overrides{};
};
