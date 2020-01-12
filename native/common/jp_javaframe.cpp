/*****************************************************************************
   Copyright 2004-2008 Steve Ménard

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

#define USE_JNI_VERSION JNI_VERSION_1_4

namespace
{

/*****************************************************************************/
// Helper classes

/** Call a java jni function with error checking.
 *
 * This version should be used when the java call is a fixed length of time
 * to complete.
 */
class JPCall
{
	JNIEnv* _env;
	JPContext* _context;
	const char* _msg;
public:

	JPCall(const JPJavaFrame& frame, const char* msg)
	{
		_env = frame.getEnv();
		_context = frame.getContext();
		_msg = msg;
	}

	void check()
	{
		if (_env && _env->functions->ExceptionCheck(_env) == JNI_TRUE)
		{
			JPJavaFrame frame(_context, _env);
			jthrowable th = _env->functions->ExceptionOccurred(_env);
			_env->functions->ExceptionClear(_env);
			_env = 0;
			throw JPypeException(frame, th, _msg, JP_STACKINFO());
		}
	}

	~JPCall() NO_EXCEPT_FALSE
	{
		// This is a throw in destructor which is only allowed on an exception safe code pattern
		if (_env != NULL && _env->functions->ExceptionCheck(_env) == JNI_TRUE)
		{
			JPJavaFrame frame(_context, _env);
			jthrowable th = _env->functions->ExceptionOccurred(_env);
			_env->functions->ExceptionClear(_env);
			_env = 0;
			throw JPypeException(frame, th, _msg, JP_STACKINFO());
		}
	}
} ;

} // default namespace

/*****************************************************************************/
// Local frames represent the JNIEnv for memory handling all java
// resources should be created within them.
//

JPJavaFrame::JPJavaFrame(JPContext* context, JNIEnv* p_env, int i)
: m_Context(context), m_Env(p_env), popped(false)
{
	// Create a memory management frame to live in
	m_Env->functions->PushLocalFrame(m_Env, i);
}

JPJavaFrame::JPJavaFrame(JPContext* context, int i)
: m_Context(context), popped(false)
{
	ASSERT_JVM_RUNNING(context);
	m_Env = context->getEnv();

	// Create a memory management frame to live in
	m_Env->functions->PushLocalFrame(m_Env, i);
}

JPJavaFrame::JPJavaFrame(const JPJavaFrame& frame)
: m_Context(frame.m_Context), m_Env(frame.m_Env), popped(false)
{
	// Create a memory management frame to live in
	m_Env->functions->PushLocalFrame(m_Env, LOCAL_FRAME_DEFAULT);
}

jobject JPJavaFrame::keep(jobject obj)
{
	popped = true;
	return m_Env->functions->PopLocalFrame(m_Env, obj);
}

JPJavaFrame::~JPJavaFrame()
{
	// Check if we have already closed the frame.
	if (!popped)
	{
		m_Env->functions->PopLocalFrame(m_Env, NULL);
	}

	// It is not safe to detach as we would loss all local references including
	// any we want to keep.
}

void JPJavaFrame::DeleteLocalRef(jobject obj)
{
	m_Env->functions->DeleteLocalRef(m_Env, obj);
}

void JPJavaFrame::DeleteGlobalRef(jobject obj)
{
	m_Env->functions->DeleteGlobalRef(m_Env, obj);
}

jobject JPJavaFrame::NewLocalRef(jobject a0)
{
	jobject res = m_Env->functions->NewLocalRef(m_Env, a0);
	return res;
}

jobject JPJavaFrame::NewGlobalRef(jobject a0)
{
	JP_TRACE_IN("JPJavaFrame::NewGlobalRef", a0);
	jobject res = m_Env->functions->NewGlobalRef(m_Env, a0);
	JP_TRACE("got", res);
	return res;
	JP_TRACE_OUT;
}

/*****************************************************************************/
// Exceptions

bool JPJavaFrame::ExceptionCheck()
{
	return (m_Env->functions->ExceptionCheck(m_Env) ? true : false);
}

void JPJavaFrame::ExceptionDescribe()
{
	m_Env->functions->ExceptionDescribe(m_Env);
}

void JPJavaFrame::ExceptionClear()
{
	m_Env->functions->ExceptionClear(m_Env);
}

jint JPJavaFrame::ThrowNew(jclass clazz, const char* msg)
{
	return m_Env->functions->ThrowNew(m_Env, clazz, msg);
}

jint JPJavaFrame::Throw(jthrowable th)
{
	return m_Env->functions->Throw(m_Env, th);
}

