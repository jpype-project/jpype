/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#include "jpype.h"
#include "jp_conversioncache.h"

namespace
{
// JPMatch::Type only needs the low 3 bits; JPConversion* singletons are
// ordinary statically/heap allocated C++ objects, so their alignment always
// leaves at least those 3 low bits free to steal.
constexpr uintptr_t TYPE_MASK = 0x7;

uintptr_t pack(JPConversion* conversion, JPMatch::Type type)
{
	auto bits = reinterpret_cast<uintptr_t>(conversion);
	JP_TRACE("JPConversionCache::pack", (bits & TYPE_MASK) == 0);
	return bits | (static_cast<uintptr_t>(type) & TYPE_MASK);
}

JPConversion* unpackConversion(uintptr_t packed)
{
	return reinterpret_cast<JPConversion*>(packed & ~TYPE_MASK);
}

JPMatch::Type unpackType(uintptr_t packed)
{
	return static_cast<JPMatch::Type>(packed & TYPE_MASK);
}
} // namespace

bool JPConversionCache::lookup(PyTypeObject* key, JPConversion*& conversion, JPMatch::Type& type) const
{
	Entry entry = m_Slots[slot(key)].load(std::memory_order_relaxed);
	if (entry.key != key)
		return false;
	conversion = unpackConversion(entry.packed);
	type = unpackType(entry.packed);
	return true;
}

void JPConversionCache::store(PyTypeObject* key, JPConversion* conversion, JPMatch::Type type)
{
	Entry entry;
	entry.key = key;
	entry.packed = pack(conversion, type);
	m_Slots[slot(key)].store(entry, std::memory_order_relaxed);
}

void JPConversionCache::clear()
{
	Entry empty;
	empty.key = nullptr;
	empty.packed = 0;
	for (auto & s : m_Slots)
		s.store(empty, std::memory_order_relaxed);
}
