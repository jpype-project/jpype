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
#include <Python.h>
#include "jpype.h"
#include "jp_reference_queue.h"

/*****************************************************************************/
// Local frames represent the JNIEnv for memory handling all java
// resources should be created within them.
//

#if defined(JP_TRACING_ENABLE) || defined(JP_INSTRUMENTATION)

static void jpype_frame_check(int popped)
{
	if (popped)
		JP_RAISE(PyExc_SystemError, "Local reference outside of frame");
}
#define JP_FRAME_CHECK() jpype_frame_check(m_Popped)
#else
#define JP_FRAME_CHECK() if (false) while (false)
#endif

JPJavaFrame::JPJavaFrame(JNIEnv* p_env, int size, bool outer)
: m_Env(p_env), m_Popped(false), m_Outer(outer)
{
	JPContext* context = JPContext_global;

	if (p_env == nullptr)
	{
		if (outer)
		{
#ifdef JP_INSTRUMENTATION
			PyJPModuleFault_throw(compile_hash("PyJPModule_getContext"));
#endif
			assertJVMRunning((JPContext*)context, JP_STACKINFO());
		}
		m_Env = context->getEnv();
	}

	// Create a memory management frame to live in
	m_Env->PushLocalFrame(size);
	JP_TRACE_JAVA("JavaFrame", (jobject) - 1);
}

JPJavaFrame::JPJavaFrame(const JPJavaFrame& frame)
: m_Env(frame.m_Env), m_Popped(false), m_Outer(false)
{
	// Create a memory management frame to live in
	m_Env->PushLocalFrame(LOCAL_FRAME_DEFAULT);
	JP_TRACE_JAVA("JavaFrame (copy)", (jobject) - 1);
}

JPContext* JPJavaFrame::getContext()
{
	// We can add guard statements here.
	return JPContext_global;
}

jobject JPJavaFrame::keep(jobject obj)
{
	if (m_Outer)
		JP_RAISE(PyExc_SystemError, "Keep on outer frame");
	JP_FRAME_CHECK();
	m_Popped = true;
	JP_TRACE_JAVA("Keep", obj);
	JP_TRACE_JAVA("~JavaFrame (keep)", (jobject) - 2);
	obj = m_Env->PopLocalFrame(obj);
	JP_TRACE_JAVA("Return", obj);
	return obj;
}

JPJavaFrame::~JPJavaFrame()
{
	// Check if we have already closed the frame.
	if (!m_Popped)
	{
		JP_TRACE_JAVA("~JavaFrame", (jobject) - 2);
		m_Env->PopLocalFrame(nullptr);
		JP_FRAME_CHECK();
	}

	// It is not safe to detach as we would lose all local references including
	// any we want to keep.
}

void JPJavaFrame::DeleteLocalRef(jobject obj)
{
	JP_TRACE_JAVA("Delete local", obj);
	m_Env->DeleteLocalRef(obj);
}

void JPJavaFrame::DeleteGlobalRef(jobject obj)
{
	JP_TRACE_JAVA("Delete global", obj);
	m_Env->DeleteGlobalRef(obj);
}

jweak JPJavaFrame::NewWeakGlobalRef(jobject obj)
{
	JP_FRAME_CHECK();
	JP_TRACE_JAVA("New weak", obj);
	jweak obj2 = m_Env->NewWeakGlobalRef(obj);
	JP_TRACE_JAVA("Weak", obj2);
	return obj2;
}

void JPJavaFrame::DeleteWeakGlobalRef(jweak obj)
{
	JP_TRACE_JAVA("Delete weak", obj);
	return m_Env->DeleteWeakGlobalRef(obj);
}

jobject JPJavaFrame::NewLocalRef(jobject obj)
{
	JP_FRAME_CHECK();
	JP_TRACE_JAVA("New local", obj);
	obj = m_Env->NewLocalRef(obj);
	JP_TRACE_JAVA("Local", obj);
	return obj;
}

jobject JPJavaFrame::NewGlobalRef(jobject obj)
{
	JP_TRACE_JAVA("New Global", obj);
	obj = m_Env->NewGlobalRef(obj);
	JP_TRACE_JAVA("Global", obj);
	return obj;
}

/*****************************************************************************/
// Exceptions
// TODO: why is this never used? Should be deleted if obsolete.
bool JPJavaFrame::ExceptionCheck()
{
	return m_Env->ExceptionCheck() != 0;
}

void JPJavaFrame::ExceptionDescribe()
{
	m_Env->ExceptionDescribe();
}

void JPJavaFrame::ExceptionClear()
{
	m_Env->ExceptionClear();
}

jint JPJavaFrame::ThrowNew(jclass clazz, const char* msg)
{
	return m_Env->ThrowNew(clazz, msg);
}

jint JPJavaFrame::Throw(jthrowable th)
{
	return m_Env->Throw(th);
}

jthrowable JPJavaFrame::ExceptionOccurred()
{
	JP_FRAME_CHECK();
	jthrowable obj;

	obj = m_Env->ExceptionOccurred();
#ifdef JP_TRACING_ENABLE
	if (obj)
	{
		m_Env->ExceptionDescribe();
	}
#endif
	JP_TRACE_JAVA("Exception", obj);
	return obj;
}

/*****************************************************************************/

#ifdef JP_INSTRUMENTATION
#define JAVA_RETURN(X,Y,Z) \
  PyJPModuleFault_throw(compile_hash(Y)); \
  X ret = Z; \
  check(); \
  return ret;
#define JAVA_RETURN_OBJ(X,Y,Z) \
  PyJPModuleFault_throw(compile_hash(Y)); \
  X ret = Z; \
  check(); \
  return ret;
#define JAVA_CHECK(Y,Z) \
  PyJPModuleFault_throw(compile_hash(Y)); \
  Z; \
  check();
#else
#define JAVA_RETURN(X,Y,Z) \
  X ret = Z; \
  JP_TRACE_JAVA(Y, 0); \
  check(); \
  return ret;
