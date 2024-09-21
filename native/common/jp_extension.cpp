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
 **************************************************************************** */
#include "include/jp_class.h"
#include "jpype.h"
#include "jp_extension.hpp" // IWYU pragma: keep

static JPPyObject packArgs(JPContext* context, const JPMethodOverride &method, jobjectArray args)
{
	JP_TRACE_IN("JProxy::getArgs");
	JPJavaFrame frame = JPJavaFrame::outer(context);
	const size_t argLen = method.paramTypes.size();
	JPPyObject pyargs = JPPyObject::call(PyTuple_New(argLen));

	for (jsize i = 0; i < argLen; i++)
	{
		jobject obj = frame.GetObjectArrayElement(args, i);
		JPClass* type = frame.findClassForObject(obj);
		if (type == NULL) {
			// this should be immutable once built...
			type = const_cast<JPClass*>(method.paramTypes[i]);
		}
		JPValue val = type->getValueFromObject(JPValue(type, obj));
		PyTuple_SetItem(pyargs.get(), i, type->convertToPythonObject(frame, val, false).keep());
	}
	return pyargs;
	JP_TRACE_OUT;
}

extern "C" JNIEXPORT jobject JNICALL Java_org_jpype_extension_Factory__call(
		JNIEnv *env,
		jclass clazz,
		jlong contextPtr,
		jlong functionId,
		jobjectArray args
	)
{
	JPClass *cls = (JPClass *) contextPtr;
	JPContext* context = cls->getContext();
	JPJavaFrame frame = JPJavaFrame::external(context, env);

	// We need the resources to be held for the full duration of the proxy.
	JPPyCallAcquire callback;
	try {
		JP_TRACE_IN("JPype_InvocationHandler_hostInvoke");
		JP_TRACE("context", context);
		try
		{
			const JPMethodOverride &method = cls->getOverrides()->at(functionId);
			// Find the return type

			// why are these functions mutating it??????????
			JPClass* returnClass = const_cast<JPClass*>(method.returnType);
			JP_TRACE("Get return type", returnClass->getCanonicalName());

			// convert the arguments into a python list
			JP_TRACE("Convert arguments");
			JPPyObject pyargs = packArgs(context, method, args);

			// Copy the privilege flags into the first argument
			// FIXME how should this be stored.

			JP_TRACE("Call Python");
			JPPyObject returnValue = JPPyObject::call(PyObject_Call(
					reinterpret_cast<PyObject*> (functionId),
					pyargs.get(), NULL));

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
			JPMatch returnMatch(&frame, returnValue.get());
			if (returnClass->isPrimitive())
			{
				JP_TRACE("Box return");
				if (returnClass->findJavaConversion(returnMatch) == JPMatch::_none)
					JP_RAISE(PyExc_TypeError, "Return value is not compatible with required type.");
				jvalue res = returnMatch.convert();
				JPBoxedType *boxed = (JPBoxedType *) ((JPPrimitiveType*) returnClass)->getBoxedClass(context);
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
			ex.toJava(context);
		} catch (...) // GCOVR_EXCL_LINE
		{
			JP_TRACE("Other Exception raised");
			env->functions->ThrowNew(env, context->m_RuntimeException.get(),
					"unknown error occurred");
		}
		return NULL;
		JP_TRACE_OUT; // GCOVR_EXCL_LINE
	}
	catch (...) // JP_TRACE_OUT implies a throw but that is not allowed.
	{}
	return NULL;
}

