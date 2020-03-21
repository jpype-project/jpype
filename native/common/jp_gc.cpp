#include <Python.h>
#include "jpype.h"
#include "pyjp.h"
#include "jp_reference_queue.h"

#ifdef WIN32
#define USE_PROCESS_INFO
#include <Windows.h>
#include <psapi.h>
#define DELTA_LIMIT 5*1024*1024
#define SOFT_LIMIT 60*1024*1024
#define HARD_LIMIT 200*1024*1024
//#define USE_RESOURCE
//#include <sys/resource.h>
//#define DELTA_LIMIT 5*1024
//#define SOFT_LIMIT 60*1024
//#define HARD_LIMIT 200*1024
#elif __APPLE__
#define USE_TASK_INFO
#include <unistd.h>
#include <sys/resource.h>
#include <mach/mach.h>
#define DELTA_LIMIT 5*1024*1024
#define SOFT_LIMIT 60*1024*1024
#define HARD_LIMIT 200*1024*1024
#else
#define USE_MALLINFO
#include <malloc.h>
#define DELTA_LIMIT 5*1024*1024
#define SOFT_LIMIT 60*1024*1024
#define HARD_LIMIT 200*1024*1024
#endif

namespace
{
bool running = false;
bool in_python_gc = false;
bool java_triggered = false;
PyObject *python_gc = NULL;
jclass _SystemClass = NULL;
jmethodID _gcMethodID;

ssize_t last_python = 0;
ssize_t last_java = 0;
ssize_t low_water = 0;
ssize_t high_water = 0;
float last_fraction = 0;
int skip_counter = 0;
int skip_last = 0;
int java_count = 0;
int python_count = 0;
}

ssize_t getWorkingSize()
{
	ssize_t current = 0;
#if defined(USE_PROCESS_INFO)
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof (pmc));
	current = pmc.WorkingSetSize;

#elif defined(USE_TASK_INFO)
	struct mach_task_basic_info info;
	mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) & info, &count) == KERN_SUCCESS)
		current = (size_t) info.resident_size;
	return (size_t) 0; /* query failed */

#elif defined(USE_MALLINFO)
	struct mallinfo mi;
	mi = mallinfo();
	current = mi.uordblks;
#endif

	if (current > high_water)
	{
		high_water = current;
		low_water = high_water;
	}
	if (current < low_water)
	{
		low_water = current;
	}
	return current;

}

void triggerPythonGC();

extern "C" void callbackJavaGCTriggered(void* context)
{
	// Don't reinstall the sentinel if we are terminated
	if (!running)
		return;
	// Install a new sentinel
	JPJavaFrame frame((JPContext*) context);
	jobject sentinel = frame.NewByteArray(0);
	frame.getContext()->getReferenceQueue()->registerRef(sentinel, context, callbackJavaGCTriggered);

	// If we were triggered from Java call a Python cleanup
	if (!in_python_gc)
	{
		// trigger Python gc
		in_python_gc = true;
		java_triggered = true;
		java_count++;
		PyGC_Collect();
	}
}

void JPGarbageCollection::init(JPJavaFrame& frame)
{
	// Install a sentinel to detect when Java has started a GC cycle
	jobject sentinel = frame.NewByteArray(0);
	JPContext *context = frame.getContext();
	context->getReferenceQueue()->registerRef(sentinel, context, callbackJavaGCTriggered);

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
	if (in_python_gc)
	{
		python_count++;
		int run_gc = 0;

		ssize_t current = getWorkingSize();

		if (java_triggered)
			last_java = current;
		else
			last_python = current;

		bool bump = high_water == low_water;
		float fraction = 0;
		if (!bump)
			fraction = (current - low_water) / (float) (high_water - low_water);
		// Decide the policy
		if (current > SOFT_LIMIT && bump)
			run_gc = 1;
		if (fraction > last_fraction && fraction + 4 * (fraction - last_fraction) > 1.0)
			run_gc = 2;
		if (current > HARD_LIMIT)
			run_gc = 4;
		if (last_python > current)
			last_python = current;

		printf("consider gc %d (%ld,%ld) %f\n", run_gc,
				low_water, high_water, fraction);

		if (run_gc > 0)
		{
			last_fraction = 1;
			// Don't reset the limit if it was count triggered
			if (run_gc != 3 && skip_counter > 0)
				skip_last = skip_counter + 5;
			skip_counter = 0;
			JPJavaFrame frame(m_Context);
			frame.CallStaticVoidMethodA(_SystemClass, _gcMethodID, 0);
		} else
		{
			last_fraction = fraction;
			skip_counter++;
		}
	}
	// Remove our lock so that we can watch for triggers
	in_python_gc = false;
	java_triggered = false;
}

void JPGarbageCollection::getStats(JPGCStats& stats)
{
	stats.current_rss = getWorkingSize();
	stats.min_rss = low_water;
	stats.max_rss = high_water;
	stats.java_rss = last_java;
	stats.python_rss = last_python;
}