#define JAVA_RETURN_OBJ(X,Y,Z) \
  JP_FRAME_CHECK(); \
  X ret = Z; \
  JP_TRACE_JAVA(Y, ret); \
  check(); \
  return ret;
#define JAVA_CHECK(Y,Z) \
  Z; \
  JP_TRACE_JAVA(Y, 0); \
  check();
#endif

void JPJavaFrame::check()
{
	JP_FRAME_CHECK();
	if (m_Env && m_Env->ExceptionCheck() == JNI_TRUE)
	{
		jthrowable th = m_Env->ExceptionOccurred();
		JP_TRACE_JAVA("ExceptionOccurred", th);
		m_Env->ExceptionClear();
		JP_TRACE_JAVA("ExceptionClear", 0);
		throw JPypeException(*this, th, JP_STACKINFO());
	}
}

jobject JPJavaFrame::NewObjectA(jclass a0, jmethodID a1, jvalue* a2)
{
	jobject res;
	JP_FRAME_CHECK();

	// Allocate the object
	JAVA_CHECK("JPJavaFrame::JPJavaFrame::NewObjectA",
			res = m_Env->AllocObject(a0));
	JAVA_CHECK("JPJavaFrame::NewObjectA::AllocObject",
			m_Env->CallVoidMethodA(res, a1, a2));

	JP_TRACE_JAVA("NewObjectA", res);
	// Initialize the object
	return res;
}

jobject JPJavaFrame::NewDirectByteBuffer(void* address, jlong capacity)
{
	JAVA_RETURN_OBJ(jobject, "JPJavaFrame::NewDirectByteBuffer",
			m_Env->NewDirectByteBuffer(address, capacity));
}

/*****************************************************************************/

jbyte JPJavaFrame::GetStaticByteField(jclass clazz, jfieldID fid)
{
	JAVA_RETURN(jbyte, "JPJavaFrame::GetStaticByteField",
			m_Env->GetStaticByteField(clazz, fid));
}

jbyte JPJavaFrame::GetByteField(jobject clazz, jfieldID fid)
{
	JAVA_RETURN(jbyte, "JPJavaFrame::GetByteField",
			m_Env->GetByteField(clazz, fid));
}

void JPJavaFrame::SetStaticByteField(jclass clazz, jfieldID fid, jbyte val)
{
	JAVA_CHECK("JPJavaFrame::SetStaticByteField",
			m_Env->SetStaticByteField(clazz, fid, val));
}

void JPJavaFrame::SetByteField(jobject clazz, jfieldID fid, jbyte val)
{
	JAVA_CHECK("JPJavaFrame::SetByteField",
			m_Env->SetByteField(clazz, fid, val));
}

jbyte JPJavaFrame::CallStaticByteMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jbyte, "JPJavaFrame::CallStaticByteMethodA",
			m_Env->CallStaticByteMethodA(clazz, mid, val));
}

jbyte JPJavaFrame::CallByteMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jbyte, "JPJavaFrame::CallByteMethodA",
			m_Env->CallByteMethodA(obj, mid, val));
}

jbyte JPJavaFrame::CallNonvirtualByteMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jbyte, "JPJavaFrame::CallNonvirtualByteMethodA",
			m_Env->CallNonvirtualByteMethodA(obj, claz, mid, val));
}

jshort JPJavaFrame::GetStaticShortField(jclass clazz, jfieldID fid)
{
	JAVA_RETURN(jshort, "JPJavaFrame::GetStaticShortField",
			m_Env->GetStaticShortField(clazz, fid));
}

jshort JPJavaFrame::GetShortField(jobject clazz, jfieldID fid)
{
	JAVA_RETURN(jshort, "JPJavaFrame::GetShortField",
			m_Env->GetShortField(clazz, fid));
}

void JPJavaFrame::SetStaticShortField(jclass clazz, jfieldID fid, jshort val)
{
	JAVA_CHECK("JPJavaFrame::SetStaticShortField",
			m_Env->SetStaticShortField(clazz, fid, val));
}

void JPJavaFrame::SetShortField(jobject clazz, jfieldID fid, jshort val)
{
	JAVA_CHECK("JPJavaFrame::SetShortField",
			m_Env->SetShortField(clazz, fid, val));
}

jshort JPJavaFrame::CallStaticShortMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jshort, "JPJavaFrame::CallStaticShortMethodA",
			m_Env->CallStaticShortMethodA(clazz, mid, val));
}

jshort JPJavaFrame::CallShortMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jshort, "JPJavaFrame::CallShortMethodA",
			m_Env->CallShortMethodA(obj, mid, val));
}

jshort JPJavaFrame::CallNonvirtualShortMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jshort, "JPJavaFrame::CallNonvirtualShortMethodA",
			m_Env->CallNonvirtualShortMethodA(obj, claz, mid, val));
}

jint JPJavaFrame::GetStaticIntField(jclass clazz, jfieldID fid)
{
	JAVA_RETURN(jint, "JPJavaFrame::GetStaticIntField",
			m_Env->GetStaticIntField(clazz, fid));
}

jint JPJavaFrame::GetIntField(jobject clazz, jfieldID fid)
{
	JAVA_RETURN(jint, "JPJavaFrame::GetIntField",
			m_Env->GetIntField(clazz, fid));
}

void JPJavaFrame::SetStaticIntField(jclass clazz, jfieldID fid, jint val)
{
	JAVA_CHECK("JPJavaFrame::SetStaticIntField",
			m_Env->SetStaticIntField(clazz, fid, val));
}

void JPJavaFrame::SetIntField(jobject clazz, jfieldID fid, jint val)
{
	JAVA_CHECK("JPJavaFrame::SetIntField",
			m_Env->SetIntField(clazz, fid, val));
}

jint JPJavaFrame::CallStaticIntMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jint, "JPJavaFrame::CallStaticIntMethodA",
			m_Env->CallStaticIntMethodA(clazz, mid, val));
}

jint JPJavaFrame::CallIntMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jint, "JPJavaFrame::CallIntMethodA",
			m_Env->CallIntMethodA(obj, mid, val));
}

