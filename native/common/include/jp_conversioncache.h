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
#ifndef JP_CONVERSIONCACHE_H
#define JP_CONVERSIONCACHE_H

#include <array>
#include <atomic>
#include <cstdint>
#include <type_traits>

#include "jp_match.h"

class JPConversion;

/**
 * Fixed-size cache mapping a Python type to a previously resolved
 * JPConversion + match quality, used by JPClass::findJavaConversion to
 * skip re-running its (possibly long, e.g. hint-list-driven) conversion
 * search for a type it has already seen.
 *
 * Design:
 *  - Fixed 16 direct-mapped slots, not an unbounded map. Memory use per
 *    JPClass is constant (16 * 16 bytes); a hash collision just evicts the
 *    older entry, degrading to a miss rather than misbehaving.
 *  - Each slot is exactly 128 bits {key, packed(conversion, type)}, held in
 *    a single std::atomic<Entry> and always read/written as one unit. That
 *    is what rules out "shear": a concurrent reader can only ever observe a
 *    fully-old or fully-new entry, never a torn mix of an old key paired
 *    with a new conversion (or vice versa). Entry is a trivially-copyable
 *    16-byte POD, so std::atomic<Entry> is well-formed in C++14 on every
 *    platform this project targets (Linux/macOS/Windows); whether the
 *    platform backs it with a real lock-free 16-byte CAS (e.g. cmpxchg16b)
 *    or an internal library lock is an implementation detail that doesn't
 *    affect correctness -- either way, this is a short, contended-rarely
 *    critical section, cheap relative to what it replaces.
 *  - JPMatch::Type only needs 3 bits (0-4); JPConversion* singletons are
 *    static objects allocated with normal alignment (>= 8 bytes), leaving
 *    at least 3 always-zero low bits free to hold it, so {conversion, type}
 *    packs into a single pointer-sized word instead of needing a third
 *    field.
 *  - memory_order_relaxed is sufficient: the only data a slot needs to
 *    "publish" is other pointers to static, already-fully-constructed
 *    JPConversion singletons -- there is nothing else those pointers guard
 *    that a reader needs synchronized visibility into.
 */
class JPConversionCache
{
public:
	static constexpr size_t SLOTS = 16;

	// std::atomic<T>'s default constructor leaves the value uninitialized
	// pre-C++20 (this project targets C++14) -- zero every slot explicitly
	// rather than relying on {} to do it.
	JPConversionCache()
	{
		clear();
	}

	/**
	 * Look up key. On a hit, fills conversion/type and returns true.
	 */
	bool lookup(PyTypeObject* key, JPConversion*& conversion, JPMatch::Type& type) const;

	/**
	 * Store (overwriting whatever was in that slot, if anything).
	 */
	void store(PyTypeObject* key, JPConversion* conversion, JPMatch::Type type);

	/** Evict everything (e.g. on JPClassHints::s_Generation change). */
	void clear();

private:
	struct Entry
	{
		PyTypeObject* key;
		uintptr_t packed;
	} ;
	static_assert(sizeof(Entry) == 16, "JPConversionCache::Entry must be exactly 128 bits");
	static_assert(std::is_trivially_copyable<Entry>::value, "JPConversionCache::Entry must be trivially copyable for std::atomic<Entry>");

	static size_t slot(PyTypeObject* key)
	{
		// Types are heap-allocated PyTypeObjects with normal alignment;
		// shift off the always-zero low bits before masking down to an
		// index so nearby-allocated types don't all collide on slot 0.
		return (reinterpret_cast<uintptr_t>(key) >> 4) & (SLOTS - 1);
	}

	std::array<std::atomic<Entry>, SLOTS> m_Slots;
} ;

#endif /* JP_CONVERSIONCACHE_H */
