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
#include "jp_reference_queue.h"
#include "jp_gc.h"

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
static int statm_fd;
static int page_size;

#else
#define USE_NONE
#endif
#define DELTA_LIMIT 20*1024*1024l

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

#elif defined(USE_PROC_INFO)
	char bytes[32];
	lseek(statm_fd, SEEK_SET, 0);
	int len = read(statm_fd, bytes, 32);
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
	struct mallinfo mi;
	mi = mallinfo();
	current = (size_t) mi.uordblks;
#endif

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

JPGarbageCollection::JPGarbageCollection()
{
	running = false;
	in_python_gc = false;
	java_triggered = false;
	python_gc = nullptr;
	_SystemClass = nullptr;
	_gcMethodID = nullptr;

	last_python = 0;
	last_java = 0;
	low_water = 0;
	high_water = 0;
	limit = 0;
	last = 0;
	java_count = 0;
	python_count = 0;
	python_triggered = 0;
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

	// Hook up our callback
	JPPyObject collect = JPPyObject::call(PyObject_GetAttrString(PyJPModule, "_collect"));
	PyList_Append(callbacks.get(), collect.get());
	JP_PY_CHECK();

	// Get the Java System gc so we can trigger
	_SystemClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/System"));
	_gcMethodID = frame.GetStaticMethodID(_SystemClass, "gc", "()V");

	jclass ctxt = JPContext_global->m_ContextClass.get();
	_ContextClass = ctxt;
	_totalMemoryID = frame.GetStaticMethodID(ctxt, "getTotalMemory", "()J");
	_freeMemoryID = frame.GetStaticMethodID(ctxt, "getFreeMemory", "()J");
	_maxMemoryID = frame.GetStaticMethodID(ctxt, "getMaxMemory", "()J");
	_usedMemoryID = frame.GetStaticMethodID(ctxt, "getUsedMemory", "()J");
	_heapMemoryID = frame.GetStaticMethodID(ctxt, "getHeapMemory", "()J");

	running = true;
	high_water = getWorkingSize();
	limit = high_water + DELTA_LIMIT;
}

void JPGarbageCollection::shutdown()
{
	running = false;
#if defined(USE_PROC_INFO)
	close(statm_fd);
#endif
}

void JPGarbageCollection::onStart()
{
	// GCOVR_EXCL_START
	// GC is triggered outside of user control.  Including it in
	// coverage just creates random statistics.
	if (!running)
		return;
	getWorkingSize();
	in_python_gc = true;
	// GCOVR_EXCL_STOP
}

void JPGarbageCollection::onEnd()
{
	// GCOVR_EXCL_START
	// GC is triggered outside of user control.  Including it in
	// coverage just creates random statistics.
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
		if (current > high_water)
			high_water = current;
		if (current < low_water)
			low_water = current;

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

		// Decide the policy
		if (current > limit)
		{
			limit = high_water + DELTA_LIMIT;
			run_gc = 1;
		}

		// Predict if we will cross the limit soon.
		Py_ssize_t pred = current + 2 * (current - last);
		last = current;
		if ((Py_ssize_t) pred > (Py_ssize_t) limit)
		{
			run_gc = 2;
			limit = high_water + (high_water>>3) + 8 * (current - last);
		}

#if 0
		{
			JPJavaFrame frame = JPJavaFrame::outer();
			jlong totalMemory = frame.CallStaticLongMethodA(_ContextClass, _totalMemoryID, nullptr);
			jlong freeMemory = frame.CallStaticLongMethodA(_ContextClass, _freeMemoryID, nullptr);
			jlong maxMemory = frame.CallStaticLongMethodA(_ContextClass, _maxMemoryID, nullptr);
			jlong usedMemory = frame.CallStaticLongMethodA(_ContextClass, _usedMemoryID, nullptr);
			jlong heapMemory = frame.CallStaticLongMethodA(_ContextClass, _heapMemoryID, nullptr);
			printf("consider gc run=%d (current=%ld, low=%ld, high=%ld, limit=%ld) %ld\n", run_gc,
				current, low_water, high_water, limit, limit - pred);
			printf(" java total=%ld free=%ld max=%ld used=%ld heap=%ld\n", totalMemory, freeMemory, maxMemory, usedMemory, heapMemory);
		}
#endif

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

void JPGarbageCollection::getStats(JPGCStats& stats)
{
	// GCOVR_EXCL_START
	stats.current_rss = getWorkingSize();
	stats.min_rss = low_water;
	stats.max_rss = high_water;
	stats.java_rss = last_java;
	stats.python_rss = last_python;
	stats.python_triggered = python_triggered;
	// GCOVR_EXCL_STOP
}
