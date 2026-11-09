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
#include <Python.h>
#include "jpype.h"
#include "pyjp.h"
#include "jp_gc.h"
#include <algorithm>
#include <chrono>

#ifdef WIN32
#define USE_PROCESS_INFO
#include <Windows.h>
#include <psapi.h>

#elif __APPLE__
#define USE_TASK_INFO
#include <unistd.h>
#include <sys/resource.h>
#include <mach/mach.h>

#elif __GLIBC__
// Linux doesn't have an available rss tally so use mallinfo
#define USE_MALLINFO
#include <malloc.h>

#elif __linux__
#define USE_PROC_INFO
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
static std::atomic<int> statm_fd{-1};
static int page_size;

#else
#define USE_NONE
#endif
#define DELTA_LIMIT (20*1024*1024l)

// A collection forced from a Java collection must not take more than about
// one part in FORCED_GC_BACKOFF of the wall time, and never runs more often
// than FORCED_GC_MIN_INTERVAL_NS.  Java can collect many times a second
// under allocation pressure, and each forced Python collection is a full one.
static const int64_t FORCED_GC_MIN_INTERVAL_NS = 50 * 1000 * 1000;
static const int64_t FORCED_GC_BACKOFF = 4;

// Set on the reference thread while it runs a forced collection so that the
// gc callbacks fired by that collection can recognize it as their own.
static thread_local bool forcing_collection = false;

static int64_t nowNs()
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
}

static size_t getWorkingSize()
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

#elif defined(USE_PROC_INFO)
	char bytes[32];
	int fd = statm_fd.load();
	if (fd < 0)
		return 0;
	// pread so that concurrent callers do not share a file offset
	int len = (int) pread(fd, bytes, sizeof (bytes), 0);
	long long sz = 0;
	int i = 0;
	for (; i < len; i++)
	{
		if (bytes[i] == ' ')
			break;
	}
	i++;
	for (; i < len; i++)
	{
		if (bytes[i] == ' ')
			return sz * page_size;
		sz *= 10;
		sz += bytes[i] - '0';
	}
	return sz * page_size;

#elif defined(USE_MALLINFO)
	struct mallinfo mi = mallinfo(); // fixme: mallinfo is deprecated in favor of mallinfo2 since glibc 2.33
	current = mi.uordblks;
#endif

	return current;
}

void JPGarbageCollection::triggered()
{
	if (!running)
		return;

	// Only one forced collection at a time, and not before the cool down
	// from the previous one has expired.
	if (forcing_gc.exchange(true))
		return;
	if (nowNs() < next_forced_ns.load())
	{
		forcing_gc = false;
		return;
	}

	int64_t start = nowNs();
	{
		forcing_collection = true;
		try
		{
			// Lock Python so we can trigger a GC
			JPPyCallAcquire callback;
			if (running)
				PyGC_Collect();
		} catch (...) // GCOVR_EXCL_LINE
		{
		}
		forcing_collection = false;
	}
	int64_t end = nowNs();
	// Parenthesized so that the max macro from Windows.h does not apply
	next_forced_ns = end + (std::max)(FORCED_GC_MIN_INTERVAL_NS,
			FORCED_GC_BACKOFF * (end - start));
	forcing_gc = false;
}

void JPGarbageCollection::init(JPJavaFrame& frame)
{
#if defined(USE_PROC_INFO)
	statm_fd = open("/proc/self/statm", O_RDONLY);
	page_size = getpagesize();
#endif
	// Get the Python garbage collector
	JPPyObject gc = JPPyObject::call(PyImport_ImportModule("gc"));
	python_gc = gc.keep();

	// Find the callbacks
	JPPyObject callbacks = JPPyObject::call(PyObject_GetAttrString(python_gc, "callbacks"));

	// Hook up our callback unless the module already did
	JPPyObject collect = JPPyObject::call(PyObject_GetAttrString(PyJPModule, "_collect"));
	int installed = PySequence_Contains(callbacks.get(), collect.get());
	if (installed < 0)
		JP_PY_CHECK();
	if (installed == 0)
	{
		PyList_Append(callbacks.get(), collect.get());
		JP_PY_CHECK();
	}

	// Get the Java System gc so we can trigger
	_SystemClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/System"));
	_gcMethodID = frame.GetStaticMethodID(_SystemClass, "gc", "()V");

	jclass ctxt = JPContext_global->m_ContextClass.get();
	_ContextClass = ctxt;

	running = true;
	high_water = getWorkingSize();
	limit = high_water + DELTA_LIMIT;
}

void JPGarbageCollection::shutdown()
{
	running = false;
#if defined(USE_PROC_INFO)
	int fd = statm_fd.exchange(-1);
	if (fd >= 0)
		close(fd);
#endif
}

void JPGarbageCollection::onStart()
{
	// GCOVR_EXCL_START
	// GC is triggered outside of user control.  Including it in
	// coverage just creates random statistics.
	if (!running || forcing_collection)
		return;
	in_python_gc = true;
	// GCOVR_EXCL_STOP
}

void JPGarbageCollection::onEnd()
{
	// GCOVR_EXCL_START
	// GC is triggered outside of user control.  Including it in
	// coverage just creates random statistics.
	if (!running || forcing_collection)
		return;
	if (in_python_gc.exchange(false))
	{
		python_count++;
		int run_gc = 0;

		size_t current = getWorkingSize();
		if (current > high_water)
			high_water = current;
		if (current < low_water)
			low_water = current;

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

		// Decide the policy
		if (current > limit)
		{
			limit = high_water + DELTA_LIMIT;
			run_gc = 1;
		}

		// Predict if we will cross the limit soon.
		Py_ssize_t slope = (Py_ssize_t) current - (Py_ssize_t) last;
		Py_ssize_t pred = (Py_ssize_t) current + 2 * slope;
		last = current;
		if (pred > (Py_ssize_t) limit)
		{
			run_gc = 2;
			// Headroom is deliberately not scaled by the growth rate: that
			// raises the limit fastest when memory grows fastest.
			limit = high_water + (high_water>>3);
		}

		if (run_gc > 0)
		{
			// Move up the low water
			low_water = (low_water + high_water) / 2;
			// Don't reset the limit if it was count triggered
			JPJavaFrame frame = JPJavaFrame::outer();
			frame.CallStaticVoidMethodA(_SystemClass, _gcMethodID, nullptr);
			python_triggered++;
		}
	}
	// GCOVR_EXCL_STOP
}

void JPGarbageCollection::getStats(JPGCStats& stats) const {
	// GCOVR_EXCL_START
	stats.current_rss = getWorkingSize();
	stats.min_rss = low_water;
	stats.max_rss = high_water;
	stats.java_rss = last_java;
	stats.python_rss = last_python;
	stats.python_triggered = python_triggered;
	// GCOVR_EXCL_STOP
}