jint JPJavaFrame::CallNonvirtualIntMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jint, "JPJavaFrame::CallNonvirtualIntMethodA",
			m_Env->CallNonvirtualIntMethodA(obj, claz, mid, val));
}

jlong JPJavaFrame::GetStaticLongField(jclass clazz, jfieldID fid)
{
	JAVA_RETURN(jlong, "JPJavaFrame::GetStaticLongField",
			m_Env->GetStaticLongField(clazz, fid));
}

jlong JPJavaFrame::GetLongField(jobject clazz, jfieldID fid)
{
	JAVA_RETURN(jlong, "JPJavaFrame::GetLongField",
			m_Env->GetLongField(clazz, fid));
}

void JPJavaFrame::SetStaticLongField(jclass clazz, jfieldID fid, jlong val)
{
	JAVA_CHECK("JPJavaFrame::SetStaticLongField",
			m_Env->SetStaticLongField(clazz, fid, val));
}

void JPJavaFrame::SetLongField(jobject clazz, jfieldID fid, jlong val)
{
	JAVA_CHECK("JPJavaFrame::SetLongField",
			m_Env->SetLongField(clazz, fid, val));
}

jlong JPJavaFrame::CallStaticLongMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jlong, "JPJavaFrame::CallStaticLongMethodA",
			m_Env->CallStaticLongMethodA(clazz, mid, val));
}

jlong JPJavaFrame::CallLongMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jlong, "JPJavaFrame::CallLongMethodA",
			m_Env->CallLongMethodA(obj, mid, val));
}

jlong JPJavaFrame::CallNonvirtualLongMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jlong, "JPJavaFrame::CallNonvirtualLongMethodA",
			m_Env->CallNonvirtualLongMethodA(obj, claz, mid, val));
}

jfloat JPJavaFrame::GetStaticFloatField(jclass clazz, jfieldID fid)
{
	JAVA_RETURN(jfloat, "JPJavaFrame::GetStaticFloatField",
			m_Env->GetStaticFloatField(clazz, fid));
}

jfloat JPJavaFrame::GetFloatField(jobject clazz, jfieldID fid)
{
	JAVA_RETURN(jfloat, "JPJavaFrame::GetFloatField",
			m_Env->GetFloatField(clazz, fid));
}

void JPJavaFrame::SetStaticFloatField(jclass clazz, jfieldID fid, jfloat val)
{
	JAVA_CHECK("JPJavaFrame::SetStaticFloatField",
			m_Env->SetStaticFloatField(clazz, fid, val));
}

void JPJavaFrame::SetFloatField(jobject clazz, jfieldID fid, jfloat val)
{
	JAVA_CHECK("JPJavaFrame::SetFloatField",
			m_Env->SetFloatField(clazz, fid, val));
}

jfloat JPJavaFrame::CallStaticFloatMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jfloat, "JPJavaFrame::CallStaticFloatMethodA",
			m_Env->CallStaticFloatMethodA(clazz, mid, val));
}

jfloat JPJavaFrame::CallFloatMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jfloat, "JPJavaFrame::CallFloatMethodA",
			m_Env->CallFloatMethodA(obj, mid, val));
}

jfloat JPJavaFrame::CallNonvirtualFloatMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jfloat, "JPJavaFrame::CallNonvirtualFloatMethodA",
			m_Env->CallNonvirtualFloatMethodA(obj, claz, mid, val));
}

jdouble JPJavaFrame::GetStaticDoubleField(jclass clazz, jfieldID fid)
{
	JAVA_RETURN(jdouble, "JPJavaFrame::GetStaticDoubleField",
			m_Env->GetStaticDoubleField(clazz, fid));
}

jdouble JPJavaFrame::GetDoubleField(jobject clazz, jfieldID fid)
{
	JAVA_RETURN(jdouble, "JPJavaFrame::GetDoubleField",
			m_Env->GetDoubleField(clazz, fid));
}

void JPJavaFrame::SetStaticDoubleField(jclass clazz, jfieldID fid, jdouble val)
{
	JAVA_CHECK("JPJavaFrame::SetStaticDoubleField",
			m_Env->SetStaticDoubleField(clazz, fid, val));
}

void JPJavaFrame::SetDoubleField(jobject clazz, jfieldID fid, jdouble val)
{
	JAVA_CHECK("JPJavaFrame::SetDoubleField",
			m_Env->SetDoubleField(clazz, fid, val));
}

jdouble JPJavaFrame::CallStaticDoubleMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jdouble, "JPJavaFrame::CallStaticDoubleMethodA",
			m_Env->CallStaticDoubleMethodA(clazz, mid, val));
}

jdouble JPJavaFrame::CallDoubleMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jdouble, "JPJavaFrame::CallDoubleMethodA",
			m_Env->CallDoubleMethodA(obj, mid, val));
}

jdouble JPJavaFrame::CallNonvirtualDoubleMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jdouble, "JPJavaFrame::CallNonvirtualDoubleMethodA",
			m_Env->CallNonvirtualDoubleMethodA(obj, claz, mid, val));
}

jchar JPJavaFrame::GetStaticCharField(jclass clazz, jfieldID fid)
{
	JAVA_RETURN(jchar, "JPJavaFrame::GetStaticCharField",
			m_Env->GetStaticCharField(clazz, fid));
}

jchar JPJavaFrame::GetCharField(jobject clazz, jfieldID fid)
{
	JAVA_RETURN(jchar, "JPJavaFrame::GetCharField",
			m_Env->GetCharField(clazz, fid));
}

void JPJavaFrame::SetStaticCharField(jclass clazz, jfieldID fid, jchar val)
{
	JAVA_CHECK("JPJavaFrame::SetStaticCharField",
			m_Env->SetStaticCharField(clazz, fid, val));
}

void JPJavaFrame::SetCharField(jobject clazz, jfieldID fid, jchar val)
{
	JAVA_CHECK("JPJavaFrame::SetCharField",
			m_Env->SetCharField(clazz, fid, val));
}