jthrowable JPJavaFrame::ExceptionOccurred()
{
	jthrowable res;

	res = m_Env->functions->ExceptionOccurred(m_Env);
#ifdef JP_TRACING_ENABLE
	if (res)
	{
		m_Env->functions->ExceptionDescribe(m_Env);
	}
#endif
	return res;
}

/*****************************************************************************/

#define JAVA_CHECK(msg)  if (ExceptionCheck()) { RAISE(JPJavaException, msg); }

jobject JPJavaFrame::NewObjectA(jclass a0, jmethodID a1, jvalue* a2)
{
	jobject res;
	JPCall call(*this, "NewObjectA");

	// Allocate the object
	res = m_Env->functions->AllocObject(m_Env, a0);
	call.check();

	// Initialize the object
	m_Env->functions->CallVoidMethodA(m_Env, res, a1, a2);
	return res;
}

jobject JPJavaFrame::NewObject(jclass a0, jmethodID a1)
{
	jobject res;
	JPCall call(*this, "NewObject");

	// Allocate the object
	res = m_Env->functions->AllocObject(m_Env, a0);
	call.check();

	// Initialize the object
	m_Env->functions->CallVoidMethod(m_Env, res, a1);
	return res;
}

jobject JPJavaFrame::NewDirectByteBuffer(void* address, jlong capacity)
{
	JPCall call(*this, "NewDirectByteBuffer");
	return m_Env->functions->NewDirectByteBuffer(m_Env, address, capacity);
}

void* JPJavaFrame::GetPrimitiveArrayCritical(jarray array, jboolean *isCopy)
{
	JPCall call(*this, "GetPrimitiveArrayCritical");
	return m_Env->functions->GetPrimitiveArrayCritical(m_Env, array, isCopy);
}

void JPJavaFrame::ReleasePrimitiveArrayCritical(jarray array, void *carray, jint mode)
{
	JPCall call(*this, "ReleasePrimitiveArrayCritical");
	m_Env->functions->ReleasePrimitiveArrayCritical(m_Env, array, carray, mode);
}

/*****************************************************************************/

jbyte JPJavaFrame::GetStaticByteField(jclass clazz, jfieldID fid)
{
	JPCall call(*this, "GetStaticByteField");
	return m_Env->functions->GetStaticByteField(m_Env, clazz, fid);
}

jbyte JPJavaFrame::GetByteField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetByteField");
	return m_Env->functions->GetByteField(m_Env, clazz, fid);
}

void JPJavaFrame::SetStaticByteField(jclass clazz, jfieldID fid, jbyte val)
{
	JPCall call(*this, "SetStaticByteField");
	m_Env->functions->SetStaticByteField(m_Env, clazz, fid, val);
}

void JPJavaFrame::SetByteField(jobject clazz, jfieldID fid, jbyte val)
{
	JPCall call(*this, "SetByteField");
	m_Env->functions->SetByteField(m_Env, clazz, fid, val);
}

jbyte JPJavaFrame::CallStaticByteMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallStaticByteMethodA");
	return m_Env->functions->CallStaticByteMethodA(m_Env, clazz, mid, val);
}

jbyte JPJavaFrame::CallByteMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallByteMethodA");
	return m_Env->functions->CallByteMethodA(m_Env, obj, mid, val);
}

jbyte JPJavaFrame::CallNonvirtualByteMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallNonvirtualByteMethodA");
	return m_Env->functions->CallNonvirtualByteMethodA(m_Env, obj, claz, mid, val);
}

jshort JPJavaFrame::GetStaticShortField(jclass clazz, jfieldID fid)
{
	JPCall call(*this, "GetStaticShortField");
	return m_Env->functions->GetStaticShortField(m_Env, clazz, fid);
}

jshort JPJavaFrame::GetShortField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetShortField");
	return m_Env->functions->GetShortField(m_Env, clazz, fid);
}

void JPJavaFrame::SetStaticShortField(jclass clazz, jfieldID fid, jshort val)
{
	JPCall call(*this, "SetStaticShortField");
	m_Env->functions->SetStaticShortField(m_Env, clazz, fid, val);
}

void JPJavaFrame::SetShortField(jobject clazz, jfieldID fid, jshort val)
{
	JPCall call(*this, "SetShortField");
	m_Env->functions->SetShortField(m_Env, clazz, fid, val);
}

jshort JPJavaFrame::CallStaticShortMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallStaticShortMethodA");
	return m_Env->functions->CallStaticShortMethodA(m_Env, clazz, mid, val);
}

jshort JPJavaFrame::CallShortMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallShortMethodA");
	return m_Env->functions->CallShortMethodA(m_Env, obj, mid, val);
}

jshort JPJavaFrame::CallNonvirtualShortMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallNonvirtualShortMethodA");
	return m_Env->functions->CallNonvirtualShortMethodA(m_Env, obj, claz, mid, val);
}

