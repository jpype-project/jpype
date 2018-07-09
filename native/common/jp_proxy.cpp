/*****************************************************************************
   Copyright 2004 Steve Ménard

   Licensed under the Apache License, Version 2.0 (the "License");
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
{ // impl detail, gets initialized by JPProxy::init()
	jclass handlerClass;
	jmethodID invocationHandlerConstructorID;
	jfieldID hostObjectID;
}

JNIEXPORT jobject JNICALL Java_jpype_JPypeInvocationHandler_hostInvoke(
		JNIEnv *env, jclass clazz, jstring name,
		jlong hostObj,
		jobjectArray args,
		jobjectArray types,
		jclass returnType)
{
	JP_TRACE_IN("Java_jpype_JPypeInvocationHandler_hostInvoke");

	JPJavaFrame frame(env);
	JPPyCallAcquire callback;

	try
	{
		string cname = JPJni::toStringUTF8(name);

		// Get the callable object
		JPPyObject callable(JPPythonEnv::getJavaProxyCallable((PyObject*) hostObj, cname));

		// If method can't be called, throw an exception
		if (callable.isNull() || callable.isNone())
		{
			frame.ThrowNew(JPJni::s_NoSuchMethodErrorClass, cname.c_str());
			return NULL;
		}

		// convert the arguments into a python list
		jsize argLen = frame.GetArrayLength(types);
		vector<JPPyObject> hostArgs;
		JPPyTuple pyargs(JPPyTuple::newTuple(argLen));
		for (int i = 0; i < argLen; i++)
		{
			jclass c = (jclass) frame.GetObjectArrayElement(types, i);
			JPClass* type = JPTypeManager::findClass(c);
			JPValue val = type->getValueFromObject(frame.GetObjectArrayElement(args, i));
			pyargs.setItem(i, type->convertToPythonObject(val).get());
		}

		JPPyObject returnValue(callable.call(pyargs.get(), NULL));
		JPClass* returnClass = JPTypeManager::findClass(returnType);

		if (returnValue.isNull() || returnValue.isNone())
		{
			if (returnClass != JPTypeManager::_void) // && returnT.getType() < JPTypeName::_object)
			{
				frame.ThrowNew(JPJni::s_RuntimeExceptionClass, "Return value is None when it cannot be");
				return NULL;
			}
		}

		if (returnClass == JPTypeManager::_void)
		{
			return NULL;
		}

		if (returnClass->canConvertToJava(returnValue.get()) == JPMatch::_none)
		{
			frame.ThrowNew(JPJni::s_RuntimeExceptionClass, "Return value is not compatible with required type.");
			return NULL;
		}

		// We must box here.
		if (dynamic_cast<JPPrimitiveType*> (returnClass) == returnClass)
		{
			returnClass = ((JPPrimitiveType*) returnClass)->getBoxedClass();
		}

		jvalue res = returnClass->convertToJava(returnValue.get());
		return frame.keep(res.l);
	} catch (JPypeException& ex)
	{
		ex.toJava();
	} catch (...)
	{
		frame.ThrowNew(JPJni::s_RuntimeExceptionClass, "unknown error occurred");
	}

	return NULL;

	JP_TRACE_OUT;
}

void JPProxy::init()
{
	JPJavaFrame frame(32);
	JP_TRACE_IN("JPProxy::init");

	// build the proxy class ...
	jobject cl = JPJni::getSystemClassLoader();

	jclass handler = frame.DefineClass("jpype/JPypeInvocationHandler", cl,
			JPThunk::_jpype_JPypeInvocationHandler,
			JPThunk::_jpype_JPypeInvocationHandler_size);
	handlerClass = (jclass) frame.NewGlobalRef(handler);

	JNINativeMethod method[1];
	method[0].name = (char*) "hostInvoke";
	method[0].signature = (char*) "(Ljava/lang/String;J[Ljava/lang/Object;[Ljava/lang/Class;Ljava/lang/Class;)Ljava/lang/Object;";
	method[0].fnPtr = (void*) &Java_jpype_JPypeInvocationHandler_hostInvoke;


	hostObjectID = frame.GetFieldID(handler, "hostObject", "J");
	invocationHandlerConstructorID = frame.GetMethodID(handler, "<init>", "()V");

	frame.RegisterNatives(handlerClass, method, 1);

	JP_TRACE_OUT;
}

JPProxy::JPProxy(PyObject* inst, JPClass::ClassList& intf)
: m_Instance(inst), m_InterfaceClasses(intf)
{
	JP_TRACE_IN("JPProxy::JPProxy");
	JPJavaFrame frame;

	jobjectArray ar = frame.NewObjectArray((int) intf.size(), JPJni::s_ClassClass, NULL);

	for (unsigned int i = 0; i < intf.size(); i++)
	{
		frame.SetObjectArrayElement(ar, i, intf[i]->getJavaClass());
	}
	m_Interfaces = ar;
	JP_TRACE_OUT;
}

JPProxy::~JPProxy()
{
}

jobject JPProxy::getProxy()
{
	JPJavaFrame frame;
	jobject cl = JPJni::getSystemClassLoader();

	jobject m_Handler = frame.NewObject(handlerClass, invocationHandlerConstructorID);
	frame.SetLongField(m_Handler, hostObjectID, (jlong) m_Instance);

	jvalue v[3];
	v[0].l = cl;
	v[1].l = m_Interfaces.get();
	v[2].l = m_Handler;

	jobject proxy = frame.CallStaticObjectMethodA(JPJni::s_ProxyClass, JPJni::s_NewProxyInstanceID, v);
	JPReferenceQueue::registerRef(proxy, this->m_Instance);
	return frame.keep(proxy);
}

