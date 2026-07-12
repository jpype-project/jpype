// --- file: common/jp_reference_queue.cpp ---
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
#include <jni.h>
#include <Python.h>
#include "jpype.h"
#include "jp_classloader.h"
#include "jp_reference_queue.h"
#include "jp_gc.h"
#include "pyjp.h"

extern "C"
{

static void releasePython(void* host)
{
	Py_XDECREF((PyObject*) host);
}

/*
 * Class:     org_jpype_ref_JPypeReferenceQueue
 * Method:    init
 * Signature: (JLjava/lang/Object;Ljava/lang/reflect/Method;)V
 */
JNIEXPORT void JNICALL Java_org_jpype_ref_NativeReference_init
(JNIEnv *env, jclass clazz, jlong ctx, jobject refqueue, jobject registerID)
{
	auto *context = (JPContext*) ctx;
	// Per-context field (not a file-scope static) - each subinterpreter's
	// NativeReferenceQueue gets its own binding here instead of clobbering
	// a single process-wide slot.  Released in JPContext::detachJVM().
	context->m_ReferenceQueue = env->NewGlobalRef(refqueue);
	context->m_ReferenceQueueRegisterMethod = env->FromReflectedMethod(registerID);
}

JNIEXPORT void JNICALL Java_org_jpype_ref_NativeReference_removeHostReference
(JNIEnv *env, jclass, jlong ctx, jlong host, jlong cleanup)
{
	// Exceptions are not allowed here
	try
	{
		auto *context = (JPContext*) ctx;
		if (context == nullptr || !context->isRunning())
			return;
		// Skip cleanup if Python is shutting down - phantom refs fire during JVM shutdown
		if (context->modulestate->is_shutting_down)
			return;
		JPJavaFrame frame = JPJavaFrame::external(env, context);
		JPPyCallAcquire callback(context->modulestate);
		if (cleanup != 0)
		{
			auto func = (JCleanupHook) cleanup;
			(*func)((void*) host);
		}
	} catch (...) // GCOVR_EXCL_LINE
	{
	}
}

/** Triggered whenever the sentinel is deleted
 */
JNIEXPORT void JNICALL Java_org_jpype_ref_NativeReference_wake
(JNIEnv *env, jclass clazz, jlong ctx)
{
	// Exceptions are not allowed here
	try
	{
		auto *context = (JPContext*) ctx;
		if (context == nullptr || !context->isRunning())
			return;
		// Skip GC trigger if Python is shutting down
		if (context->modulestate->is_shutting_down)
			return;
		JPPyCallAcquire callback(context->modulestate);
		context->m_GC->triggered();
	} catch (...) // GCOVR_EXCL_LINE
	{
	}
}

}

void JPReferenceQueue::registerRef(JPJavaFrame &frame, jobject obj, PyObject* hostRef)
{
	// There are certain calls such as exception handling in which the 
	// Python object is null.  In those cases, we don't need to bind the Java
	// object lifespan and can just ignore it.
	if (hostRef == nullptr)
		return;

	// MATCH TO DECREF IN releasePython
	Py_INCREF(hostRef);
	registerRef(frame, obj, hostRef, &releasePython);
}

void JPReferenceQueue::registerRef(JPJavaFrame &frame, jobject obj, void* host, JCleanupHook func)
{
	JP_TRACE_IN("JPReferenceQueue::registerRef");

	// create the ref ...
	jvalue args[3];
	args[0].l = obj;
	args[1].j = (jlong) host;
	args[2].j = (jlong) func;

	JPContext* context = frame.getContext();
	if (context->m_ReferenceQueue == nullptr)
		JP_RAISE(PyExc_SystemError, "Memory queue not installed");
	JP_TRACE("Register reference");
	frame.CallVoidMethodA(context->m_ReferenceQueue, context->m_ReferenceQueueRegisterMethod, args);
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}
