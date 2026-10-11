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

static jobject s_ReferenceQueue = nullptr;
static jmethodID s_ReferenceQueueRegisterMethod = nullptr;

extern "C"
{

static void releasePython(void* host)
{
	Py_XDECREF((PyObject*) host);
}

/*
 * Class:     org_jpype_ref_JPypeReferenceQueue
 * Method:    init
 * Signature: (Ljava/lang/Object;Ljava/lang/reflect/Method;)V
 */
JNIEXPORT void JNICALL Java_org_jpype_ref_JPypeReferenceNative_init
(JNIEnv *env, jclass clazz, jobject refqueue, jobject registerID)
{
	s_ReferenceQueue = env->NewGlobalRef(refqueue);
	s_ReferenceQueueRegisterMethod = env->FromReflectedMethod(registerID);
}

JNIEXPORT void JNICALL Java_org_jpype_ref_JPypeReferenceNative_removeHostReference
(JNIEnv *env, jclass, jlong host, jlong cleanup)
{
	// Exceptions are not allowed here
	try
	{
		JPJavaFrame frame = JPJavaFrame::external(env);
		JPPyCallAcquire callback;
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
JNIEXPORT void JNICALL Java_org_jpype_ref_JPypeReferenceNative_wake
(JNIEnv *env, jclass clazz)
{
	// Exceptions are not allowed here
	try
	{
		JPContext* context = JPContext_global;
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

	// MATCH TO DECREF IN releasePython -- but releasePython only ever
	// runs via the Java-side reference queue's phantom-ref callback,
	// which requires registration below to have actually succeeded. If
	// it throws instead (queue not installed, or the registration JNI
	// call itself raises), that callback never gets a chance to fire, so
	// the incref must be undone here on the throw path or hostRef leaks
	// permanently.
	Py_INCREF(hostRef);
	try
	{
		registerRef(frame, obj, hostRef, &releasePython);
		// GCOVR_EXCL_START
		// This call's only site (jp_exception.cpp's Python-to-Java
		// exception conversion) is itself wrapped in a separate,
		// pre-existing fail-fast guard that deliberately crashes the
		// process on any exception there -- so this catch is unreachable
		// through that call site without also triggering that guard
		// first. Kept as defense-in-depth for any future caller.
	} catch (...)
	{
		Py_DECREF(hostRef);
		throw;
	}
	// GCOVR_EXCL_STOP
}

void JPReferenceQueue::registerRef(JPJavaFrame &frame, jobject obj, void* host, JCleanupHook func)
{
	JP_TRACE_IN("JPReferenceQueue::registerRef");

	// create the ref ...
	jvalue args[3];
	args[0].l = obj;
	args[1].j = (jlong) host;
	args[2].j = (jlong) func;

	if (s_ReferenceQueue == nullptr)
		JP_RAISE(PyExc_SystemError, "Memory queue not installed");
	JP_TRACE("Register reference");
	frame.CallVoidMethodA(s_ReferenceQueue, s_ReferenceQueueRegisterMethod, args);
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}
