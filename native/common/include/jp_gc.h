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
#ifndef JP_GC_H
#define JP_GC_H

#include <atomic>
#include <cstdint>

struct JPGCStats
{
	long long python_rss;
	long long java_rss;
	long long current_rss;
	long long max_rss;
	long long min_rss;
	long long python_triggered;
} ;

class JPGarbageCollection {
public:

	explicit JPGarbageCollection(JPContext* context);

	void init(JPJavaFrame& frame);

	void shutdown();

	/**
	 * Called from the Java reference thread when a Java collection has
	 * happened.  Reflects it into a Python collection, at most one at a
	 * time and throttled by the cost of the previous reflection.
	 */
	void triggered();

	/**
	 * Called when Python starts it Garbage collector
	 */
	void onStart();

	/**
	 * Called when Python finishes it Garbage collector
	 */
	void onEnd();

	void getStats(JPGCStats& stats) const;

private:
	size_t getWorkingSize() const;

	JPContext* m_Context;
	std::atomic<bool> running{false};
	std::atomic<bool> in_python_gc{false};
	// Claimed by the thread performing a forced (Java triggered) collection.
	std::atomic<bool> forcing_gc{false};
	// Earliest time (steady clock, ns) at which another forced collection
	// may start.
	std::atomic<int64_t> next_forced_ns{0};
	PyObject *python_gc{};

	size_t last_python{};
	size_t last_java{};
	size_t low_water{};
	size_t high_water{};
	size_t limit{};
	size_t last{};
	int python_count{};
	int python_triggered{};

	// Only used by getWorkingSize()'s USE_PROC_INFO path (generic non-glibc
	// Linux). Per-instance rather than process-static: each interpreter
	// (main + every subinterpreter) has its own JPGarbageCollection, and a
	// shared fd would let one interpreter's shutdown() close a fd another
	// interpreter's getWorkingSize() is still reading from.  Atomic because
	// shutdown() can race the reference thread.
	std::atomic<int> statm_fd{-1};
	int page_size{};
} ;

#endif /* JP_GC_H */