jchar JPJavaFrame::CallStaticCharMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jchar, "JPJavaFrame::CallStaticCharMethodA",
			m_Env->CallStaticCharMethodA(clazz, mid, val));
}

jchar JPJavaFrame::CallCharMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jchar, "JPJavaFrame::CallCharMethodA",
			m_Env->CallCharMethodA(obj, mid, val));
}

jchar JPJavaFrame::CallNonvirtualCharMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jchar, "JPJavaFrame::CallNonvirtualCharMethodA",
			m_Env->CallNonvirtualCharMethodA(obj, claz, mid, val));
}

jboolean JPJavaFrame::GetStaticBooleanField(jclass clazz, jfieldID fid)
{
	JAVA_RETURN(jboolean, "JPJavaFrame::GetStaticBooleanField",
			m_Env->GetStaticBooleanField(clazz, fid));
}

jboolean JPJavaFrame::GetBooleanField(jobject clazz, jfieldID fid)
{
	JAVA_RETURN(jboolean, "JPJavaFrame::GetBooleanField",
			m_Env->GetBooleanField(clazz, fid));
}

void JPJavaFrame::SetStaticBooleanField(jclass clazz, jfieldID fid, jboolean val)
{
	JAVA_CHECK("JPJavaFrame::SetStaticBooleanField",
			m_Env->SetStaticBooleanField(clazz, fid, val));
}

void JPJavaFrame::SetBooleanField(jobject clazz, jfieldID fid, jboolean val)
{
	JAVA_CHECK("JPJavaFrame::SetBooleanField",
			m_Env->SetBooleanField(clazz, fid, val));
}

jboolean JPJavaFrame::CallStaticBooleanMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jboolean, "JPJavaFrame::CallStaticBooleanMethodA",
			m_Env->CallStaticBooleanMethodA(clazz, mid, val));
}

jboolean JPJavaFrame::CallBooleanMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jboolean, "JPJavaFrame::CallBooleanMethodA",
			m_Env->CallBooleanMethodA(obj, mid, val));
}

jboolean JPJavaFrame::CallNonvirtualBooleanMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN(jboolean, "JPJavaFrame::CallNonvirtualBooleanMethodA",
			m_Env->CallNonvirtualBooleanMethodA(obj, claz, mid, val));
}

jclass JPJavaFrame::GetObjectClass(jobject obj)
{
	JAVA_RETURN_OBJ(jclass, "JPJavaFrame::GetObjectClass",
			m_Env->GetObjectClass(obj));
}

jobject JPJavaFrame::GetStaticObjectField(jclass clazz, jfieldID fid)
{
	JAVA_RETURN_OBJ(jobject, "JPJavaFrame::GetStaticObjectField",
			m_Env->GetStaticObjectField(clazz, fid));
}

jobject JPJavaFrame::GetObjectField(jobject clazz, jfieldID fid)
{
	JAVA_RETURN_OBJ(jobject, "JPJavaFrame::GetObjectField",
			m_Env->GetObjectField(clazz, fid));
}

void JPJavaFrame::SetStaticObjectField(jclass clazz, jfieldID fid, jobject val)
{
	JAVA_CHECK("JPJavaFrame::SetStaticObjectField",
			m_Env->SetStaticObjectField(clazz, fid, val));
}

void JPJavaFrame::SetObjectField(jobject clazz, jfieldID fid, jobject val)
{
	JAVA_CHECK("JPJavaFrame::SetObjectField",
			m_Env->SetObjectField(clazz, fid, val));
}

jobject JPJavaFrame::CallStaticObjectMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN_OBJ(jobject, "JPJavaFrame::CallStaticObjectMethodA",
			m_Env->CallStaticObjectMethodA(clazz, mid, val));
}

jobject JPJavaFrame::CallObjectMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JAVA_RETURN_OBJ(jobject, "JPJavaFrame::CallObjectMethodA",
			m_Env->CallObjectMethodA(obj, mid, val));
}

jobject JPJavaFrame::CallNonvirtualObjectMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JAVA_RETURN_OBJ(jobject, "JPJavaFrame::CallNonvirtualObjectMethodA",
			m_Env->CallNonvirtualObjectMethodA(obj, claz, mid, val));
}

jbyteArray JPJavaFrame::NewByteArray(jsize len)
{
	JAVA_RETURN_OBJ(jbyteArray, "JPJavaFrame::NewByteArray",
			m_Env->NewByteArray(len));
}

void JPJavaFrame::SetByteArrayRegion(jbyteArray array, jsize start, jsize len, jbyte* vals)
{
	JAVA_CHECK("JPJavaFrame::SetByteArrayRegion",
			m_Env->SetByteArrayRegion(array, start, len, vals));
}

void JPJavaFrame::GetByteArrayRegion(jbyteArray array, jsize start, jsize len, jbyte* vals)
{
	JAVA_CHECK("JPJavaFrame::GetByteArrayRegion",
			m_Env->GetByteArrayRegion(array, start, len, vals));
}

jbyte* JPJavaFrame::GetByteArrayElements(jbyteArray array, jboolean* isCopy)
{
	JAVA_RETURN(jbyte*, "JPJavaFrame::GetByteArrayElements",
			m_Env->GetByteArrayElements(array, isCopy));
}

void JPJavaFrame::ReleaseByteArrayElements(jbyteArray array, jbyte* v, jint mode)
{
	JAVA_CHECK("JPJavaFrame::ReleaseByteArrayElements",
			m_Env->ReleaseByteArrayElements(array, v, mode));
}

jshortArray JPJavaFrame::NewShortArray(jsize len)
{
	JAVA_RETURN_OBJ(jshortArray, "JPJavaFrame::NewShortArray",
			m_Env->NewShortArray(len));
}

void JPJavaFrame::SetShortArrayRegion(jshortArray array, jsize start, jsize len, jshort* vals)
{
	JAVA_CHECK("JPJavaFrame::SetShortArrayRegion",
			m_Env->SetShortArrayRegion(array, start, len, vals));
}

