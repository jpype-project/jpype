// --- file: common/jp_proxy.cpp ---
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


// Consider using a Multirelease Jar to skip this starting in 16
extern "C" JNIEXPORT jobject JNICALL Java_org_jpype_proxy_JPypeProxyType_getDefaultHandle(
    JNIEnv *env, jclass clazz, jclass iface, jobject method, jclass mhClass)
{
    JPJavaFrame frame = JPJavaFrame::external(env);
    try {
        jmethodID lookupMethod = env->GetStaticMethodID(mhClass, "lookup", 
            "()Ljava/lang/invoke/MethodHandles$Lookup;");

#if 1
		// Until we drop Java 11 we are stuck with this extreme unfriendly code
		jclass lookupClass = env->FindClass("java/lang/invoke/MethodHandles$Lookup");
		jfieldID implLookupField = env->GetStaticFieldID(lookupClass, "IMPL_LOOKUP", "Ljava/lang/invoke/MethodHandles$Lookup;");
		jobject trustedLookup = env->GetStaticObjectField(lookupClass, implLookupField);
		jmethodID unreflectMethod = env->GetMethodID(lookupClass, "unreflectSpecial", 
			"(Ljava/lang/reflect/Method;Ljava/lang/Class;)Ljava/lang/invoke/MethodHandle;");
		jobject methodHandle = env->CallObjectMethod(trustedLookup, unreflectMethod, method, iface);
#else
		// Until we drop Java 15 we are stuck with this backdoor (after 15 we can use the safe method in Java)
        jobject lookupObj = env->CallStaticObjectMethod(mhClass, lookupMethod);
        jclass lookupClass = env->GetObjectClass(lookupObj);
        jmethodID unreflectMethod = env->GetMethodID(lookupClass, "unreflectSpecial", 
            "(Ljava/lang/reflect/Method;Ljava/lang/Class;)Ljava/lang/invoke/MethodHandle;");
        jobject methodHandle = env->CallObjectMethod(lookupObj, unreflectMethod, method, iface);
#endif

        if (env->ExceptionCheck())
        {
            env->ExceptionDescribe(); // Useful for debugging the "Pile of Pain"
            return nullptr;
        }

        return methodHandle; 
    }
    catch (JPypeException& ex) {
        // Fallback for safety
        return nullptr;
    }
}

extern "C" JNIEXPORT jobject JNICALL Java_org_jpype_proxy_JPypeProxyInstance_hostInvoke(
		JNIEnv *env, jclass clazz,
		jstring name,
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
printf("return class %p %ld\n", returnClass, returnTypePtr);
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
printf("return class %p\n", returnClass);
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

    // 1. Pack interfaces into a Class[] array
    jobjectArray ar = frame.NewObjectArray((int) intf.size(),
            JPContext_global->_java_lang_Class->getJavaClass(), nullptr);
    for (unsigned int i = 0; i < intf.size(); i++)
        frame.SetObjectArrayElement(ar, i, intf[i]->getJavaClass());

    // 2. Get the ProxyInstance from the Factory (handles deduplication/caching)
    jvalue v_factory[2];
    v_factory[0].j = (jlong) &JPProxy::releaseProxyPython; // Cleanup pointer
    v_factory[1].l = ar;

	// Corrected JNI call for the static method
	jobject proxyType = frame.CallStaticObjectMethodA(
		JPContext_global->m_ProxyFactoryClass.get(),
		JPContext_global->m_ProxyFactory_getProxyTypeID, 
		v_factory);

    // 3. Store the Type reference for later instance creation
    m_ProxyType = JPObjectRef(proxyType); 
    m_Ref = nullptr;
    JP_TRACE_OUT;
}

JPProxy::~JPProxy()
{
	try
	{
		if (m_Ref != nullptr && JPContext_global->isRunning())
			JPContext_global->getEnv()->DeleteWeakGlobalRef(m_Ref);
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

	// We reuse the reference but to avoid loops we use weak referencing
    jobject instance = nullptr;
    if (m_Ref != nullptr)
        instance = frame.NewLocalRef(m_Ref);

    if (instance == nullptr)
    {
        JP_TRACE("Create new instance from Type");
        Py_INCREF(m_Instance);
        jvalue v[1];
        v[0].j = (jlong) this;
        instance = frame.CallObjectMethodA(m_ProxyType.get(),
                JPContext_global->m_ProxyType_newInstanceID, v);
        m_Ref = frame.NewWeakGlobalRef(instance);
    }
    
    jvalue out;
    out.l = frame.keep(instance);
    return out;
    JP_TRACE_OUT;
}

JPProxyInstance::JPProxyInstance(JPJavaFrame& frame,
        jclass clss,
        const string& name,
        JPClass* super,
        JPClassList& interfaces,
        jint modifiers)
: JPClass(frame, clss, name, super, interfaces, modifiers)
{
	m_GetInstanceID = frame.GetStaticMethodID(clss, "getInstance", "(Ljava/lang/Object;)J");
}

JPProxyInstance::~JPProxyInstance() = default;

JPPyObject JPProxyInstance::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast)
{
	JP_TRACE_IN("JPProxyInstance::convertToPythonObject");
	jlong hostPtr = frame.CallStaticLongMethodA(getJavaClass(), m_GetInstanceID, &val);
	if (hostPtr != 0)
	{
		JPProxy *proxy = (JPProxy*) hostPtr;
		PyJPProxy *pproxy = proxy->m_Instance;

		// Standard JPype return logic
		if (pproxy->m_Convert && pproxy->m_Target != Py_None)
			return JPPyObject::use(pproxy->m_Target);

		return JPPyObject::use((PyObject*) pproxy);
	}

	// Fallback if it's a different kind of proxy or null
	return JPClass::convertToPythonObject(frame, val, cast);
	JP_TRACE_OUT;
}


JPProxyDirect::JPProxyDirect(PyJPProxy* inst, JPClassList& intf)
: JPProxy(inst, intf)
{
}

JPProxyDirect::~JPProxyDirect() = default;

JPPyObject JPProxyDirect::getCallable(const string& cname, int& addSelf)
{
	return JPPyObject::accept(PyObject_GetAttrString((PyObject*) m_Instance, cname.c_str()));
}

JPProxyIndirect::JPProxyIndirect(PyJPProxy* inst, JPClassList& intf)
: JPProxy(inst, intf)
{
}

JPProxyIndirect::~JPProxyIndirect() = default;

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
