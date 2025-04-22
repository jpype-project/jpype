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

	explicit JPGarbageCollection();

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
	bool running;
	bool in_python_gc;
	bool java_triggered;
	PyObject *python_gc;
	jclass _SystemClass;
	jclass _ContextClass;
	jmethodID _gcMethodID;

	jmethodID _totalMemoryID;
	jmethodID _freeMemoryID;
	jmethodID _maxMemoryID;
	jmethodID _usedMemoryID;
	jmethodID _heapMemoryID;

	size_t last_python;
	size_t last_java;
	size_t low_water;
	size_t high_water;
	size_t limit;
	size_t last;
	int java_count;
	int python_count;
	int python_triggered;
} ;

#endif /* JP_GC_H */
