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
#include <pythread.h>

#include "jp_primitive_common.h"

PyThread_type_lock lock;

class JPThreadLock
{
public:

	JPThreadLock()
	{
		JP_TRACE_LOCKS("Thread attempt lock", this);
		if (!PyThread_acquire_lock(lock, NOWAIT_LOCK))
		{
			JP_TRACE_LOCKS("Thread wait lock", this);
			Py_BEGIN_ALLOW_THREADS
			PyThread_acquire_lock(lock, WAIT_LOCK);
			Py_END_ALLOW_THREADS
		}
		JP_TRACE_LOCKS("Thread locked", this);
	}

	~JPThreadLock()
	{
		JP_TRACE_LOCKS("Thread release", this);
		PyThread_release_lock(lock);
	}

};

JPPyTuple getArgs(JPContext* context, jlongArray parameterTypePtrs,
		  jobjectArray args)
{
	JPJavaFrame frame(context);
	jsize argLen = frame.GetArrayLength(parameterTypePtrs);
	JPPyTuple pyargs(JPPyTuple::newTuple(argLen));
	JPPrimitiveArrayAccessor<jlongArray, jlong*> accessor(frame, parameterTypePtrs,
							&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);

	jlong* types = accessor.get();
	for (jsize i = 0; i < argLen; i++)
	{
		JPClass* type = (JPClass*) types[i];
		JPValue val = type->getValueFromObject(frame.GetObjectArrayElement(args, i));
		pyargs.setItem(i, type->convertToPythonObject(val).get());
	}
	return pyargs;
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

	// We need the resources to be held for the full duration of the proxy.
	JPPyCallAcquire callback;

	// Python has a bug that is corrupting the stack if a
	// second thread passes this point.  I don't particularly
	// like this solution as it prevents a Proxy from calling
	// code that itself issues a proxy.  But after 2 weeks of debugging,
	// trying random formulations, stripping it down, trying again...
	// well this is the only thing that works. Perhaps there is some
	// other bug that once fixed will correct this problem.
	JPThreadLock guard;
	{
		JP_TRACE_IN_C("JPype_InvocationHandler_hostInvoke");
		try
		{
			if (hostObj == 0)
			{
				env->functions->ThrowNew(env, context->_java_lang_RuntimeException.get(),
							"host reference is null");
				return NULL;
			}

			string cname = context->toStringUTF8(name);
			JP_TRACE("Get callable for", cname);

			// Get the callable object
			JPPyObject callable(JPPythonEnv::getJavaProxyCallable((PyObject*) hostObj, cname));
			PyErr_Clear();

			// If method can't be called, throw an exception
			if (callable.isNull() || callable.isNone())
			{
				JP_TRACE("Callable not found");
				JP_RAISE_METHOD_NOT_FOUND(cname);
				return NULL;
			}

			// Find the return type
			JP_TRACE("Get return type");
			JPClass* returnClass = (JPClass*) returnTypePtr;

			// convert the arguments into a python list
			JP_TRACE("Convert arguments");
			JPPyTuple pyargs(getArgs(context, parameterTypePtrs, args));

			JP_TRACE("Call Python");
			JPPyObject returnValue(callable.call(pyargs.get(), NULL));

			JP_TRACE("Handle return");
			if (returnClass == context->_void)
			{
				JP_TRACE("Void return");
				return NULL;
			}

			// This should not be able to happen.  The call checks
			// for an error and throws, thus this condition cannot be hit.
			if (returnValue.isNull())
			{
				JP_TRACE("Null return");
				JP_RAISE_TYPE_ERROR("Return value is None when it cannot be");
			}

			if (returnClass->canConvertToJava(returnValue.get()) == JPMatch::_none)
			{
				JP_TRACE("Cannot convert");
				JP_RAISE_TYPE_ERROR("Return value is not compatible with required type.");
			}

			// We must box here.
			if (returnClass->isPrimitive())
			{
				JP_TRACE("Box return");
				returnClass = ((JPPrimitiveType*) returnClass)->getBoxedClass();
			}

			JP_TRACE("Convert return");
			jvalue res = returnClass->convertToJava(returnValue.get());
			return res.l;
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
		JP_TRACE_OUT_C;
	}
}

JPProxyFactory::JPProxyFactory(JPContext* context)
{
	m_Context = context;
	JPJavaFrame frame(m_Context);
	JP_TRACE_IN("JPProxy::init");
	lock = PyThread_allocate_lock();

	jclass proxyClass = context->getClassLoader()->findClass("org.jpype.proxy.JPypeProxy");

	JNINativeMethod method[1];
	method[0].name = (char*) "hostInvoke";
	method[0].signature = (char*) "(JLjava/lang/String;JJ[J[Ljava/lang/Object;)Ljava/lang/Object;";
	method[0].fnPtr = (void*) &JPype_InvocationHandler_hostInvoke;
	frame.GetMethodID(proxyClass, "<init>", "()V");
	frame.RegisterNatives(proxyClass, method, 1);

	m_ProxyClass = JPClassRef(context, proxyClass);
	m_NewProxyID = frame.GetStaticMethodID(m_ProxyClass.get(),
					"newProxy",
					"(Lorg/jpype/JPypeContext;J[Ljava/lang/Class;)Lorg/jpype/proxy/JPypeProxy;");
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
	jvalue v[3];
	v[0].l = m_Factory->m_Context->getJavaContext();
	v[1].j = (jlong) inst;
	v[2].l = ar;

	// Create the proxy
	jobject proxy = frame.CallStaticObjectMethodA(m_Factory->m_ProxyClass.get(),
						m_Factory->m_NewProxyID, v);
	m_Proxy = JPObjectRef(m_Factory->m_Context, proxy);
	JP_TRACE_OUT;
}

JPProxy::~JPProxy()
{
}

jobject JPProxy::getProxy()
{
	JP_TRACE_IN("JPProxy::getProxy");
	JPContext* context = getContext();
	JPJavaFrame frame(context);

	// Use the proxy to make an instance
	JP_TRACE("Create handler");
	Py_INCREF(m_Instance);
	jobject instance = frame.CallObjectMethodA(m_Proxy.get(),
						m_Factory->m_NewInstanceID, 0);

	return frame.keep(instance);
	JP_TRACE_OUT;
}

JPProxyType::JPProxyType() : JPClass(handlerClass)
{
}

JPProxyType::~JPProxyType()
{
}

JPPyObject JPProxyType::convertToPythonObject(jvalue val)
{
	JPJavaFrame frame;
	jobject ih = frame.CallStaticObjectMethodA(proxyClass, getInvocationHandlerID, &val);
	PyJPProxy* proxy = (PyJPProxy*) frame.GetLongField(ih, hostObjectID);
	return JPPyObject(JPPyRef::_use, proxy->m_Target);
}