jint JPJavaFrame::GetStaticIntField(jclass clazz, jfieldID fid)
{
	JPCall call(*this, "GetStaticIntField");
	return m_Env->functions->GetStaticIntField(m_Env, clazz, fid);
}

jint JPJavaFrame::GetIntField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetIntField");
	return m_Env->functions->GetIntField(m_Env, clazz, fid);
}

void JPJavaFrame::SetStaticIntField(jclass clazz, jfieldID fid, jint val)
{
	JPCall call(*this, "SetStaticIntField");
	m_Env->functions->SetStaticIntField(m_Env, clazz, fid, val);
}

void JPJavaFrame::SetIntField(jobject clazz, jfieldID fid, jint val)
{
	JPCall call(*this, "SetIntField");
	m_Env->functions->SetIntField(m_Env, clazz, fid, val);
}

jint JPJavaFrame::CallStaticIntMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallStaticIntMethodA");
	return m_Env->functions->CallStaticIntMethodA(m_Env, clazz, mid, val);
}

jint JPJavaFrame::CallIntMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallIntMethodA");
	return m_Env->functions->CallIntMethodA(m_Env, obj, mid, val);
}

jint JPJavaFrame::CallNonvirtualIntMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallNonvirtualIntMethodA");
	return m_Env->functions->CallNonvirtualIntMethodA(m_Env, obj, claz, mid, val);
}

jlong JPJavaFrame::GetStaticLongField(jclass clazz, jfieldID fid)
{
	JPCall call(*this, "GetStaticLongField");
	return m_Env->functions->GetStaticLongField(m_Env, clazz, fid);
}

jlong JPJavaFrame::GetLongField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetLongField");
	return m_Env->functions->GetLongField(m_Env, clazz, fid);
}

void JPJavaFrame::SetStaticLongField(jclass clazz, jfieldID fid, jlong val)
{
	JPCall call(*this, "SetStaticLongField");
	m_Env->functions->SetStaticLongField(m_Env, clazz, fid, val);
}

void JPJavaFrame::SetLongField(jobject clazz, jfieldID fid, jlong val)
{
	JPCall call(*this, "SetLongField");
	m_Env->functions->SetLongField(m_Env, clazz, fid, val);
}

jlong JPJavaFrame::CallStaticLongMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallStaticLongMethodA");
	return m_Env->functions->CallStaticLongMethodA(m_Env, clazz, mid, val);
}

jlong JPJavaFrame::CallLongMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallLongMethodA");
	return m_Env->functions->CallLongMethodA(m_Env, obj, mid, val);
}

jlong JPJavaFrame::CallNonvirtualLongMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallNonvirtualLongMethodA");
	return m_Env->functions->CallNonvirtualLongMethodA(m_Env, obj, claz, mid, val);
}

jfloat JPJavaFrame::GetStaticFloatField(jclass clazz, jfieldID fid)
{
	JPCall call(*this, "GetStaticFloatField");
	return m_Env->functions->GetStaticFloatField(m_Env, clazz, fid);
}

jfloat JPJavaFrame::GetFloatField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetFloatField");
	return m_Env->functions->GetFloatField(m_Env, clazz, fid);
}

void JPJavaFrame::SetStaticFloatField(jclass clazz, jfieldID fid, jfloat val)
{
	JPCall call(*this, "SetStaticFloatField");
	m_Env->functions->SetStaticFloatField(m_Env, clazz, fid, val);
}

void JPJavaFrame::SetFloatField(jobject clazz, jfieldID fid, jfloat val)
{
	JPCall call(*this, "SetFloatField");
	m_Env->functions->SetFloatField(m_Env, clazz, fid, val);
}

jfloat JPJavaFrame::CallStaticFloatMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallStaticFloatMethodA");
	return m_Env->functions->CallStaticFloatMethodA(m_Env, clazz, mid, val);
}

jfloat JPJavaFrame::CallFloatMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallFloatMethodA");
	return m_Env->functions->CallFloatMethodA(m_Env, obj, mid, val);
}

jfloat JPJavaFrame::CallNonvirtualFloatMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallNonvirtualFloatMethodA");
	return m_Env->functions->CallNonvirtualFloatMethodA(m_Env, obj, claz, mid, val);
}

jdouble JPJavaFrame::GetStaticDoubleField(jclass clazz, jfieldID fid)
{
	JPCall call(*this, "GetStaticDoubleField");
	return m_Env->functions->GetStaticDoubleField(m_Env, clazz, fid);
}

jdouble JPJavaFrame::GetDoubleField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetDoubleField");
	return m_Env->functions->GetDoubleField(m_Env, clazz, fid);
}