void JPJavaFrame::GetShortArrayRegion(jshortArray array, jsize start, jsize len, jshort* vals)
{
	JAVA_CHECK("JPJavaFrame::GetShortArrayRegion",
			m_Env->GetShortArrayRegion(array, start, len, vals));
}

jshort* JPJavaFrame::GetShortArrayElements(jshortArray array, jboolean* isCopy)
{
	JAVA_RETURN(jshort*, "JPJavaFrame::GetShortArrayElements",
			m_Env->GetShortArrayElements(array, isCopy));
}

void JPJavaFrame::ReleaseShortArrayElements(jshortArray array, jshort* v, jint mode)
{
	JAVA_CHECK("JPJavaFrame::ReleaseShortArrayElements",
			m_Env->ReleaseShortArrayElements(array, v, mode));
}

jintArray JPJavaFrame::NewIntArray(jsize len)
{
	JAVA_RETURN_OBJ(jintArray, "JPJavaFrame::NewIntArray",
			m_Env->NewIntArray(len));
}

void JPJavaFrame::SetIntArrayRegion(jintArray array, jsize start, jsize len, jint* vals)
{
	JAVA_CHECK("JPJavaFrame::SetIntArrayRegion",
			m_Env->SetIntArrayRegion(array, start, len, vals));
}

void JPJavaFrame::GetIntArrayRegion(jintArray array, jsize start, jsize len, jint* vals)
{
	JAVA_CHECK("JPJavaFrame::GetIntArrayRegion",
			m_Env->GetIntArrayRegion(array, start, len, vals));
}

jint* JPJavaFrame::GetIntArrayElements(jintArray array, jboolean* isCopy)
{
	JAVA_RETURN(jint*, "JPJavaFrame::GetIntArrayElements",
			m_Env->GetIntArrayElements(array, isCopy));
}

void JPJavaFrame::ReleaseIntArrayElements(jintArray array, jint* v, jint mode)
{
	JAVA_CHECK("JPJavaFrame::ReleaseIntArrayElements",
			m_Env->ReleaseIntArrayElements(array, v, mode));
}

jlongArray JPJavaFrame::NewLongArray(jsize len)
{
	JAVA_RETURN_OBJ(jlongArray, "JPJavaFrame::NewLongArray",
			m_Env->NewLongArray(len));
}

void JPJavaFrame::SetLongArrayRegion(jlongArray array, jsize start, jsize len, jlong* vals)
{
	JAVA_CHECK("JPJavaFrame::SetLongArrayRegion",
			m_Env->SetLongArrayRegion(array, start, len, vals));
}

void JPJavaFrame::GetLongArrayRegion(jlongArray array, jsize start, jsize len, jlong* vals)
{
	JAVA_CHECK("JPJavaFrame::GetLongArrayRegion",
			m_Env->GetLongArrayRegion(array, start, len, vals));
}

jlong* JPJavaFrame::GetLongArrayElements(jlongArray array, jboolean* isCopy)
{
	JAVA_RETURN(jlong*, "JPJavaFrame::GetLongArrayElements",
			m_Env->GetLongArrayElements(array, isCopy));
}

void JPJavaFrame::ReleaseLongArrayElements(jlongArray array, jlong* v, jint mode)
{
	JAVA_CHECK("JPJavaFrame::ReleaseLongArrayElements",
			m_Env->ReleaseLongArrayElements(array, v, mode));
}

jfloatArray JPJavaFrame::NewFloatArray(jsize len)
{
	JAVA_RETURN_OBJ(jfloatArray, "JPJavaFrame::NewFloatArray",
			m_Env->NewFloatArray(len));
}

void JPJavaFrame::SetFloatArrayRegion(jfloatArray array, jsize start, jsize len, jfloat* vals)
{
	JAVA_CHECK("JPJavaFrame::SetFloatArrayRegion",
			m_Env->SetFloatArrayRegion(array, start, len, vals));
}

void JPJavaFrame::GetFloatArrayRegion(jfloatArray array, jsize start, jsize len, jfloat* vals)
{
	JAVA_CHECK("JPJavaFrame::GetFloatArrayRegion",
			m_Env->GetFloatArrayRegion(array, start, len, vals));
}

jfloat* JPJavaFrame::GetFloatArrayElements(jfloatArray array, jboolean* isCopy)
{
	JAVA_RETURN(jfloat*, "JPJavaFrame::GetFloatArrayElements",
			m_Env->GetFloatArrayElements(array, isCopy));
}

void JPJavaFrame::ReleaseFloatArrayElements(jfloatArray array, jfloat* v, jint mode)
{
	JAVA_CHECK("JPJavaFrame::ReleaseFloatArrayElements",
			m_Env->ReleaseFloatArrayElements(array, v, mode));
}

jdoubleArray JPJavaFrame::NewDoubleArray(jsize len)
{
	JAVA_RETURN_OBJ(jdoubleArray, "JPJavaFrame::NewDoubleArray",
			m_Env->NewDoubleArray(len));
}

void JPJavaFrame::SetDoubleArrayRegion(jdoubleArray array, jsize start, jsize len, jdouble* vals)
{
	JAVA_CHECK("JPJavaFrame::SetDoubleArrayRegion",
			m_Env->SetDoubleArrayRegion(array, start, len, vals));
}

void JPJavaFrame::GetDoubleArrayRegion(jdoubleArray array, jsize start, jsize len, jdouble* vals)
{
	JAVA_CHECK("JPJavaFrame::GetDoubleArrayRegion",
			m_Env->GetDoubleArrayRegion(array, start, len, vals));
}

jdouble* JPJavaFrame::GetDoubleArrayElements(jdoubleArray array, jboolean* isCopy)
{
	JAVA_RETURN(jdouble*, "JPJavaFrame::GetDoubleArrayElements",
			m_Env->GetDoubleArrayElements(array, isCopy));
}

void JPJavaFrame::ReleaseDoubleArrayElements(jdoubleArray array, jdouble* v, jint mode)
{
	JAVA_CHECK("JPJavaFrame::ReleaseDoubleArrayElements",
			m_Env->ReleaseDoubleArrayElements(array, v, mode));
}

