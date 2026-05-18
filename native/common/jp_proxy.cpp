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

extern "C" PyObject *Py_JP_CALL;

JPPyObject getArgs(JPJavaFrame& frame, jlongArray parameterTypePtrs,
		jobjectArray args, PyObject* self, int addSelf, jsize argLen)
{
	JP_TRACE_IN("JProxy::getArgs");
	jsize extra = addSelf?1:0;
	JPPyObject pyargs = JPPyObject::call(PyTuple_New(argLen+extra));
	JPPrimitiveArrayAccessor<jlongArray, jlong*> accessor(frame, parameterTypePtrs,
			&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);
	JPContext *context = frame.getContext();

	if (addSelf)
	{
		Py_IncRef(self);
		PyTuple_SetItem(pyargs.get(), 0, self);
	}
	jlong* types = accessor.get();
	for (jsize i = 0; i < argLen; i++)
	{
		jobject obj = frame.GetObjectArrayElement(args, i);
		JPClass* type = reinterpret_cast<JPClass*> (types[i]);
		JPValue val = type->getValueFromObject(frame, JPValue(type, obj));
		// We know the exact type here so we can use cast (except for String which must take slow path)
		PyTuple_SetItem(pyargs.get(), i+extra, type->convertToPythonObject(frame, val, type!=(JPClass*) (context->_java_lang_String)).keep());
		frame.DeleteLocalRef(obj);
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
		jmethodID lookupMethod = env->GetStaticMethodID(mhClass, "lookup", 
			"()Ljava/lang/invoke/MethodHandles$Lookup;");
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
		jlong cname,
		jlong hostObj,
		jlong returnTypePtr,
		jlongArray parameterTypePtrs,
		jobjectArray args,
		jsize sz)
{
	if (JPContext_global == nullptr || !JPContext_global->isRunning())
	{
		jclass ise = env->FindClass("java/lang/IllegalStateException");
		env->ThrowNew(ise, "JPype Proxy invoked after the Python Interpreter was terminated.");
		return nullptr; 
	}

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

			// Get the callable object
			int addSelf = 0;
			JPProxy* proxy = (JPProxy*) hostObj;
			PyObject* pyMethodName = reinterpret_cast<PyObject*>(cname);
			JPPyObject callable(proxy->getCallable(pyMethodName, addSelf));

			// Find the return type
			auto* returnClass = (JPClass*) returnTypePtr;
			JP_TRACE("Get return type", returnClass->getCanonicalName());

#if 0
			// Get the name attribute from the callable
			const char* nameStr = PyUnicode_AsUTF8(pyMethodName);
			if (nameStr) {
				printf("DEBUG: Calling Python method: %s callable=%p %s:%s\n", returnClass->getCanonicalName().c_str(), callable.get(), Py_TYPE(proxy->m_Instance->m_Target)->tp_name, nameStr);
				fflush(stdout);
			}
#endif

			// If method can't be called, throw an exception
			if (callable.isNull() || callable.get() == Py_None)
				return parameterTypePtrs;

			// convert the arguments into a python list
			JP_TRACE("Convert arguments");
			JPPyObject pyargs = getArgs(frame, parameterTypePtrs, args, proxy->m_Instance->m_Target, addSelf, sz);

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

JPProxy::JPProxy(PyJPProxy* inst, JPClassList& intf, bool convert)
: m_Instance(inst), m_InterfaceClasses(intf)
{
	JP_TRACE_IN("JPProxy::JPProxy");
	JPJavaFrame frame = JPJavaFrame::outer();
	inst->m_Convert = convert;

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
}

JPProxyInstance::~JPProxyInstance() = default;
JPProxyDirect::~JPProxyDirect() = default;
JPProxyIndirectAttr::~JPProxyIndirectAttr() = default;
JPProxyIndirectDict::~JPProxyIndirectDict() = default;
JPProxyFunctional::~JPProxyFunctional() = default;

JPPyObject JPProxyDirect::getCallable(PyObject* name, int& addSelf)
{
	return JPPyObject::accept(PyObject_GetAttr((PyObject*) m_Instance, name));
}

JPPyObject JPProxyIndirectAttr::getCallable(PyObject* name, int& addSelf)
{
	JPPyObject out = JPPyObject::accept(PyObject_GetAttr(m_Instance->m_Dispatch, name));
	if (!out.isNull())
	{
		addSelf = (m_Instance->m_Dispatch != m_Instance->m_Target) && (m_Instance->m_Target != Py_None);
		return out;
	}
	return JPPyObject::accept(PyObject_GetAttr((PyObject*) m_Instance, name));
}

JPPyObject JPProxyIndirectDict::getCallable(PyObject* name, int& addSelf)
{
    JPPyObject out = JPPyObject::use(PyDict_GetItem(m_Instance->m_Dispatch, name));
	if (!out.isNull())
    {
        addSelf = (m_Instance->m_Dispatch != m_Instance->m_Target) && (m_Instance->m_Target != Py_None);
        return out;
    }
    return JPPyObject::accept(PyObject_GetAttr((PyObject*) m_Instance, name));
}

JPProxyFunctional::JPProxyFunctional(PyJPProxy* inst, JPClassList& intf)
: JPProxy(inst, intf, true)
{
	m_Functional = dynamic_cast<JPFunctional*>( intf[0]);
}

JPPyObject JPProxyFunctional::getCallable(PyObject* name, int& addSelf)
{
	if (name == m_Functional->getMethod())
		return JPPyObject::accept(PyObject_GetAttr(m_Instance->m_Dispatch, Py_JP_CALL));
	return JPPyObject::accept(PyObject_GetAttr((PyObject*) m_Instance, name));
}


