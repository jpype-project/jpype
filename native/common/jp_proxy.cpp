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

#include "jp_context.h"

JNIEXPORT jobject JNICALL Java_jpype_JPypeInvocationHandler_hostInvoke(
		JNIEnv *env, jclass clazz, jlong contextPtr, jstring name,
		jlong hostObj,
		jobjectArray args,
		jobjectArray types,
		jclass returnType)
{
	JP_TRACE_IN("Java_jpype_JPypeInvocationHandler_hostInvoke");

	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context);
	JPPyCallAcquire callback;

	try
	{
		string cname = context->toStringUTF8(name);

		// Get the callable object
		JPPyObject callable(JPPythonEnv::getJavaProxyCallable((PyObject*) hostObj, cname));

		// If method can't be called, throw an exception
		if (callable.isNull() || callable.isNone())
		{
			frame.ThrowNew(context->_java_lang_NoSuchMethodError.get(), cname.c_str());
			return NULL;
		}

		// convert the arguments into a python list
		jsize argLen = frame.GetArrayLength(types);
		vector<JPPyObject> hostArgs;
		JPPyTuple pyargs(JPPyTuple::newTuple(argLen));
		for (int i = 0; i < argLen; i++)
		{
			jclass c = (jclass) frame.GetObjectArrayElement(types, i);
			JPClass* type = JPTypeManager::findClassByName(c);
			JPValue val = type->getValueFromObject(frame.GetObjectArrayElement(args, i));
			pyargs.setItem(i, type->convertToPythonObject(val).get());
		}

		JPPyObject returnValue(callable.call(pyargs.get(), NULL));
		JPClass* returnClass = JPTypeManager::findClassByName(returnType);

		if (returnValue.isNull() || returnValue.isNone())
		{
			if (returnClass != context->_void) // && returnT.getType() < JPTypeName::_object)
			{
				frame.ThrowNew(context->_java_lang_RuntimeException.get(), "Return value is None when it cannot be");
				return NULL;
			}
		}

		if (returnClass == context->_void)
		{
			return NULL;
		}

		if (returnClass->canConvertToJava(returnValue.get()) == JPMatch::_none)
		{
			frame.ThrowNew(context->_java_lang_RuntimeException.get(), "Return value is not compatible with required type.");
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
		ex.toJava(context);
	} catch (...)
	{
		frame.ThrowNew(context->_java_lang_RuntimeException.get(), "unknown error occurred");
	}

	return NULL;

	JP_TRACE_OUT;
}

JPProxyFactory::JPProxyFactory(JPContext* context)
{
	JPJavaFrame frame(context);
	JP_TRACE_IN("JPProxy::init");

	jclass handlerClass = JPClassLoader::findClass("org.jpype.proxy.JPypeInvocationHandler");

	JNINativeMethod method[1];
	method[0].name = (char*) "hostInvoke";
	method[0].signature = (char*) "(Ljava/lang/String;J[Ljava/lang/Object;[Ljava/lang/Class;Ljava/lang/Class;)Ljava/lang/Object;";
	method[0].fnPtr = (void*) &Java_jpype_JPypeInvocationHandler_hostInvoke;

	hostObjectID = frame.GetFieldID(handlerClass, "hostObject", "J");
	contextID = frame.GetFieldID(handlerClass, "context", "J");
	invocationHandlerConstructorID = frame.GetMethodID(handlerClass, "<init>", "()V");

	frame.RegisterNatives(handlerClass, method, 1);
	
	m_HandlerClass = JPClassRef(context, handlerClass);
	m_ProxyClass = JPClassRef(context, (jclass) frame.FindClass("java/lang/reflect/Proxy")));
	m_NewProxyInstanceID = frame.GetStaticMethodID(m_ProxyClass.get(), 
			"newProxyInstance",
			"(Ljava/lang/ClassLoader;[Ljava/lang/Class;Ljava/lang/reflect/InvocationHandler;)Ljava/lang/Object;");

	JP_TRACE_OUT;
}

JPProxy::JPProxy(JPContext* context, PyObject* inst, JPClassList& intf)
: m_Instance(inst), m_InterfaceClasses(intf), m_Context(context)
{
	JP_TRACE_IN("JPProxy::JPProxy");
	JPJavaFrame frame(context);

	jobjectArray ar = frame.NewObjectArray((int) intf.size(), m_Context->_java_lang_Class->getJavaClass(), NULL);

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
	JPJavaFrame frame(m_Context);
	jobject cl = m_Context->getClassLoader()->getSystemClassLoader();

	jobject m_Handler = frame.NewObject(handlerClass, invocationHandlerConstructorID);
	frame.SetLongField(m_Handler, hostObjectID, (jlong) m_Instance);
	frame.SetLongField(m_Handler, contextID, (jlong) m_Context);

	jvalue v[3];
	v[0].l = cl;
	v[1].l = m_Interfaces.get();
	v[2].l = m_Handler;

	jobject proxy = frame.CallStaticObjectMethodA(JPJni::s_ProxyClass, JPJni::s_NewProxyInstanceID, v);
	JPReferenceQueue::registerRef(proxy, this->m_Instance);
	return frame.keep(proxy);
}