jcharArray JPJavaFrame::NewCharArray(jsize len)
{
	JAVA_RETURN_OBJ(jcharArray, "JPJavaFrame::NewCharArray",
			m_Env->NewCharArray(len));
}

void JPJavaFrame::SetCharArrayRegion(jcharArray array, jsize start, jsize len, jchar* vals)
{
	JAVA_CHECK("JPJavaFrame::SetCharArrayRegion",
			m_Env->SetCharArrayRegion(array, start, len, vals));
}

void JPJavaFrame::GetCharArrayRegion(jcharArray array, jsize start, jsize len, jchar* vals)
{
	JAVA_CHECK("JPJavaFrame::GetCharArrayRegion",
			m_Env->GetCharArrayRegion(array, start, len, vals));
}

jchar* JPJavaFrame::GetCharArrayElements(jcharArray array, jboolean* isCopy)
{
	JAVA_RETURN(jchar*, "JPJavaFrame::GetCharArrayElements",
			m_Env->GetCharArrayElements(array, isCopy));
}

void JPJavaFrame::ReleaseCharArrayElements(jcharArray array, jchar* v, jint mode)
{
	JAVA_CHECK("JPJavaFrame::ReleaseCharArrayElements",
			m_Env->ReleaseCharArrayElements(array, v, mode));
}

jbooleanArray JPJavaFrame::NewBooleanArray(jsize len)
{
	JAVA_RETURN_OBJ(jbooleanArray, "JPJavaFrame::NewBooleanArray",
			m_Env->NewBooleanArray(len));
}

void JPJavaFrame::SetBooleanArrayRegion(jbooleanArray array, jsize start, jsize len, jboolean* vals)
{
	JAVA_CHECK("JPJavaFrame::SetBooleanArrayRegion",
			m_Env->SetBooleanArrayRegion(array, start, len, vals));
}

void JPJavaFrame::GetBooleanArrayRegion(jbooleanArray array, jsize start, jsize len, jboolean* vals)
{
	JAVA_CHECK("JPJavaFrame::GetBooleanArrayRegion",
			m_Env->GetBooleanArrayRegion(array, start, len, vals));
}

jboolean* JPJavaFrame::GetBooleanArrayElements(jbooleanArray array, jboolean* isCopy)
{
	JAVA_RETURN(jboolean*, "JPJavaFrame::GetBooleanArrayElements",
			m_Env->GetBooleanArrayElements(array, isCopy));
}

void JPJavaFrame::ReleaseBooleanArrayElements(jbooleanArray array, jboolean* v, jint mode)
{
	JAVA_CHECK("JPJavaFrame::ReleaseBooleanArrayElements",
			m_Env->ReleaseBooleanArrayElements(array, v, mode));
}

int JPJavaFrame::MonitorEnter(jobject a0)
{
	JAVA_RETURN(int, "JPJavaFrame::MonitorEnter",
			m_Env->MonitorEnter(a0));
}

int JPJavaFrame::MonitorExit(jobject a0)
{
	JAVA_RETURN(int, "JPJavaFrame::MonitorExit",
			m_Env->MonitorExit(a0));
}

jmethodID JPJavaFrame::FromReflectedMethod(jobject a0)
{
	JAVA_RETURN(jmethodID, "JPJavaFrame::FromReflectedMethod",
			m_Env->FromReflectedMethod(a0));
}

jfieldID JPJavaFrame::FromReflectedField(jobject a0)
{
	JAVA_RETURN(jfieldID, "JPJavaFrame::FromReflectedField",
			m_Env->FromReflectedField(a0));
}

jclass JPJavaFrame::FindClass(const string& a0)
{
	JAVA_RETURN(jclass, "JPJavaFrame::FindClass",
			m_Env->FindClass(a0.c_str()));
}

jobjectArray JPJavaFrame::NewObjectArray(jsize a0, jclass elementClass, jobject initialElement)
{
	JAVA_RETURN(jobjectArray, "JPJavaFrame::NewObjectArray",
			m_Env->NewObjectArray(a0, elementClass, initialElement));
}

void JPJavaFrame::SetObjectArrayElement(jobjectArray a0, jsize a1, jobject a2)
{
	JAVA_CHECK("JPJavaFrame::SetObjectArrayElement",
			m_Env->SetObjectArrayElement(a0, a1, a2));
}

void JPJavaFrame::CallStaticVoidMethodA(jclass a0, jmethodID a1, jvalue* a2)
{
	JAVA_CHECK("JPJavaFrame::CallStaticVoidMethodA",
			m_Env->CallStaticVoidMethodA(a0, a1, a2));
}

void JPJavaFrame::CallVoidMethodA(jobject a0, jmethodID a1, jvalue* a2)
{
	JAVA_CHECK("JPJavaFrame::CallVoidMethodA",
			m_Env->CallVoidMethodA(a0, a1, a2));
}

void JPJavaFrame::CallNonvirtualVoidMethodA(jobject a0, jclass a1, jmethodID a2, jvalue* a3)
{
	JAVA_CHECK("JPJavaFrame::CallVoidMethodA",
			m_Env->CallNonvirtualVoidMethodA(a0, a1, a2, a3));
}

jboolean JPJavaFrame::IsInstanceOf(jobject a0, jclass a1)
{
	JAVA_RETURN(jboolean, "JPJavaFrame::IsInstanceOf",
			m_Env->IsInstanceOf(a0, a1));
}

jboolean JPJavaFrame::IsAssignableFrom(jclass a0, jclass a1)
{
	JAVA_RETURN(jboolean, "JPJavaFrame::IsAssignableFrom",
			m_Env->IsAssignableFrom(a0, a1));
}

jstring JPJavaFrame::NewStringUTF(const char* a0)
{
	JAVA_RETURN_OBJ(jstring, "JPJavaFrame::NewString",
			m_Env->NewStringUTF(a0));
}

const char* JPJavaFrame::GetStringUTFChars(jstring a0, jboolean* a1)
{
	JAVA_RETURN(const char*, "JPJavaFrame::GetStringUTFChars",
			m_Env->GetStringUTFChars(a0, a1));
}