void JPJavaFrame::SetStaticDoubleField(jclass clazz, jfieldID fid, jdouble val)
{
	JPCall call(*this, "SetStaticDoubleField");
	m_Env->functions->SetStaticDoubleField(m_Env, clazz, fid, val);
}

void JPJavaFrame::SetDoubleField(jobject clazz, jfieldID fid, jdouble val)
{
	JPCall call(*this, "SetDoubleField");
	m_Env->functions->SetDoubleField(m_Env, clazz, fid, val);
}

jdouble JPJavaFrame::CallStaticDoubleMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallStaticDoubleMethodA");
	return m_Env->functions->CallStaticDoubleMethodA(m_Env, clazz, mid, val);
}

jdouble JPJavaFrame::CallDoubleMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallDoubleMethodA");
	return m_Env->functions->CallDoubleMethodA(m_Env, obj, mid, val);
}

jdouble JPJavaFrame::CallNonvirtualDoubleMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallNonvirtualDoubleMethodA");
	return m_Env->functions->CallNonvirtualDoubleMethodA(m_Env, obj, claz, mid, val);
}

jchar JPJavaFrame::GetStaticCharField(jclass clazz, jfieldID fid)
{
	JPCall call(*this, "GetStaticCharField");
	return m_Env->functions->GetStaticCharField(m_Env, clazz, fid);
}

jchar JPJavaFrame::GetCharField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetCharField");
	return m_Env->functions->GetCharField(m_Env, clazz, fid);
}

void JPJavaFrame::SetStaticCharField(jclass clazz, jfieldID fid, jchar val)
{
	JPCall call(*this, "SetStaticCharField");
	m_Env->functions->SetStaticCharField(m_Env, clazz, fid, val);
}

void JPJavaFrame::SetCharField(jobject clazz, jfieldID fid, jchar val)
{
	JPCall call(*this, "SetCharField");
	m_Env->functions->SetCharField(m_Env, clazz, fid, val);
}

jchar JPJavaFrame::CallStaticCharMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallStaticCharMethodA");
	return m_Env->functions->CallStaticCharMethodA(m_Env, clazz, mid, val);
}

jchar JPJavaFrame::CallCharMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallCharMethodA");
	return m_Env->functions->CallCharMethodA(m_Env, obj, mid, val);
}

jchar JPJavaFrame::CallNonvirtualCharMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallNonvirtualCharMethodA");
	return m_Env->functions->CallNonvirtualCharMethodA(m_Env, obj, claz, mid, val);
}

jboolean JPJavaFrame::GetStaticBooleanField(jclass clazz, jfieldID fid)
{
	JPCall call(*this, "GetStaticBooleanField");
	return m_Env->functions->GetStaticBooleanField(m_Env, clazz, fid);
}

jboolean JPJavaFrame::GetBooleanField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetBooleanField");
	return m_Env->functions->GetBooleanField(m_Env, clazz, fid);
}

void JPJavaFrame::SetStaticBooleanField(jclass clazz, jfieldID fid, jboolean val)
{
	JPCall call(*this, "SetStaticBooleanField");
	m_Env->functions->SetStaticBooleanField(m_Env, clazz, fid, val);
}

void JPJavaFrame::SetBooleanField(jobject clazz, jfieldID fid, jboolean val)
{
	JPCall call(*this, "SetBooleanField");
	m_Env->functions->SetBooleanField(m_Env, clazz, fid, val);
}

jboolean JPJavaFrame::CallStaticBooleanMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallStaticBooleanMethodA");
	return m_Env->functions->CallStaticBooleanMethodA(m_Env, clazz, mid, val);
}

jboolean JPJavaFrame::CallBooleanMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallBooleanMethodA");
	return m_Env->functions->CallBooleanMethodA(m_Env, obj, mid, val);
}

jboolean JPJavaFrame::CallNonvirtualBooleanMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallNonvirtualBooleanMethodA");
	return m_Env->functions->CallNonvirtualBooleanMethodA(m_Env, obj, claz, mid, val);
}

jobject JPJavaFrame::GetStaticObjectField(jclass clazz, jfieldID fid)
{
	JPCall call(*this, "GetStaticObjectField");
	return m_Env->functions->GetStaticObjectField(m_Env, clazz, fid);
}

jobject JPJavaFrame::GetObjectField(jobject clazz, jfieldID fid)
{
	JPCall call(*this, "GetObjectField");
	return m_Env->functions->GetObjectField(m_Env, clazz, fid);
}

void JPJavaFrame::SetStaticObjectField(jclass clazz, jfieldID fid, jobject val)
{
	JPCall call(*this, "SetStaticObjectField");
	m_Env->functions->SetStaticObjectField(m_Env, clazz, fid, val);
}

