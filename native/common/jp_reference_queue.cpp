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
	jclass s_ReferenceClass;
	jmethodID s_ReferenceConstructorMethod;
	jclass s_ReferenceQueueClass;
	jmethodID s_ReferenceQueueConstructorMethod;
	jmethodID s_ReferenceQueueRegisterMethod;
	jmethodID s_ReferenceQueueStartMethod;
	jmethodID s_ReferenceQueueRunMethod;
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

	// build the proxy class ...
	jobject cl = JPJni::getSystemClassLoader();
	s_ReferenceClass = (jclass) frame.NewGlobalRef(frame.DefineClass("jpype/ref/JPypeReference", cl,
			JPThunk::_jpype_ref_JPypeReference,
			JPThunk::_jpype_ref_JPypeReference_size));
	s_ReferenceQueueClass = (jclass) frame.NewGlobalRef(frame.DefineClass("jpype/ref/JPypeReferenceQueue", cl,
			JPThunk::_jpype_ref_JPypeReferenceQueue,
			JPThunk::_jpype_ref_JPypeReferenceQueue_size));

	//Required due to bug in jvm
	//See: http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6493522
	s_ReferenceQueueConstructorMethod = frame.GetMethodID(s_ReferenceQueueClass, "<init>", "()V");

	JNINativeMethod method2[1];
	method2[0].name = (char*) "removeHostReference";
	method2[0].signature = (char*) "(J)V";
	method2[0].fnPtr = (void*) &Java_jpype_ref_JPypeReferenceQueue_removeHostReference;

	frame.RegisterNatives(s_ReferenceQueueClass, method2, 1);

	// Get all required methods
	s_ReferenceQueueRegisterMethod = frame.GetMethodID(s_ReferenceQueueClass, "registerRef", "(Ljpype/ref/JPypeReference;J)V");
	s_ReferenceQueueStartMethod = frame.GetMethodID(s_ReferenceQueueClass, "startManaging", "()V");
	s_ReferenceQueueRunMethod = frame.GetMethodID(s_ReferenceQueueClass, "run", "()V");
	s_ReferenceQueueStopMethod = frame.GetMethodID(s_ReferenceQueueClass, "stop", "()V");
	s_ReferenceConstructorMethod = frame.GetMethodID(s_ReferenceClass, "<init>", "(Ljava/lang/Object;Ljava/lang/ref/ReferenceQueue;)V");

	JP_TRACE_OUT;
}


// FIXME move the loader for the custom class from jp_proxy.cpp

void JPReferenceQueue::startJPypeReferenceQueue(bool useJavaThread)
{
	JP_TRACE_IN("JPReferenceQueue::startJPypeReferenceQueue");
	JPJavaFrame frame;
	s_ReferenceQueue = frame.NewGlobalRef(frame.NewObject(s_ReferenceQueueClass, s_ReferenceQueueConstructorMethod));
	if (useJavaThread)
	{
		frame.CallVoidMethod(s_ReferenceQueue, s_ReferenceQueueStartMethod);
	}
	else
	{
		frame.CallVoidMethod(s_ReferenceQueue, s_ReferenceQueueRunMethod);
	}
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
	JP_TRACE_IN("JPReferenceQueue::rregisterRef");
	JPJavaFrame frame;

	// create the ref ...
	jvalue args[2];
	args[0].l = obj;
	args[1].l = s_ReferenceQueue;

	JP_TRACE("Create reference obj");
	jobject refObj = frame.NewObjectA(s_ReferenceClass, s_ReferenceConstructorMethod, args);

	args[0].l = refObj;
	args[1].j = (jlong) hostRef;

	// MATCH TO DECREF IN Java_jpype_ref_JPypeReferenceQueue_removeHostReference
	Py_INCREF(hostRef);

	JP_TRACE(s_ReferenceQueue);
	JP_TRACE(s_ReferenceQueueRegisterMethod);
	JP_TRACE(refObj);

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
