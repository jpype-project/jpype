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
#include "jpype.h"
#include "pyjp.h"
#include "jp_classloader.h"
#include "jp_reference_queue.h"
#include "jp_primitive_accessor.h"

extern "C"
{

static void releaseProxyPython(void* host)
{
	Py_XDECREF((PyObject*) host);
}

}

JPPyTuple getArgs(JPContext* context, jlongArray parameterTypePtrs,
		jobjectArray args)
{
	JP_TRACE_IN("JProxy::getArgs");
	JPJavaFrame frame(context);
	jsize argLen = frame.GetArrayLength(parameterTypePtrs);
	JPPyTuple pyargs(JPPyTuple::newTuple(argLen));
	JPPrimitiveArrayAccessor<jlongArray, jlong*> accessor(frame, parameterTypePtrs,
			&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);

	jlong* types = accessor.get();
	for (jsize i = 0; i < argLen; i++)
	{
		JP_TRACE("Convert", i, type->getCanonicalName());
		jobject obj = frame.GetObjectArrayElement(args, i);
		JPClass* type = frame.findClassForObject(obj);
		if (type == NULL)
			type = reinterpret_cast<JPClass*> (accessor.get()[i]);
		JPValue val = type->getValueFromObject(JPValue(type, obj));
		pyargs.setItem(i, type->convertToPythonObject(frame, val).get());
	}
	return pyargs;
	JP_TRACE_OUT;
}

JNIEXPORT jobject JNICALL JPype_InvocationHandler_hostInvoke(
		JNIEnv *env, jclass clazz,
		jlong contextPtr, jstring name,
		jlong hostObj,
		jlong returnTypePtr,
		jlongArray parameterTypePtrs,
		jobjectArray args)
{
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(context, env);

	// We need the resources to be held for the full duration of the proxy.
	JPPyCallAcquire callback;
	{
		JP_TRACE_IN("JPype_InvocationHandler_hostInvoke");
		JP_TRACE("context", context);
		JP_TRACE("hostObj", (void*) hostObj);
		try
		{
			if (hostObj == 0)
			{
				env->functions->ThrowNew(env, context->_java_lang_RuntimeException.get(),
						"host reference is null");
				return NULL;
			}

			string cname = frame.toStringUTF8(name);
			JP_TRACE("Get callable for", cname);

			// Get the callable object
			// FAILS HERE
			JPPyObject callable(PyJPProxy_getCallable((PyObject*) hostObj, cname));
			PyErr_Clear();

			// If method can't be called, throw an exception
			if (callable.isNull() || callable.isNone())
			{
				JP_TRACE("Callable not found");
				JP_RAISE_METHOD_NOT_FOUND(cname);
				return NULL;
			}

			// Find the return type
			JPClass* returnClass = (JPClass*) returnTypePtr;
			JP_TRACE("Get return type", returnClass->getCanonicalName());

			// convert the arguments into a python list
			JP_TRACE("Convert arguments");
			JPPyTuple pyargs(getArgs(context, parameterTypePtrs, args));

			JP_TRACE("Call Python");
			JPPyObject returnValue(JPPyRef::_call, PyObject_Call(callable.get(), pyargs.get(), NULL));

			JP_TRACE("Handle return", Py_TYPE(returnValue.get())->tp_name);
			if (returnClass == context->_void)
			{
				JP_TRACE("Void return");
				return NULL;
			}

			// This is a SystemError where the caller return null without
			// setting a Python error.
			if (returnValue.isNull())
			{
				JP_TRACE("Null return");
				JP_RAISE(PyExc_TypeError, "Return value is null when it cannot be");
			}

			// We must box here.
			if (returnClass->isPrimitive())
			{
				JP_TRACE("Box return");
				returnClass = ((JPPrimitiveType*) returnClass)->getBoxedClass(context);
			}

			JPMatch returnMatch;
			if (returnClass->getJavaConversion(&frame, returnMatch, returnValue.get()) == JPMatch::_none)
			{
				JP_TRACE("Cannot convert");
				JP_RAISE(PyExc_TypeError, "Return value is not compatible with required type.");
			}

			JP_TRACE("Convert return to", returnClass->getCanonicalName());
			jvalue res = returnMatch.conversion->convert(&frame, returnClass, returnValue.get());
			return frame.keep(res.l);
		} catch (JPypeException& ex)
		{
			JP_TRACE("JPypeException raised");
			ex.toJava(context);
		} catch (...)
		{
			JP_TRACE("Other Exception raised");
			env->functions->ThrowNew(env, context->_java_lang_RuntimeException.get(),
					"unknown error occurred");
		}
		return NULL;
		JP_TRACE_OUT;
	}
}

