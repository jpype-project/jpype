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
#include "jpype.h"
#include "pyjp.h"
#include "jp_proxy.h"
#include "jp_classloader.h"
#include "jp_reference_queue.h"
#include "jp_primitive_accessor.h"
#include "jp_boxedtype.h"
#include "jp_functional.h"

JPPyObject getArgs(jlongArray parameterTypePtrs,
		jobjectArray args, PyObject* self, int addSelf)
{
	JP_TRACE_IN("JProxy::getArgs");
	JPJavaFrame frame = JPJavaFrame::outer();
	jsize argLen = frame.GetArrayLength(parameterTypePtrs);
	jsize extra = addSelf?1:0;
	JPPyObject pyargs = JPPyObject::call(PyTuple_New(argLen+extra));
	JPPrimitiveArrayAccessor<jlongArray, jlong*> accessor(frame, parameterTypePtrs,
			&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);

	if (addSelf)
	{
		Py_IncRef(self);
		PyTuple_SetItem(pyargs.get(), 0, self);
	}
	jlong* types = accessor.get();
	for (jsize i = 0; i < argLen; i++)
	{
		jobject obj = frame.GetObjectArrayElement(args, i);
		JPClass* type = frame.findClassForObject(obj);
		if (type == nullptr)
			type = reinterpret_cast<JPClass*> (types[i]);
		JPValue val = type->getValueFromObject(frame, JPValue(type, obj));
		PyTuple_SetItem(pyargs.get(), i+extra, type->convertToPythonObject(frame, val, false).keep());
	}
	return pyargs;
	JP_TRACE_OUT;
}

extern "C" JNIEXPORT jobject JNICALL Java_org_jpype_proxy_JPypeProxy_hostInvoke(
		JNIEnv *env, jclass clazz,
		jlong contextPtr, jstring name,
		jlong hostObj,
		jlong returnTypePtr,
		jlongArray parameterTypePtrs,
		jobjectArray args,
		jobject missing)
{
	JPJavaFrame frame = JPJavaFrame::external(env);

	// We need the resources to be held for the full duration of the proxy.
	JPPyCallAcquire callback;
	try
	{
		JP_TRACE_IN("JPype_InvocationHandler_hostInvoke");
		JP_TRACE("hostObj", (void*) hostObj);
		try
		{
			// GCOVR_EXCL_START
			// Sanity check, should never be hit
			if (hostObj == 0)
			{
				env->functions->ThrowNew(env, JPContext_global->m_RuntimeException.get(),
						"host reference is null");
				return nullptr;
			}
			// GCOVR_EXCL_STOP

			string cname = frame.toStringUTF8(name);
			JP_TRACE("Get callable for", cname);

			// Get the callable object
			int addSelf = 0;
			JPProxy* proxy = (JPProxy*) hostObj;
			JPPyObject callable(proxy->getCallable(cname, addSelf));

			// If method can't be called, throw an exception
			if (callable.isNull() || callable.get() == Py_None)
                return missing;

			// Find the return type
			auto* returnClass = (JPClass*) returnTypePtr;
			JP_TRACE("Get return type", returnClass->getCanonicalName());

			// convert the arguments into a python list
			JP_TRACE("Convert arguments");
			JPPyObject pyargs = getArgs(parameterTypePtrs, args, proxy->m_Instance->m_Target, addSelf);

			JP_TRACE("Call Python");
			JPPyObject returnValue = JPPyObject::call(PyObject_Call(callable.get(), pyargs.get(), nullptr));

			JP_TRACE("Handle return", Py_TYPE(returnValue.get())->tp_name);
			if (returnClass == JPContext_global->_void)
			{
				JP_TRACE("Void return");
				return nullptr;
			}

			// This is a SystemError where the caller return null without
			// setting a Python error.
			if (returnValue.isNull())
			{
				JP_TRACE("Null return");
				JP_RAISE(PyExc_TypeError, "Return value is null when it cannot be");
			}

			// We must box here.
			JPMatch returnMatch(&frame, returnValue.get());
			if (returnClass->isPrimitive())
			{
				JP_TRACE("Box return");
				if (returnClass->findJavaConversion(returnMatch) == JPMatch::_none)
					JP_RAISE(PyExc_TypeError, "Return value is not compatible with required type.");
				jvalue res = returnMatch.convert();
				auto *boxed =  dynamic_cast<JPBoxedType *>( (dynamic_cast<JPPrimitiveType*>( returnClass))->getBoxedClass(frame));
				jvalue res2;
				res2.l = boxed->box(frame, res);
				return frame.keep(res2.l);
			}

			if (returnClass->findJavaConversion(returnMatch) == JPMatch::_none)
			{
				JP_TRACE("Cannot convert");
				JP_RAISE(PyExc_TypeError, "Return value is not compatible with required type.");
			}

			JP_TRACE("Convert return to", returnClass->getCanonicalName());
			jvalue res = returnMatch.convert();
			return frame.keep(res.l);
		} catch (JPypeException& ex)
		{
			JP_TRACE("JPypeException raised");
			ex.toJava();
		} catch (...)  // GCOVR_EXCL_LINE
		{
			JP_TRACE("Other Exception raised");
			env->functions->ThrowNew(env, JPContext_global->m_RuntimeException.get(),
					"unknown error occurred");
		}
		return nullptr;
		JP_TRACE_OUT;  // GCOVR_EXCL_LINE
	}
	catch (...) // JP_TRACE_OUT implies a throw but that is not allowed. 
	{}
	return NULL;
}

