#include <Python.h>
#include "jpype.h"
#include "pyjp.h"
#include "jp_reference_queue.h"

#ifdef WIN32
#define USE_PROCESS_INFO
#include <Windows.h>
#include <psapi.h>
#elif __APPLE__
#define USE_TASK_INFO
#include <unistd.h>
#include <sys/resource.h>
#include <mach/mach.h>
#else
// Linux doesn't have an available rss tally so use mallinfo
#define USE_MALLINFO
#include <malloc.h>
#endif
#define DELTA_LIMIT 10*1024*1024l
#define HARD_LIMIT 200*1024*1024l

namespace
{
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
int skip_counter = 0;
int skip_last = 0;
int java_count = 0;
int python_count = 0;
}

size_t getWorkingSize()
{
	size_t current = 0;
#if defined(USE_PROCESS_INFO)
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof (pmc));
	current = (size_t) pmc.WorkingSetSize;

#elif defined(USE_TASK_INFO)
	struct mach_task_basic_info info;
	mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) & info, &count) == KERN_SUCCESS)
		current = (size_t) info.resident_size;
	return (size_t) 0; /* query failed */

#elif defined(USE_MALLINFO)
	struct mallinfo mi;
	mi = mallinfo();
	current = (size_t) mi.uordblks;
#endif

	if (current > high_water)
		high_water = current;
	if (current < low_water)
		low_water = current;
	return current;
}

void triggerPythonGC();

void JPGarbageCollection::triggered()
{
	// If we were triggered from Java call a Python cleanup
	if (!in_python_gc)
	{
		// trigger Python gc
		in_python_gc = true;
		java_triggered = true;
		java_count++;

		// Lock Python so we call trigger a GC
		JPPyCallAcquire callback;
		PyGC_Collect();
	}
}

void JPGarbageCollection::init(JPJavaFrame& frame)
{
	// Get the Python garbage collector
	JPPyObject gc(JPPyRef::_call, PyImport_ImportModule("gc"));
	python_gc = gc.keep();

	// Find the callbacks
	JPPyObject callbacks(JPPyRef::_call, PyObject_GetAttrString(python_gc, "callbacks"));

	// Hook up our callback
	JPPyObject collect(JPPyRef::_call, PyObject_GetAttrString(PyJPModule, "_collect"));
	PyList_Append(callbacks.get(), collect.get());
	JP_PY_CHECK();

	// Get the Java System gc so we can trigger
	_SystemClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/System"));
	_gcMethodID = frame.GetStaticMethodID(_SystemClass, "gc", "()V");

	running = true;
	getWorkingSize();
	limit = high_water + DELTA_LIMIT;
}

void JPGarbageCollection::shutdown()
{
	running = false;
}

void JPGarbageCollection::onStart()
{
	if (!running)
		return;
	getWorkingSize();
	in_python_gc = true;
}

void JPGarbageCollection::onEnd()
{
	if (!running)
		return;
	if (java_triggered)
	{
		// Remove our lock so that we can watch for triggers
		java_triggered = false;
		return;
	}
	if (in_python_gc)
	{
		in_python_gc = false;
		python_count++;
		int run_gc = 0;

		size_t current = getWorkingSize();

		if (java_triggered)
			last_java = current;
		else
			last_python = current;

		// Things are getting better so use high water as limit
		if (current == low_water)
		{
			limit = (limit + high_water) / 2;
			if ( high_water > low_water + 4 * DELTA_LIMIT)
				high_water = low_water + 4 * DELTA_LIMIT;
		}

		if (last_python > current)
			last_python = current;

		if (current < last)
		{
			last = current;
			return;
		}
		last = current;

		// Decide the policy
		if (current > limit)
		{
			limit = high_water + DELTA_LIMIT;
			run_gc = 1;
		}

		// Predict if we will cross the limit soon.
		ssize_t pred = current + 2 * (current - last);
		if (pred > limit)
			run_gc = 2;

		//		printf("consider gc %d (%ld, %ld, %ld, %ld) %ld\n", run_gc,
		//				current, low_water, high_water, limit, limit - pred);

		if (run_gc > 0)
		{
			low_water = (low_water + high_water) / 2;
			// Don't reset the limit if it was count triggered
			if (run_gc != 3 && skip_counter > 0)
				skip_last = skip_counter + 5;
			skip_counter = 0;
			JPJavaFrame frame(m_Context);
			frame.CallStaticVoidMethodA(_SystemClass, _gcMethodID, 0);
		} else
		{
			skip_counter++;
		}
	}
}

void JPGarbageCollection::getStats(JPGCStats& stats)
{
	stats.current_rss = getWorkingSize();
	stats.min_rss = low_water;
	stats.max_rss = high_water;
	stats.java_rss = last_java;
	stats.python_rss = last_python;
}