void JPJavaFrame::ReleaseStringUTFChars(jstring a0, const char* a1)
{
	JAVA_CHECK("JPJavaFrame::ReleaseStringUTFChars",
			m_Env->ReleaseStringUTFChars(a0, a1));
}

jsize JPJavaFrame::GetArrayLength(jarray a0)
{
	JAVA_RETURN(jsize, "JPJavaFrame::GetArrayLength",
			m_Env->GetArrayLength(a0));
}

jobject JPJavaFrame::GetObjectArrayElement(jobjectArray a0, jsize a1)
{
	JAVA_RETURN_OBJ(jobject, "JPJavaFrame::GetObjectArrayElement",
			m_Env->GetObjectArrayElement(a0, a1));
}

jmethodID JPJavaFrame::GetMethodID(jclass a0, const char* a1, const char* a2)
{
	JAVA_RETURN(jmethodID, "JPJavaFrame::GetMethodID",
			m_Env->GetMethodID(a0, a1, a2));
}

jmethodID JPJavaFrame::GetStaticMethodID(jclass a0, const char* a1, const char* a2)
{
	JAVA_RETURN(jmethodID, "JPJavaFrame::GetStaticMethodID",
			m_Env->GetStaticMethodID(a0, a1, a2));
}

jfieldID JPJavaFrame::GetFieldID(jclass a0, const char* a1, const char* a2)
{
	JAVA_RETURN(jfieldID, "JPJavaFrame::GetFieldID",
			m_Env->GetFieldID(a0, a1, a2));
}

jfieldID JPJavaFrame::GetStaticFieldID(jclass a0, const char* a1, const char* a2)
{
	JAVA_RETURN(jfieldID, "JPJavaFrame::GetStaticFieldID",
			m_Env->GetStaticFieldID(a0, a1, a2));
}

jsize JPJavaFrame::GetStringUTFLength(jstring a0)
{
	JAVA_RETURN(jsize, "JPJavaFrame::GetStringUTFLength",
			m_Env->GetStringUTFLength(a0));
}

jclass JPJavaFrame::DefineClass(const char* a0, jobject a1, const jbyte* a2, jsize a3)
{
	JAVA_RETURN(jclass, "JPJavaFrame::DefineClass",
			m_Env->DefineClass(a0, a1, a2, a3));
}

jint JPJavaFrame::RegisterNatives(jclass a0, const JNINativeMethod* a1, jint a2)
{
	JAVA_RETURN(jint, "JPJavaFrame::RegisterNatives",
			m_Env->RegisterNatives(a0, a1, a2));
}

void* JPJavaFrame::GetDirectBufferAddress(jobject obj)
{
	JAVA_RETURN(void*, "JPJavaFrame::GetDirectBufferAddress",
			m_Env->GetDirectBufferAddress(obj));
}

jlong JPJavaFrame::GetDirectBufferCapacity(jobject obj)
{
	JAVA_RETURN(jlong, "JPJavaFrame::GetDirectBufferAddress",
			m_Env->GetDirectBufferCapacity(obj));
}

jboolean JPJavaFrame::isBufferReadOnly(jobject obj)
{
	return CallBooleanMethodA(obj, getContext()->m_Buffer_IsReadOnlyID, nullptr);
}

jobject JPJavaFrame::asReadOnlyBuffer(jobject obj)
{
	JAVA_RETURN_OBJ(jobject, "JPJavaFrame::asReadOnlyBuffer",
			m_Env->CallObjectMethod(obj, getContext()->m_Buffer_AsReadOnlyID));
}

jboolean JPJavaFrame::orderBuffer(jobject obj)
{
	jvalue arg;
	arg.l = obj;
	JPContext *context = getContext();
	return CallBooleanMethodA(context->m_JavaContext.get(),
			context->m_Context_OrderID, &arg);
}

// GCOVR_EXCL_START
// This is used when debugging reference counting problems.

jclass JPJavaFrame::getClass(jobject obj)
{
	return (jclass) CallObjectMethodA(obj, getContext()->m_Object_GetClassID, nullptr);
}
// GCOVR_EXCL_STOP

class JPStringAccessor
{
	JPJavaFrame& frame_;
	jboolean isCopy;

public:
	const char* cstr;
	int length;
	jstring jstr_;

	JPStringAccessor(JPJavaFrame& frame, jstring jstr)
	: frame_(frame), jstr_(jstr)
	{
		cstr = frame_.GetStringUTFChars(jstr, &isCopy);
		length = frame_.GetStringUTFLength(jstr);
	}

	~JPStringAccessor()
	{
		try
		{
			frame_.ReleaseStringUTFChars(jstr_, cstr);
		}		catch (JPypeException&)
		{
			// Error during release must be eaten.
			// If Java does not accept a release of the buffer it
			// is Java's issue, not ours.
		}
	}
} ;

string JPJavaFrame::toString(jobject o)
{
	auto str = (jstring) CallObjectMethodA(o, getContext()->m_Object_ToStringID, nullptr);
	if (str == nullptr)
		return "null";
	return toStringUTF8(str);
}

string JPJavaFrame::toStringUTF8(jstring str)
{
	JPStringAccessor contents(*this, str);
#ifdef ANDROID
	return string(contents.cstr, contents.length);
#else
	return transcribe(contents.cstr, contents.length, JPEncodingJavaUTF8(), JPEncodingUTF8());
#endif
}

jstring JPJavaFrame::fromStringUTF8(const string& str)
{
#ifdef ANDROID
	return (jstring) NewStringUTF(str.c_str());
#else
	string mstr = transcribe(str.c_str(), str.size(), JPEncodingUTF8(), JPEncodingJavaUTF8());
	return (jstring) NewStringUTF(mstr.c_str());
#endif
}

jobject JPJavaFrame::toCharArray(jstring jstr)
{
	return CallObjectMethodA(jstr, getContext()->m_String_ToCharArrayID, nullptr);
}

