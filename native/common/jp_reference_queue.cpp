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

JNIEXPORT void JNICALL JPype_ReferenceQueue_removeHostReference(
		JNIEnv *env, jclass clazz, jlong context, jlong hostObj)
{
	JPJavaFrame frame((JPContext*) context, env);
	JP_TRACE_IN("JPype_ReferenceQueue_removeHostReference");

	JPPyCallAcquire callback;
	if (hostObj > 0)
	{
		PyObject* hostObjRef = (PyObject*) hostObj;
		Py_DECREF(hostObjRef);
	}

	JP_TRACE_OUT;
}

JPReferenceQueue::JPReferenceQueue(JPContext* context)
{
	m_Context = context;
	JPJavaFrame frame(context, 32);
	JP_TRACE_IN("JPReferenceQueue::init");

	// build the ReferenceQueue class ...
	jclass cls = context->getClassLoader()->findClass("org.jpype.ref.JPypeReferenceQueue");

	//Required due to bug in jvm
	//See: http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6493522
	frame.GetMethodID(cls, "<init>", "()V");

	JNINativeMethod method2[1];
	method2[0].name = (char*) "removeHostReference";
	method2[0].signature = (char*) "(JJ)V";
	method2[0].fnPtr = (void*) &JPype_ReferenceQueue_removeHostReference;
	frame.RegisterNatives(cls, method2, 1);

	// Get all required methods
	m_ReferenceQueueRegisterMethod = frame.GetMethodID(cls, "registerRef", "(Ljava/lang/Object;J)V");

	JP_TRACE_OUT;
}

void JPReferenceQueue::registerRef(jobject obj, PyObject* hostRef)
{
	JP_TRACE_IN("JPReferenceQueue::registerRef");
	JPJavaFrame frame(m_Context);

	// create the ref ...
	jvalue args[2];
	args[0].l = obj;
	args[1].j = (jlong) hostRef;

	// MATCH TO DECREF IN Java_jpype_ref_JPypeReferenceQueue_removeHostReference
	Py_INCREF(hostRef);

	JP_TRACE("Register reference");
	frame.CallVoidMethodA(m_ReferenceQueue.get(), m_ReferenceQueueRegisterMethod, args);
	JP_TRACE_OUT;
}

void JPReferenceQueue::registerRef(PyObject* ref, PyObject* targetRef)
{
	JP_TRACE_IN("JPReferenceQueue::registerRef");
	JPJavaFrame frame(m_Context);
	JPValue* objRef = JPPythonEnv::getJavaValue(ref);
	jobject srcObject = objRef->getJavaObject();
	registerRef(srcObject, targetRef);
	JP_TRACE_OUT;
}