void JPJavaFrame::SetObjectField(jobject clazz, jfieldID fid, jobject val)
{
	JPCall call(*this, "SetObjectField");
	m_Env->functions->SetObjectField(m_Env, clazz, fid, val);
}

jobject JPJavaFrame::CallStaticObjectMethodA(jclass clazz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallStaticObjectMethodA");
	return m_Env->functions->CallStaticObjectMethodA(m_Env, clazz, mid, val);
}

jobject JPJavaFrame::CallObjectMethodA(jobject obj, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallObjectMethodA");
	return m_Env->functions->CallObjectMethodA(m_Env, obj, mid, val);
}

jobject JPJavaFrame::CallNonvirtualObjectMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val)
{
	JPCall call(*this, "CallNonvirtualObjectMethodA");
	return m_Env->functions->CallNonvirtualObjectMethodA(m_Env, obj, claz, mid, val);
}

jbyteArray JPJavaFrame::NewByteArray(jsize len)
{
	JPCall call(*this, "NewByteArray");
	return m_Env->functions->NewByteArray(m_Env, len);
}

void JPJavaFrame::SetByteArrayRegion(jbyteArray array, jsize start, jsize len, jbyte* vals)
{
	JPCall call(*this, "SetByteArrayRegion");
	m_Env->functions->SetByteArrayRegion(m_Env, array, start, len, vals);
}

void JPJavaFrame::GetByteArrayRegion(jbyteArray array, jsize start, jsize len, jbyte* vals)
{
	JPCall call(*this, "GetByteArrayRegion");
	m_Env->functions->GetByteArrayRegion(m_Env, array, start, len, vals);
}

jbyte* JPJavaFrame::GetByteArrayElements(jbyteArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetByteArrayElements");
	return m_Env->functions->GetByteArrayElements(m_Env, array, isCopy);
}

void JPJavaFrame::ReleaseByteArrayElements(jbyteArray array, jbyte* v, jint mode)
{
	JPCall call(*this, "ReleaseByteArrayElements");
	m_Env->functions->ReleaseByteArrayElements(m_Env, array, v, mode);
}

jshortArray JPJavaFrame::NewShortArray(jsize len)
{
	JPCall call(*this, "NewShortArray");
	return m_Env->functions->NewShortArray(m_Env, len);
}

void JPJavaFrame::SetShortArrayRegion(jshortArray array, jsize start, jsize len, jshort* vals)
{
	JPCall call(*this, "SetShortArrayRegion");
	m_Env->functions->SetShortArrayRegion(m_Env, array, start, len, vals);
}

void JPJavaFrame::GetShortArrayRegion(jshortArray array, jsize start, jsize len, jshort* vals)
{
	JPCall call(*this, "GetShortArrayRegion");
	m_Env->functions->GetShortArrayRegion(m_Env, array, start, len, vals);
}

jshort* JPJavaFrame::GetShortArrayElements(jshortArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetShortArrayElements");
	return m_Env->functions->GetShortArrayElements(m_Env, array, isCopy);
}

void JPJavaFrame::ReleaseShortArrayElements(jshortArray array, jshort* v, jint mode)
{
	JPCall call(*this, "ReleaseShortArrayElements");
	m_Env->functions->ReleaseShortArrayElements(m_Env, array, v, mode);
}

jintArray JPJavaFrame::NewIntArray(jsize len)
{
	JPCall call(*this, "NewIntArray");
	return m_Env->functions->NewIntArray(m_Env, len);
}

void JPJavaFrame::SetIntArrayRegion(jintArray array, jsize start, jsize len, jint* vals)
{
	JPCall call(*this, "SetIntArrayRegion");
	m_Env->functions->SetIntArrayRegion(m_Env, array, start, len, vals);
}

void JPJavaFrame::GetIntArrayRegion(jintArray array, jsize start, jsize len, jint* vals)
{
	JPCall call(*this, "GetIntArrayRegion");
	m_Env->functions->GetIntArrayRegion(m_Env, array, start, len, vals);
}

jint* JPJavaFrame::GetIntArrayElements(jintArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetIntArrayElements");
	return m_Env->functions->GetIntArrayElements(m_Env, array, isCopy);
}

void JPJavaFrame::ReleaseIntArrayElements(jintArray array, jint* v, jint mode)
{
	JPCall call(*this, "ReleaseIntArrayElements");
	m_Env->functions->ReleaseIntArrayElements(m_Env, array, v, mode);
}

jlongArray JPJavaFrame::NewLongArray(jsize len)
{
	JPCall call(*this, "NewLongArray");
	return m_Env->functions->NewLongArray(m_Env, len);
}