bool JPJavaFrame::equals(jobject o1, jobject o2 )
{
	jvalue args;
	args.l = o2;
	return CallBooleanMethodA(o1, getContext()->m_Object_EqualsID, &args) != 0;
}

jint JPJavaFrame::hashCode(jobject o)
{
	return CallIntMethodA(o, getContext()->m_Object_HashCodeID, nullptr);
}

jobject JPJavaFrame::collectRectangular(jarray obj)
{
	JPContext* context = getContext();
	if (context->m_Context_collectRectangularID == nullptr)
		return nullptr;
	jvalue v;
	v.l = (jobject) obj;
	JAVA_RETURN(jobject, "JPJavaFrame::collectRectangular",
			CallObjectMethodA(
			context->m_JavaContext.get(),
			context->m_Context_collectRectangularID, &v));
}

jobject JPJavaFrame::assemble(jobject dims, jobject parts)
{
	JPContext* context = getContext();
	if (context->m_Context_collectRectangularID == nullptr)
		return nullptr;
	jvalue v[2];
	v[0].l = (jobject) dims;
	v[1].l = (jobject) parts;
	JAVA_RETURN(jobject, "JPJavaFrame::assemble",
			CallObjectMethodA(
			context->m_JavaContext.get(),
			context->m_Context_assembleID, v));
}

jobject JPJavaFrame::newArrayInstance(jclass c, jintArray dims)
{
	JPContext* context = getContext();
	jvalue v[2];
	v[0].l = (jobject) c;
	v[1].l = (jobject) dims;
	JAVA_RETURN(jobject, "JPJavaFrame::newArrayInstance",
			CallStaticObjectMethodA(
			context->m_Array.get(),
			context->m_Array_NewInstanceID, v));
}

jobject JPJavaFrame::callMethod(jobject method, jobject obj, jobject args)
{
	JP_TRACE_IN("JPJavaFrame::callMethod");
	JPContext* context = getContext();
	if (context->m_CallMethodID == nullptr)
		return nullptr;
	JPJavaFrame frame(*this);
	jvalue v[3];
	v[0].l = method;
	v[1].l = obj;
	v[2].l = args;
	return frame.keep(frame.CallObjectMethodA(context->m_Reflector.get(), context->m_CallMethodID, v));
	JP_TRACE_OUT;
}

string JPJavaFrame::getFunctional(jclass c)
{
	JPContext* context = getContext();
	jvalue v;
	v.l = (jobject) c;
	return toStringUTF8((jstring) CallStaticObjectMethodA(
			context->m_ContextClass.get(),
			context->m_Context_GetFunctionalID, &v));
}

JPClass *JPJavaFrame::findClass(jclass obj)
{
	return getContext()->getTypeManager()->findClass(obj);
}

JPClass *JPJavaFrame::findClassByName(const string& name)
{
	return getContext()->getTypeManager()->findClassByName(name);
}

JPClass *JPJavaFrame::findClassForObject(jobject obj)
{
	return getContext()->getTypeManager()->findClassForObject(obj);
}

jint JPJavaFrame::compareTo(jobject obj, jobject obj2)
{
	jvalue v;
	v.l = obj2;
	jint ret = m_Env->CallIntMethodA(obj, getContext()->m_CompareToID, &v);
	if (m_Env->ExceptionOccurred())
	{
		m_Env->ExceptionClear();
		JP_RAISE(PyExc_TypeError, "Unable to compare")
	}
	return ret;
}

jthrowable JPJavaFrame::getCause(jthrowable th)
{
	return (jthrowable) CallObjectMethodA((jobject) th, getContext()->m_Throwable_GetCauseID, nullptr);
}

jstring JPJavaFrame::getMessage(jthrowable th)
{
	return (jstring) CallObjectMethodA((jobject) th, getContext()->m_Throwable_GetMessageID, nullptr);
}

jboolean JPJavaFrame::isPackage(const string& str)
{
	JPContext* context = getContext();
	jvalue v;
	v.l = fromStringUTF8(str);
	JAVA_RETURN(jboolean, "JPJavaFrame::isPackage",
			CallBooleanMethodA(context->m_JavaContext.get(), context->m_Context_IsPackageID, &v));
}

jobject JPJavaFrame::getPackage(const string& str)
{
	JPContext* context = getContext();
	jvalue v;
	v.l = fromStringUTF8(str);
	JAVA_RETURN(jobject, "JPJavaFrame::getPackage",
			CallObjectMethodA(context->m_JavaContext.get(), context->m_Context_GetPackageID, &v));
}

jobject JPJavaFrame::getPackageObject(jobject pkg, const string& str)
{
	jvalue v;
	v.l = fromStringUTF8(str);
	JAVA_RETURN(jobject, "JPJavaFrame::getPackageObject",
			CallObjectMethodA(pkg, getContext()->m_Package_GetObjectID, &v));
}

jarray JPJavaFrame::getPackageContents(jobject pkg)
{
	jvalue v;
	JAVA_RETURN(auto, "JPJavaFrame::getPackageContents",
			(jarray) CallObjectMethodA(pkg, getContext()->m_Package_GetContentsID, &v));
}

void JPJavaFrame::newWrapper(JPClass* cls)
{
	JPContext* context = getContext();
	JPPyCallRelease call;
	jvalue jv;
	jv.j = (jlong) cls;
	CallVoidMethodA(context->getJavaContext(),
			context->m_Context_NewWrapperID, &jv);
}

void JPJavaFrame::registerRef(jobject obj, PyObject* hostRef)
{
	JPReferenceQueue::registerRef(*this, obj, hostRef);
}

void JPJavaFrame::registerRef(jobject obj, void* ref, JCleanupHook cleanup)
{
	JPReferenceQueue::registerRef(*this, obj, ref, cleanup);
}

void JPJavaFrame::clearInterrupt(bool throws)
{
	JPContext* context = getContext();
	JPPyCallRelease call;
	jvalue jv;
	jv.z = throws;
	CallVoidMethodA(context->m_ContextClass.get(),
			context->m_Context_ClearInterruptID, &jv);
}