JPProxy::JPProxy(PyJPProxy* inst, JPClassList& intf)
: m_Instance(inst), m_InterfaceClasses(intf)
{
	JP_TRACE_IN("JPProxy::JPProxy");
	JPJavaFrame frame = JPJavaFrame::outer();

	// Convert the interfaces to a Class[]
	jobjectArray ar = frame.NewObjectArray((int) intf.size(),
			JPContext_global->_java_lang_Class->getJavaClass(), nullptr);
	for (unsigned int i = 0; i < intf.size(); i++)
	{
		frame.SetObjectArrayElement(ar, i, intf[i]->getJavaClass());
	}
	jvalue v[4];
	v[0].l = JPContext_global->getJavaContext();
	v[1].j = (jlong) this;
	v[2].j = (jlong) & JPProxy::releaseProxyPython;
	v[3].l = ar;

	// Create the proxy
	jobject proxy = frame.CallStaticObjectMethodA(JPContext_global->m_ProxyClass.get(),
			JPContext_global->m_Proxy_NewID, v);
	m_Proxy = JPObjectRef(proxy);
	m_Ref = nullptr;
	JP_TRACE_OUT;
}

JPProxy::~JPProxy()
{
	try
	{
		if (m_Ref != nullptr && JPContext_global->isRunning())
		{
			JPContext_global->getEnv()->DeleteWeakGlobalRef(m_Ref);
		}
	} catch (JPypeException &ex)  // GCOVR_EXCL_LINE
	{
		// Cannot throw
	}
}

void JPProxy::releaseProxyPython(void* host)
{
	Py_XDECREF(((JPProxy*) host)->m_Instance);
}

jvalue JPProxy::getProxy()
{
	JP_TRACE_IN("JPProxy::getProxy");
	JPJavaFrame frame = JPJavaFrame::inner();

	jobject instance = nullptr;
	if (m_Ref != nullptr)
	{
		instance = frame.NewLocalRef(m_Ref);
	}

	if (instance == nullptr)
	{
		// Use the proxy to make an instance
		JP_TRACE("Create handler");
		Py_INCREF(m_Instance);
		instance = frame.CallObjectMethodA(m_Proxy.get(),
				JPContext_global->m_Proxy_NewInstanceID, nullptr);
		m_Ref = frame.NewWeakGlobalRef(instance);
	}
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
= default;

JPPyObject JPProxyType::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast)
{
	JP_TRACE_IN("JPProxyType::convertToPythonObject");
	jobject ih = frame.CallStaticObjectMethodA(m_ProxyClass.get(),
			m_GetInvocationHandlerID, &val);

	JPProxy *proxy = (JPProxy*) frame.GetLongField(ih, m_InstanceID);
	PyJPProxy *pproxy = proxy->m_Instance;

	// Is it a native Python object
	if (pproxy->m_Convert && pproxy->m_Target != Py_None)
		return JPPyObject::use(pproxy->m_Target);

	// Is it a user extended class
	if (pproxy->m_Dispatch == Py_None)
		return JPPyObject::use((PyObject*) pproxy);

	// Return the Proxy itself
	JP_TRACE("Target", pproxy);
	return JPPyObject::use((PyObject*) pproxy);
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}

JPProxyDirect::JPProxyDirect(PyJPProxy* inst, JPClassList& intf)
: JPProxy(inst, intf)
{
}

JPProxyDirect::~JPProxyDirect()
= default;

JPPyObject JPProxyDirect::getCallable(const string& cname, int& addSelf)
{
	return JPPyObject::accept(PyObject_GetAttrString((PyObject*) m_Instance, cname.c_str()));
}

JPProxyIndirect::JPProxyIndirect(PyJPProxy* inst, JPClassList& intf)
: JPProxy(inst, intf)
{
}

JPProxyIndirect::~JPProxyIndirect()
= default;

JPPyObject JPProxyIndirect::getCallable(const string& cname, int& addSelf)
{
	JPPyObject out = JPPyObject::accept(PyObject_GetAttrString(m_Instance->m_Dispatch, cname.c_str()));
	if (!out.isNull())
	{
		addSelf = (m_Instance->m_Dispatch != m_Instance->m_Target) && (m_Instance->m_Target != Py_None);
		return out;
	}
	return JPPyObject::accept(PyObject_GetAttrString((PyObject*) m_Instance, cname.c_str()));
}

JPProxyFunctional::JPProxyFunctional(PyJPProxy* inst, JPClassList& intf)
: JPProxy(inst, intf)
{
	m_Functional = dynamic_cast<JPFunctional*>( intf[0]);
}

JPProxyFunctional::~JPProxyFunctional()
= default;

JPPyObject JPProxyFunctional::getCallable(const string& cname, int& addSelf)
{
	if (cname == m_Functional->getMethod())
		return JPPyObject::accept(PyObject_GetAttrString(m_Instance->m_Dispatch, "__call__"));
	return JPPyObject::accept(PyObject_GetAttrString((PyObject*) m_Instance, cname.c_str()));
}