void JPJavaFrame::SetLongArrayRegion(jlongArray array, jsize start, jsize len, jlong* vals)
{
	JPCall call(*this, "SetLongArrayRegion");
	m_Env->functions->SetLongArrayRegion(m_Env, array, start, len, vals);
}

void JPJavaFrame::GetLongArrayRegion(jlongArray array, jsize start, jsize len, jlong* vals)
{
	JPCall call(*this, "GetLongArrayRegion");
	m_Env->functions->GetLongArrayRegion(m_Env, array, start, len, vals);
}

jlong* JPJavaFrame::GetLongArrayElements(jlongArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetLongArrayElements");
	return m_Env->functions->GetLongArrayElements(m_Env, array, isCopy);
}

void JPJavaFrame::ReleaseLongArrayElements(jlongArray array, jlong* v, jint mode)
{
	JPCall call(*this, "ReleaseLongArrayElements");
	m_Env->functions->ReleaseLongArrayElements(m_Env, array, v, mode);
}

jfloatArray JPJavaFrame::NewFloatArray(jsize len)
{
	JPCall call(*this, "NewFloatArray");
	return m_Env->functions->NewFloatArray(m_Env, len);
}

void JPJavaFrame::SetFloatArrayRegion(jfloatArray array, jsize start, jsize len, jfloat* vals)
{
	JPCall call(*this, "SetFloatArrayRegion");
	m_Env->functions->SetFloatArrayRegion(m_Env, array, start, len, vals);
}

void JPJavaFrame::GetFloatArrayRegion(jfloatArray array, jsize start, jsize len, jfloat* vals)
{
	JPCall call(*this, "GetFloatArrayRegion");
	m_Env->functions->GetFloatArrayRegion(m_Env, array, start, len, vals);
}

jfloat* JPJavaFrame::GetFloatArrayElements(jfloatArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetFloatArrayElements");
	return m_Env->functions->GetFloatArrayElements(m_Env, array, isCopy);
}

void JPJavaFrame::ReleaseFloatArrayElements(jfloatArray array, jfloat* v, jint mode)
{
	JPCall call(*this, "ReleaseFloatArrayElements");
	m_Env->functions->ReleaseFloatArrayElements(m_Env, array, v, mode);
}

jdoubleArray JPJavaFrame::NewDoubleArray(jsize len)
{
	JPCall call(*this, "NewDoubleArray");
	return m_Env->functions->NewDoubleArray(m_Env, len);
}

void JPJavaFrame::SetDoubleArrayRegion(jdoubleArray array, jsize start, jsize len, jdouble* vals)
{
	JPCall call(*this, "SetDoubleArrayRegion");
	m_Env->functions->SetDoubleArrayRegion(m_Env, array, start, len, vals);
}

void JPJavaFrame::GetDoubleArrayRegion(jdoubleArray array, jsize start, jsize len, jdouble* vals)
{
	JPCall call(*this, "GetDoubleArrayRegion");
	m_Env->functions->GetDoubleArrayRegion(m_Env, array, start, len, vals);
}

jdouble* JPJavaFrame::GetDoubleArrayElements(jdoubleArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetDoubleArrayElements");
	return m_Env->functions->GetDoubleArrayElements(m_Env, array, isCopy);
}

void JPJavaFrame::ReleaseDoubleArrayElements(jdoubleArray array, jdouble* v, jint mode)
{
	JPCall call(*this, "ReleaseDoubleArrayElements");
	m_Env->functions->ReleaseDoubleArrayElements(m_Env, array, v, mode);
}

jcharArray JPJavaFrame::NewCharArray(jsize len)
{
	JPCall call(*this, "NewCharArray");
	return m_Env->functions->NewCharArray(m_Env, len);
}

void JPJavaFrame::SetCharArrayRegion(jcharArray array, jsize start, jsize len, jchar* vals)
{
	JPCall call(*this, "SetCharArrayRegion");
	m_Env->functions->SetCharArrayRegion(m_Env, array, start, len, vals);
}

void JPJavaFrame::GetCharArrayRegion(jcharArray array, jsize start, jsize len, jchar* vals)
{
	JPCall call(*this, "GetCharArrayRegion");
	m_Env->functions->GetCharArrayRegion(m_Env, array, start, len, vals);
}

jchar* JPJavaFrame::GetCharArrayElements(jcharArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetCharArrayElements");
	return m_Env->functions->GetCharArrayElements(m_Env, array, isCopy);
}

void JPJavaFrame::ReleaseCharArrayElements(jcharArray array, jchar* v, jint mode)
{
	JPCall call(*this, "ReleaseCharArrayElements");
	m_Env->functions->ReleaseCharArrayElements(m_Env, array, v, mode);
}

