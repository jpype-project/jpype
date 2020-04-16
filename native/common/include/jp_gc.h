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

	JPGarbageCollection(JPContext *context);

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
	JPContext *m_Context;
	bool running;
	bool in_python_gc;
	bool java_triggered;
	PyObject *python_gc;
	jclass _SystemClass;
	jmethodID _gcMethodID;

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

