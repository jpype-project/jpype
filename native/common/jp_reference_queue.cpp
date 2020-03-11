/*****************************************************************************
   Copyright 2004-2008 Steve Ménard

   Licensed under the Apache License, Version 2.0 (the "License
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

 *****************************************************************************/
#include <Python.h>
#include "jpype.h"
#include "jp_classloader.h"
#include "jp_reference_queue.h"

extern "C"
{

static void releasePython(void* host)
{
	Py_XDECREF((PyObject*) host);
}

}

JNIEXPORT void JNICALL Java_jpype_ref_JPypeReferenceQueue_removeHostReference(
		JNIEnv *env, jclass clazz, jlong contextPtr, jlong host, jlong cleanup)
{
	JP_TRACE_IN("JPype_ReferenceQueue_removeHostReference");
	JPContext *context = (JPContext*) contextPtr;
	JPJavaFrame frame((JPContext*) context, env);
	JPPyCallAcquire callback;
	if (cleanup != 0)
	{
		JCleanupHook func = (JCleanupHook) cleanup;
		(*func)((void*) host);
	}
	JP_TRACE_OUT;
}

JPReferenceQueue::JPReferenceQueue(JPJavaFrame& frame)
{
	JP_TRACE_IN("JPReferenceQueue::init");
	m_Context = frame.getContext();

	// build the ReferenceQueue class ...
	jclass cls = m_Context->getClassLoader()
			->findClass(frame, "org.jpype.ref.JPypeReferenceQueue");

	//Required due to bug in jvm
	//See: http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6493522
	frame.GetMethodID(cls, "<init>", "()V");

	JNINativeMethod method2[1];
	method2[0].name = (char*) "removeHostReference";
	method2[0].signature = (char*) "(JJJ)V";
	method2[0].fnPtr = (void*) &Java_jpype_ref_JPypeReferenceQueue_removeHostReference;
	frame.RegisterNatives(cls, method2, 1);

	// Get all required methods
	m_ReferenceQueueRegisterMethod = frame.GetMethodID(cls, "registerRef", "(Ljava/lang/Object;JJ)V");

	JP_TRACE_OUT;
}

JPReferenceQueue::~JPReferenceQueue()
{
}

void JPReferenceQueue::registerRef(jobject obj, PyObject* hostRef)
{
	// MATCH TO DECREF IN unreferencePython
	Py_INCREF(hostRef);
	registerRef(obj, hostRef, &releasePython);
}

void JPReferenceQueue::registerRef(jobject obj, void* host, JCleanupHook func)
{
	JP_TRACE_IN("JPReferenceQueue::registerRef");
	JPJavaFrame frame(m_Context);

	// create the ref ...
	jvalue args[3];
	args[0].l = obj;
	args[1].j = (jlong) host;
	args[2].j = (jlong) func;

	JP_TRACE("Register reference");
	frame.CallVoidMethodA(m_ReferenceQueue.get(), m_ReferenceQueueRegisterMethod, args);
	JP_TRACE_OUT;
}