jbooleanArray JPJavaFrame::NewBooleanArray(jsize len)
{
	JPCall call(*this, "NewBooleanArray");
	return m_Env->functions->NewBooleanArray(m_Env, len);
}

void JPJavaFrame::SetBooleanArrayRegion(jbooleanArray array, jsize start, jsize len, jboolean* vals)
{
	JPCall call(*this, "SetBooleanArrayRegion");
	m_Env->functions->SetBooleanArrayRegion(m_Env, array, start, len, vals);
}

void JPJavaFrame::GetBooleanArrayRegion(jbooleanArray array, jsize start, jsize len, jboolean* vals)
{
	JPCall call(*this, "GetBooleanArrayRegion");
	m_Env->functions->GetBooleanArrayRegion(m_Env, array, start, len, vals);
}

jboolean* JPJavaFrame::GetBooleanArrayElements(jbooleanArray array, jboolean* isCopy)
{
	JPCall call(*this, "GetBooleanArrayElements");
	return m_Env->functions->GetBooleanArrayElements(m_Env, array, isCopy);
}

void JPJavaFrame::ReleaseBooleanArrayElements(jbooleanArray array, jboolean* v, jint mode)
{
	JPCall call(*this, "ReleaseBooleanArrayElements");
	m_Env->functions->ReleaseBooleanArrayElements(m_Env, array, v, mode);
}

int JPJavaFrame::MonitorEnter(jobject a0)
{
	JPCall call(*this, "MonitorEnter");
	return m_Env->functions->MonitorEnter(m_Env, a0);
}

int JPJavaFrame::MonitorExit(jobject a0)
{
	JPCall call(*this, "MonitorExit");
	return m_Env->functions->MonitorExit(m_Env, a0);
}

jmethodID JPJavaFrame::FromReflectedMethod(jobject a0)
{
	JPCall call(*this, "FromReflectedMethod");
	return m_Env->functions->FromReflectedMethod(m_Env, a0);
}

jfieldID JPJavaFrame::FromReflectedField(jobject a0)
{
	JPCall call(*this, "FromReflectedField");
	return m_Env->functions->FromReflectedField(m_Env, a0);
}

jclass JPJavaFrame::FindClass(const string& a0)
{
	JPCall call(*this, "FindClass");
	return m_Env->functions->FindClass(m_Env, a0.c_str());
}

jboolean JPJavaFrame::IsInstanceOf(jobject a0, jclass a1)
{
	JPCall call(*this, "IsInstanceOf");
	return m_Env->functions->IsInstanceOf(m_Env, a0, a1);
}

jobjectArray JPJavaFrame::NewObjectArray(jsize a0, jclass a1, jobject a2)
{
	JPCall call(*this, "NewObjectArray");
	return m_Env->functions->NewObjectArray(m_Env, a0, a1, a2);
}

void JPJavaFrame::SetObjectArrayElement(jobjectArray a0, jsize a1, jobject a2)
{
	JPCall call(*this, "SetObjectArrayElement");
	m_Env->functions->SetObjectArrayElement(m_Env, a0, a1, a2);
}

void JPJavaFrame::CallStaticVoidMethodA(jclass a0, jmethodID a1, jvalue* a2)
{
	JPCall call(*this, "CallStaticVoidMethodA");
	m_Env->functions->CallStaticVoidMethodA(m_Env, a0, a1, a2);
}

void JPJavaFrame::CallVoidMethodA(jobject a0, jmethodID a1, jvalue* a2)
{
	JPCall call(*this, "CallVoidMethodA");
	m_Env->functions->CallVoidMethodA(m_Env, a0, a1, a2);
}

void JPJavaFrame::CallNonvirtualVoidMethodA(jobject a0, jclass a1, jmethodID a2, jvalue* a3)
{
	JPCall call(*this, "CallVoidMethodA");
	m_Env->functions->CallNonvirtualVoidMethodA(m_Env, a0, a1, a2, a3);
}

jboolean JPJavaFrame::IsAssignableFrom(jclass a0, jclass a1)
{
	JPCall call(*this, "IsAssignableFrom");
	return m_Env->functions->IsAssignableFrom(m_Env, a0, a1);
}

jstring JPJavaFrame::NewStringUTF(const char* a0)
{
	JPCall call(*this, "NewString");
	return m_Env->functions->NewStringUTF(m_Env, a0);
}

jclass JPJavaFrame::GetSuperclass(jclass a0)
{
	JPCall call(*this, "GetSuperclass");
	return m_Env->functions->GetSuperclass(m_Env, a0);
}

const char* JPJavaFrame::GetStringUTFChars(jstring a0, jboolean* a1)
{
	JPCall call(*this, "GetStringUTFChars");
	return m_Env->functions->GetStringUTFChars(m_Env, a0, a1);
}

