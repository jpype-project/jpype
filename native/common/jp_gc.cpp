#include <Python.h>
#include "jpype.h"
#include "jp_reference_queue.h"

#ifndef WIN32
#include <sys/resource.h>
#endif

namespace
{
bool in_python_gc = false;
PyObject *python_gc = NULL;
jclass _SystemClass = NULL;
jmethodID _gcMethodID;
//PyMemAllocatorEx raw_allocators;
}

void triggerPythonGC();

extern "C" void callbackJavaGCTriggered(void* n)
{
	printf("Sentinel trigger\n");
	// Install a new sentinel
	JPJavaFrame frame;
	jobject sentinel = frame.NewByteArray(0);
	JPReferenceQueue::registerRef(sentinel, 0, callbackJavaGCTriggered);
	if (!in_python_gc)
	{
		printf("Force Python\n");
		// trigger Python gc
		in_python_gc = true;
		PyGC_Collect();
	}
}

//extern "C" void* gc_malloc_raw(void *ctx, size_t sz)
//{
//	printf("Malloc %lu\n", sz);
//	PyMemAllocatorEx *alloc = (PyMemAllocatorEx *) ctx;
//	return raw_allocators.malloc(raw_allocators.ctx, sz);
//}
//
//extern "C" void* gc_calloc_raw(void *ctx, size_t nelem, size_t elsize)
//{
//	printf("Calloc %lu\n", nelem * elsize);
//	PyMemAllocatorEx *alloc = (PyMemAllocatorEx *) ctx;
//	PyMemAllocatorEx *prior = (PyMemAllocatorEx *) alloc->ctx;
//	return raw_allocators.calloc(raw_allocators.ctx, nelem, elsize);
//}
//
//extern "C" void* gc_realloc_raw(void *ctx, void* ptr, size_t size)
//{
//	printf("Realloc %lu\n", size);
//	PyMemAllocatorEx *alloc = (PyMemAllocatorEx *) ctx;
//	PyMemAllocatorEx *prior = (PyMemAllocatorEx *) alloc->ctx;
//	return raw_allocators.realloc(raw_allocators.ctx, ptr, size);
//}
//
//extern "C" void gc_free_raw(void *ctx, void* ptr)
//{
//	printf("Free %p\n", ptr);
//	PyMemAllocatorEx *alloc = (PyMemAllocatorEx *) ctx;
//	PyMemAllocatorEx *prior = (PyMemAllocatorEx *) alloc->ctx;
//	raw_allocators.free(raw_allocators.ctx, ptr);
//}

// Connection to numpy if using a weak import if possible
//typedef void (*PyDataMem_EventHookFunc)(void *inp, void *outp, size_t size,
//		void *user_data);

void **PyArray_API;
typedef void (PyDataMem_EventHookFunc) (void *inp, void *outp, size_t size,
		void *user_data);
typedef PyDataMem_EventHookFunc * PyDataMem_SetEventHook \
       (PyDataMem_EventHookFunc *, void *, void **);
#define PyDataMem_SetEventHook \
        (*(PyDataMem_EventHookFunc * (*)(PyDataMem_EventHookFunc *, void *, void **)) \
         PyArray_API[291])

//static void myevent_hook(void *inp, void *outp, size_t size,
//		void *user_data)
//{
//	if (inp == NULL)
//	{
//		ssize_t* q = (ssize_t*) outp;
//		printf("Hook no in %d %p %d\n", size, outp, q[-1]);
//	}
//	if (size == 0)
//	{
//		ssize_t* q = (ssize_t*) inp;
//		printf("Hook no size %p %d\n", inp, q[-1]);
//	}
//}

void JPGarbageCollection::init()
{
	JPJavaFrame frame;
	// Install a sentinel to detect when Java has started a GC cycle
	jobject sentinel = frame.NewByteArray(0);
	JPReferenceQueue::registerRef(sentinel, 0, callbackJavaGCTriggered);

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


	//	PyObject *numpy = PyImport_ImportModule("numpy.core._multiarray_umath");
	//	if (numpy != NULL)
	//	{
	//		PyObject *c_api = PyObject_GetAttrString(numpy, "_ARRAY_API");
	//		PyArray_API = (void **) PyCapsule_GetPointer(c_api, NULL);
	//		printf("array %p\n", PyArray_API[291]);
	//		void *old_data;
	//		PyDataMem_SetEventHook(myevent_hook, NULL, &old_data);
	//	} else
	//	{
	//		PyErr_Clear();
	//	}
	// Connect into Python so we know when we need to trigger a Java collection
	//	PyMemAllocatorEx alloc;
	//	alloc.malloc = gc_malloc_raw;
	//	alloc.calloc = gc_calloc_raw;
	//	alloc.realloc = gc_realloc_raw;
	//	alloc.free = gc_free_raw;
	//
	//	alloc.ctx = &raw_allocators;
	//	PyMem_GetAllocator(PYMEM_DOMAIN_MEM, &raw_allocators);
	//	PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &alloc);
}

void JPGarbageCollection::onStart()
{
	if (in_python_gc)
		return;
	printf("Start Python\n");
	JPJavaFrame frame;
	in_python_gc = true;
	//	printf("Force Java\n");
	//	frame.CallStaticVoidMethodA(_SystemClass, _gcMethodID, 0);
}

void JPGarbageCollection::onEnd()
{
	printf("End Python\n");
	if (in_python_gc)
	{
		rusage usage;
		getrusage(RUSAGE_SELF, &usage);
		printf("usage %ld\n", usage.ru_maxrss);
	}
	// Remove our lock so that we can watch for triggers
	in_python_gc = false;
}
