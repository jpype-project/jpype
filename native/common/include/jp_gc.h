#ifndef JP_GC_H
#define JP_GC_H

struct JPGCStats
{
	long long python_rss;
	long long java_rss;
	long long current_rss;
	long long max_rss;
	long long min_rss;
} ;

class JPGarbageCollection
{
public:

	JPGarbageCollection(JPContext *context)
	{
		m_Context = context;
	}

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
	bool running = false;
	bool in_python_gc = false;
	bool java_triggered = false;
	PyObject *python_gc = NULL;
	jclass _SystemClass = NULL;
	jmethodID _gcMethodID;

	size_t last_python = 0;
	size_t last_java = 0;
	size_t low_water = 0;
	size_t high_water = 0;
	size_t limit = 0;
	size_t last = 0;
	int java_count = 0;
	int python_count = 0;
} ;

#endif /* JP_GC_H */