void JPJavaFrame::ReleaseStringUTFChars(jstring a0, const char* a1)
{
	JPCall call(*this, "ReleaseStringUTFChars");
	m_Env->functions->ReleaseStringUTFChars(m_Env, a0, a1);
}

jsize JPJavaFrame::GetArrayLength(jarray a0)
{
	JPCall call(*this, "GetArrayLength");
	return m_Env->functions->GetArrayLength(m_Env, a0);
}

jobject JPJavaFrame::GetObjectArrayElement(jobjectArray a0, jsize a1)
{
	JPCall call(*this, "GetObjectArrayElement");
	return m_Env->functions->GetObjectArrayElement(m_Env, a0, a1);
}

jclass JPJavaFrame::GetObjectClass(jobject a0)
{
	JPCall call(*this, "GetObjectClass");
	return m_Env->functions->GetObjectClass(m_Env, a0);
}

jmethodID JPJavaFrame::GetMethodID(jclass a0, const char* a1, const char* a2)
{
	JPCall call(*this, "GetMethodID");
	return m_Env->functions->GetMethodID(m_Env, a0, a1, a2);
}

jmethodID JPJavaFrame::GetStaticMethodID(jclass a0, const char* a1, const char* a2)
{
	JPCall call(*this, "GetStaticMethodID");
	return m_Env->functions->GetStaticMethodID(m_Env, a0, a1, a2);
}

jfieldID JPJavaFrame::GetFieldID(jclass a0, const char* a1, const char* a2)
{
	JPCall call(*this, "GetFieldID");
	return m_Env->functions->GetFieldID(m_Env, a0, a1, a2);
}

jfieldID JPJavaFrame::GetStaticFieldID(jclass a0, const char* a1, const char* a2)
{
	JPCall call(*this, "GetStaticFieldID");
	return m_Env->functions->GetStaticFieldID(m_Env, a0, a1, a2);
}

const jchar* JPJavaFrame::GetStringChars(jstring a0, jboolean* a1)
{
	JPCall call(*this, "GetStringChars");
	return m_Env->functions->GetStringChars(m_Env, a0, a1);
}

void JPJavaFrame::ReleaseStringChars(jstring a0, const jchar* a1)
{
	JPCall call(*this, "ReleaseStringChars");
	m_Env->functions->ReleaseStringChars(m_Env, a0, a1);
}

jsize JPJavaFrame::GetStringLength(jstring a0)
{
	JPCall call(*this, "GetStringLength");
	return m_Env->functions->GetStringLength(m_Env, a0);
}

jsize JPJavaFrame::GetStringUTFLength(jstring a0)
{
	JPCall call(*this, "GetStringUTFLength");
	return m_Env->functions->GetStringUTFLength(m_Env, a0);
}

jclass JPJavaFrame::DefineClass(const char* a0, jobject a1, const jbyte* a2, jsize a3)
{
	JPCall call(*this, "DefineClass");
	return m_Env->functions->DefineClass(m_Env, a0, a1, a2, a3);
}

jint JPJavaFrame::RegisterNatives(jclass a0, const JNINativeMethod* a1, jint a2)
{
	JPCall call(*this, "RegisterNatives");
	return m_Env->functions->RegisterNatives(m_Env, a0, a1, a2);
}

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
		frame_.ReleaseStringUTFChars(jstr_, cstr);
	}
} ;

string JPJavaFrame::toString(jobject o)
{
	jstring str = (jstring) CallObjectMethodA(o, m_Context->m_Object_ToStringID, 0);
	return toStringUTF8(str);
}

string JPJavaFrame::toStringUTF8(jstring str)
{
	JPStringAccessor contents(*this, str);
	return transcribe(contents.cstr, contents.length, JPEncodingJavaUTF8(), JPEncodingUTF8());
}

jstring JPJavaFrame::fromStringUTF8(const string& str)
{
	string mstr = transcribe(str.c_str(), str.size(), JPEncodingUTF8(), JPEncodingJavaUTF8());
	return (jstring) NewStringUTF(mstr.c_str());
}

jobject JPJavaFrame::callMethod(jobject method, jobject obj, jobject args)
{
	JP_TRACE_IN("JPJavaFrame::callMethod");
	if (m_Context->m_CallMethodID == 0)
		return NULL;
	JPJavaFrame frame(*this);
	jvalue v[3];
	v[0].l = method;
	v[1].l = obj;
	v[2].l = args;
	return frame.keep(frame.CallObjectMethodA(m_Context->m_JavaContext.get(), m_Context->m_CallMethodID, v));
	JP_TRACE_OUT;
}