JPProxyFactory::JPProxyFactory(JPJavaFrame& frame)
{
	JP_TRACE_IN("JPProxy::init");
	m_Context = frame.getContext();

	jclass proxyClass = m_Context->getClassLoader()->findClass(frame, "org.jpype.proxy.JPypeProxy");

	JNINativeMethod method[1];
	method[0].name = (char*) "hostInvoke";
	method[0].signature = (char*) "(JLjava/lang/String;JJ[J[Ljava/lang/Object;)Ljava/lang/Object;";
	method[0].fnPtr = (void*) &JPype_InvocationHandler_hostInvoke;
	frame.GetMethodID(proxyClass, "<init>", "()V");
	frame.RegisterNatives(proxyClass, method, 1);

	m_ProxyClass = JPClassRef(frame, proxyClass);
	m_NewProxyID = frame.GetStaticMethodID(m_ProxyClass.get(),
			"newProxy",
			"(Lorg/jpype/JPypeContext;JJ[Ljava/lang/Class;)Lorg/jpype/proxy/JPypeProxy;");
	m_NewInstanceID = frame.GetMethodID(m_ProxyClass.get(),
			"newInstance",
			"()Ljava/lang/Object;");

	JP_TRACE_OUT;
}

JPProxy* JPProxyFactory::newProxy(PyObject* inst, JPClassList& intf)
{
	return new JPProxy(this, inst, intf);
}

JPProxy::JPProxy(JPProxyFactory* factory, PyObject* inst, JPClassList& intf)
: m_Factory(factory), m_Instance(inst), m_InterfaceClasses(intf)
{
	JP_TRACE_IN("JPProxy::JPProxy");
	JP_TRACE("Context", m_Factory->m_Context);
	JPJavaFrame frame(m_Factory->m_Context);

	// Convert the interfaces to a Class[]
	jobjectArray ar = frame.NewObjectArray((int) intf.size(),
			m_Factory->m_Context->_java_lang_Class->getJavaClass(), NULL);
	for (unsigned int i = 0; i < intf.size(); i++)
	{
		frame.SetObjectArrayElement(ar, i, intf[i]->getJavaClass());
	}
	jvalue v[4];
	v[0].l = m_Factory->m_Context->getJavaContext();
	v[1].j = (jlong) inst;
	v[2].j = (jlong) & releaseProxyPython;
	v[3].l = ar;

	// Create the proxy
	jobject proxy = frame.CallStaticObjectMethodA(m_Factory->m_ProxyClass.get(),
			m_Factory->m_NewProxyID, v);
	m_Proxy = JPObjectRef(m_Factory->m_Context, proxy);
	JP_TRACE_OUT;
}

JPProxy::~JPProxy()
{
}

jvalue JPProxy::getProxy()
{
	JP_TRACE_IN("JPProxy::getProxy");
	JPContext* context = getContext();
	JPJavaFrame frame(context);

	// Use the proxy to make an instance
	JP_TRACE("Create handler");
	Py_INCREF(m_Instance);
	jobject instance = frame.CallObjectMethodA(m_Proxy.get(),
			m_Factory->m_NewInstanceID, 0);

	jvalue out;
	out.l = frame.keep(instance);
	return out;
	JP_TRACE_OUT;
}

JPProxyType::JPProxyType(JPJavaFrame& frame,
		jclass clss,
		const string& name,
		JPClass* super,
		JPClassList& interfaces,
		jint modifiers)
: JPClass(frame, clss, name, super, interfaces, modifiers)
{
	jclass proxyClass = frame.FindClass("java/lang/reflect/Proxy");
	m_ProxyClass = JPClassRef(frame, proxyClass);
	m_GetInvocationHandlerID = frame.GetStaticMethodID(proxyClass, "getInvocationHandler",
			"(Ljava/lang/Object;)Ljava/lang/reflect/InvocationHandler;");
	m_InstanceID = frame.GetFieldID(clss, "instance", "J");
}

JPProxyType::~JPProxyType()
{
}

JPPyObject JPProxyType::convertToPythonObject(JPJavaFrame& frame, jvalue val)
{
	JP_TRACE_IN("JPProxyType::convertToPythonObject");
	jobject ih = frame.CallStaticObjectMethodA(m_ProxyClass.get(),
			m_GetInvocationHandlerID, &val);
	PyObject *target = (PyObject*) frame.GetLongField(ih, m_InstanceID);
	JP_TRACE("Target", target);
	return JPPyObject(JPPyRef::_use, target);
	JP_TRACE_OUT;
}
