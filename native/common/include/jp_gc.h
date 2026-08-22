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

struct JPGCStats
{
	long long python_rss;
	long long java_rss;
	long long current_rss;
	long long max_rss;
	long long min_rss;
	long long python_triggered;
} ;

class JPGarbageCollection
{
public:

	explicit JPGarbageCollection(JPContext* context);

	void init(JPJavaFrame& frame);

	void shutdown();
	void triggered();

	/**
	 * Called when Python starts it Garbage collector
	 */
	void onStart();

	/**
	 * Called when Python finishes it Garbage collector
	 */
	void onEnd();

	void getStats(JPGCStats& stats);

private:
	size_t getWorkingSize();

	JPContext* m_Context;
	bool running;
	bool in_python_gc;
	bool java_triggered;
	PyObject *python_gc;

	size_t last_python;
	size_t last_java;
	size_t low_water;
	size_t high_water;
	size_t limit;
	size_t last;
	int java_count;
	int python_count;
	int python_triggered;

	// Only used by getWorkingSize()'s USE_PROC_INFO path (generic non-glibc
	// Linux). Per-instance rather than process-static: each interpreter
	// (main + every subinterpreter) has its own JPGarbageCollection, and a
	// shared fd would let one interpreter's shutdown() close a fd another
	// interpreter's getWorkingSize() is still reading from.
	int statm_fd;
	int page_size;
} ;

#endif /* JP_GC_H */
