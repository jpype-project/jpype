// --- file: python/pyjp_misc.cpp ---
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
#include "jp_primitive_accessor.h"
#include "jp_gc.h"
#include "jp_proxy.h"

#ifdef WIN32
#include <Windows.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL
Java_org_jpype_manager_StringManager_impl(JNIEnv *env, jclass obj, jlong contextPtr, jstring name)
{
	JPContext* context = reinterpret_cast<JPContext*>(contextPtr);
	PyJPModuleState* st = context->modulestate;
	JPJavaFrame frame = JPJavaFrame::external(env, context);
	JPPyCallAcquire callback(st);
	try
	{
		if (st->strings_dict == nullptr)
			return 0;


		string str = frame.toStringUTF8(name);
		auto len = static_cast<Py_ssize_t>(str.size());
		JPPyObject bytes = JPPyObject::call(PyBytes_FromStringAndSize(str.c_str(), len));

		// This messes with ownership so it is dangerous to apply as RAII
		PyObject* pyStr = PyUnicode_FromEncodedObject(bytes.get(), "UTF-8", "strict");
		if (pyStr == nullptr)
			JP_RAISE_PYTHON();

		PyUnicode_InternInPlace(&pyStr);

		PyObject* canonical = PyDict_SetDefault(st->strings_dict, pyStr, pyStr);
		Py_XINCREF(canonical);
		Py_DECREF(pyStr);
		return reinterpret_cast<jlong>(canonical);
	}
	catch (JPypeException& ex)
	{
		ex.toJava(frame);
	}
	catch (...)
	{
		JPContext* context = reinterpret_cast<JPContext*>(contextPtr);
		if (context != nullptr)
			env->ThrowNew(context->m_RuntimeException, "unknown error");
	}
	return 0;
}

void PyJPModule_rethrow(const JPStackInfo& info)
{
	JP_TRACE_IN("PyJPModule_rethrow");
	JP_TRACE(info.getFile(), info.getLine());
	try
	{
		throw;
	} catch (JPypeException& ex)
	{
		ex.from(info); // this likely wont be necessary, but for now we will add the entry point.
		ex.toPython();
		return;
	} catch (std::exception &ex)
	{
		PyErr_Format(PyExc_RuntimeError, "Unhandled C++ exception occurred: %s", ex.what());
		return;
	} catch (...)
	{
		PyErr_Format(PyExc_RuntimeError, "Unhandled C++ exception occurred");
		return;
	}
	JP_TRACE_OUT; // GCOVR_EXCL_LINE
}

PyObject *PyJPModule_convertBuffer(PyJPModuleState* st, JPPyBuffer& buffer, PyObject *dtype)
{
	JPContext *context = st->context;
	if (context == nullptr)
	{
		PyErr_SetString(PyExc_RuntimeError, "JPype module context is not available");
		return nullptr;
	}

	JPJavaFrame frame = JPJavaFrame::outer(context);
	Py_buffer& view = buffer.getView();

	// Okay two possibilities here. We have a valid dtype specified,
	// or we need to figure it out from the buffer.
	JPClass *cls = nullptr;

	if (view.suboffsets != nullptr && view.suboffsets[view.ndim - 1] > 0)
	{
		PyErr_Format(PyExc_TypeError, "last dimension is not contiguous");
		return nullptr;
	}

	// First lets find out what we are unpacking
	Py_ssize_t itemsize = view.itemsize;
	const char *format = view.format;
	if (format == nullptr)
		format = "B";

	// Standard size for 'l' is 4 in docs, but numpy uses format 'l' for long long
	if (itemsize == 8 && format[0] == 'l')
		format = "q";
	if (itemsize == 8 && format[0] == 'L')
		format = "Q";

	if (dtype != nullptr && dtype != Py_None)
	{
		cls = PyJPClass_getJPClass(dtype);
		if (cls == nullptr || !cls->isPrimitive())
		{
			PyErr_Format(PyExc_TypeError, "'%s' is not a Java primitive type", Py_TYPE(dtype)->tp_name);
			return nullptr;
		}
	}
	else
	{
		switch (format[0])
		{
			case '?': cls = context->_boolean; break;
			case 'c': break;
			case 'b': cls = context->_byte; break;
			case 'B': break;
			case 'h': cls = context->_short; break;
			case 'H': break;
			case 'i':
			case 'l': cls = context->_int; break;
			case 'I':
			case 'L': break;
			case 'q': cls = context->_long; break;
			case 'Q': break;
			case 'f': cls = context->_float; break;
			case 'd': cls = context->_double; break;
			case 'n':
			case 'N':
			case 'P':
			default:
				break;
		}

		if (cls == nullptr)
		{
			PyErr_Format(PyExc_TypeError, "'%s' type code not supported without dtype specified", format);
			return nullptr;
		}
	}

	// Now we have a valid format code, so next lets get a converter for the type.
	auto *pcls = dynamic_cast<JPPrimitiveType *>(cls);

	// Convert the shape
	Py_ssize_t subs = 1;
	Py_ssize_t base = 1;
	auto jdims = (jintArray) context->_int->newArrayOf(frame, view.ndim);
	if (view.shape != nullptr)
	{
		JPPrimitiveArrayAccessor<jintArray, jint*> accessor(frame, jdims,
				&JPJavaFrame::GetIntArrayElements, &JPJavaFrame::ReleaseIntArrayElements);
		jint *a = accessor.get();
		for (int i = 0; i < view.ndim; ++i)
			a[i] = view.shape[i];

		accessor.commit();

		for (int i = 0; i < view.ndim - 1; ++i)
			subs *= view.shape[i];

		base = view.shape[view.ndim - 1];
	}
	else
	{
		if (view.ndim > 1)
		{
			PyErr_Format(PyExc_TypeError, "buffer dims inconsistent");
			return nullptr;
		}
		base = view.len / view.itemsize;
	}

	return pcls->newMultiArray(frame, buffer, subs, base, (jobject) jdims);
}

void PyJPModule_installGC(PyObject* module)
{
	// Get the Python garbage collector
	JPPyObject gc = JPPyObject::call(PyImport_ImportModule("gc"));

	// Find the callbacks
	JPPyObject callbacks = JPPyObject::call(PyObject_GetAttrString(gc.get(), "callbacks"));

	// Hook up our callback
	JPPyObject collect = JPPyObject::call(PyObject_GetAttrString(module, "_collect"));
	PyList_Append(callbacks.get(), collect.get());
	JP_PY_CHECK();
}

#ifdef __cplusplus
}
#endif

#ifdef JP_INSTRUMENTATION

// Test-only fault injection trigger, set via _jpype.fault("Name") from
// Python. Deliberately a single process-wide global rather than
// per-interpreter state: PyJPModuleFault_check/throw are called from deep
// inside native/common/ call frames (JP_TRACE_IN/JP_FAULT_RETURN/JP_BLOCK)
// with no module/state pointer available, and this path never runs in
// production (JP_INSTRUMENTATION is coverage-build only).
uint32_t _PyJPModule_fault_code = 0;

int PyJPModuleFault_check(uint32_t code)
{
	return (code == _PyJPModule_fault_code);
}

void PyJPModuleFault_throw(uint32_t code)
{
	if (code == _PyJPModule_fault_code)
	{
		_PyJPModule_fault_code = (uint32_t) -1;
		JP_RAISE(PyExc_SystemError, "fault");
	}
}
#endif


