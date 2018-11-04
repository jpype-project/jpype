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
#include <jpype.h>
#include <jp_thunk.h>

namespace
{ // impl detail
	jmethodID s_ReferenceQueueRegisterMethod;
	jmethodID s_ReferenceQueueStartMethod;
	jmethodID s_ReferenceQueueStopMethod;

	jobject s_ReferenceQueue;
}

JNIEXPORT void JNICALL Java_jpype_ref_JPypeReferenceQueue_removeHostReference(
		JNIEnv *env, jclass clazz, jlong hostObj)
{
	JPJavaFrame frame;
	JP_TRACE_IN("Java_jpype_ref_JPypeReferenceQueue_removeHostReference");

	JPPyCallAcquire callback;
	if (hostObj > 0)
	{
		PyObject* hostObjRef = (PyObject*) hostObj;
		Py_DECREF(hostObjRef);
	}

	//return NULL;
	JP_TRACE_OUT;
}

void JPReferenceQueue::init()
{
	JPJavaFrame frame(32);
	JP_TRACE_IN("JPReferenceQueue::init");

	// build the ReferenceQueue class ...
	jclass cls = JPClassLoader::findClass("org.jpype.ref.JPypeReferenceQueue");

	//Required due to bug in jvm
	//See: http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6493522
	jmethodID ctor = frame.GetMethodID(cls, "<init>", "()V");

	JNINativeMethod method2[1];
	method2[0].name = (char*) "removeHostReference";
	method2[0].signature = (char*) "(J)V";
	method2[0].fnPtr = (void*) &Java_jpype_ref_JPypeReferenceQueue_removeHostReference;

	frame.RegisterNatives(cls, method2, 1);

	jmethodID getInstanceID = frame.GetStaticMethodID(cls, "getInstance", "()Lorg/jpype/ref/JPypeReferenceQueue;");
	s_ReferenceQueue = frame.NewGlobalRef(frame.CallStaticObjectMethod(cls, getInstanceID));

	// Get all required methods
	s_ReferenceQueueRegisterMethod = frame.GetMethodID(cls, "registerRef", "(Ljava/lang/Object;J)V");
	s_ReferenceQueueStartMethod = frame.GetMethodID(cls, "start", "()V");
	s_ReferenceQueueStopMethod = frame.GetMethodID(cls, "stop", "()V");

	JP_TRACE_OUT;
}


// FIXME move the loader for the custom class from jp_proxy.cpp

void JPReferenceQueue::startJPypeReferenceQueue(bool useJavaThread)
{
	JP_TRACE_IN("JPReferenceQueue::startJPypeReferenceQueue");
	JPJavaFrame frame;
	frame.CallVoidMethod(s_ReferenceQueue, s_ReferenceQueueStartMethod);
	JP_TRACE_OUT;
}

void JPReferenceQueue::shutdown()
{
	JP_TRACE_IN("JPReferenceQueue::shutdown");
	JPJavaFrame frame;
	frame.CallVoidMethod(s_ReferenceQueue, s_ReferenceQueueStopMethod);
	JP_TRACE_OUT;
}

void JPReferenceQueue::registerRef(jobject obj, PyObject* hostRef)
{
	JP_TRACE_IN("JPReferenceQueue::registerRef");
	JPJavaFrame frame;

	// create the ref ...
	jvalue args[2];
	args[0].l = obj;
	args[1].j = (jlong) hostRef;

	// MATCH TO DECREF IN Java_jpype_ref_JPypeReferenceQueue_removeHostReference
	Py_INCREF(hostRef);

	JP_TRACE("Register reference");
	frame.CallVoidMethodA(s_ReferenceQueue, s_ReferenceQueueRegisterMethod, args);
	JP_TRACE_OUT;
}

void JPReferenceQueue::registerRef(PyObject* ref, PyObject* targetRef)
{
	JP_TRACE_IN("JPReferenceQueue::registerRef");
	JPJavaFrame frame;
	JPValue* objRef = JPPythonEnv::getJavaValue(ref);
	jobject srcObject = objRef->getJavaObject();
	registerRef(srcObject, targetRef);
	JP_TRACE_OUT;
}
