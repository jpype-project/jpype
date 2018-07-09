/*****************************************************************************
   Copyright 2004 Steve MÃ©nard

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
#ifndef _JP_JAVA_FRAME_H_
#define _JP_JAVA_FRAME_H_

/** A Java Frame represents a memory managed scope in which
 * java objects can be manipulated.
 *
 * Any resources created within a Java frame will be automatically
 * deallocated at when the frame falls out of scope unless captured
 * by a global reference.
 *
 * This should be used around all entry points from python that 
 * call java code.  Failure may lead to local references not being
 * released.  Methods will large numbers of local references 
 * should allocate a local frame.  At most one local reference 
 * from a local frame can be kept.  Addition must use global referencing.
 *
 * JavaFrames are created to hold a certain number of items in 
 * scope at time.  They will grow automatically if more items are
 * created, but this has overhead.  For most cases the default
 * will be fine.  However, when working with an array in which
 * the number of items in scope can be known in advance, it is
 * good to size the frame appropraitely.
 *
 * A JavaFrame should not be used in a destructor as it can
 * throw. The most common use of JavaFrame is to delete a
 * global reference.  See the ReleaseGlobalReference for 
 * this purpose.
 */
static const int LOCAL_FRAME_DEFAULT = 8;

class JPJavaFrame
{
	JNIEnv* env;
	bool attached;
	bool popped;

public:
	/** Create a new JavaFrame from an existing JNIEnv.
	 *
	 * This is used when a JPype method is called by the java
	 * virtual machine.  In this case we are supplied with a
	 * JNIEnv from the machine to use. This method cannot
	 * fail.
	 *
	 * @param size determines how many objects can be
	 * created in this scope without additional overhead.
	 *
	 */
	explicit JPJavaFrame(JNIEnv* env, int size = LOCAL_FRAME_DEFAULT);

	/** Create a new JavaFrame.
	 *
	 * This method will automatically attach the thread
	 * if it is not already attached.
	 *
	 * @param size determines how many objects can be
	 * created in this scope without additional overhead.
	 *
	 * @throws JPypeException if the jpype cannot
	 * acquire an env handle to work with jvm.
	 */
	JPJavaFrame(int size = LOCAL_FRAME_DEFAULT);

	/** Exit the local scope and clean up all java
	 * objects.  
	 *
	 * Only the one local object passed to outer scope
	 * by the keep method will be kept alive.
	 */
	~JPJavaFrame();

	/** Exit the local frame and keep a local reference to an object 
	 *
	 * This must be called only once when the frame is about to leave 
	 * scope. Any local references other than the one that is kept
	 * are destroyed.  If the next line is not "return", you are using
	 * this incorrectly.
	 *
	 * Further calls to the frame will still suceed as we do not 
	 * check for operation on a closed frame, but is not advised.
	 */
	jobject keep(jobject);

	/** Create a new global reference to a java object.
	 *
	 * This java reference may be held in a class until the object is 
	 * no longer needed.  It should be deleted with DeleteGlobalRef
	 * or ReleaseGlobalRef.
	 */
	jobject NewGlobalRef(jobject obj);

	/** Delete a global reference.
	 */
	void DeleteGlobalRef(jobject obj);

	/** Release a global reference checking for shutdown.
	 *
	 * This should be used in any calls to release resources
	 * from a destructor.  It cannot fail even if the 
	 * JVM is no longer operating.
	 */
	static void ReleaseGlobalRef(jobject obj);

	/** Create a new local reference.
	 *
	 * This is only used when promoting a WeakReference to
	 * a local reference.
	 */
	jobject NewLocalRef(jobject obj);

	/** Prematurely delete a local reference.
	 *
	 * This is used when processing an array to keep
	 * the objects in scope from growing.
	 */
	void DeleteLocalRef(jobject obj);

	JNIEnv* getEnv() const
	{
		return env;
	}

private:

	jint PushLocalFrame(jint);
	jobject PopLocalFrame(jobject);

public:

	bool ExceptionCheck();
	void ExceptionDescribe();
	void ExceptionClear();
	jthrowable ExceptionOccurred();

	jint ThrowNew(jclass clazz, const char* msg);

	jint Throw(jthrowable th);

	jobject NewDirectByteBuffer(void* address, jlong capacity);

	/** NewObjectA */
	jobject NewObjectA(jclass a0, jmethodID a1, jvalue* a2);

	/** NewObject */
	jobject NewObject(jclass a0, jmethodID a1);

	/** get/release primitive arrays critical (direct pointer access) */
	void* GetPrimitiveArrayCritical(jarray array, jboolean *isCopy);
	void ReleasePrimitiveArrayCritical(jarray array, void *carray, jint mode);

	// Monitor
	int MonitorEnter(jobject a0);
	int MonitorExit(jobject a0);

	// Class probe methods
	jclass GetSuperclass(jclass a0);
	jclass GetObjectClass(jobject a0);

	jclass FindClass(const string& a0);
	jclass DefineClass(const char* a0, jobject a1, const jbyte* a2, jsize a3);
	jint RegisterNatives(jclass a0, const JNINativeMethod* a1, jint a2);

	jboolean IsInstanceOf(jobject a0, jclass a1);
	jboolean IsAssignableFrom(jclass a0, jclass a1);

	jsize GetArrayLength(jarray a0);
	jobject GetObjectArrayElement(jobjectArray a0, jsize a1);

	jfieldID FromReflectedField(jobject a0);
	jfieldID GetFieldID(jclass a0, const char* a1, const char* a2);
	jfieldID GetStaticFieldID(jclass a0, const char* a1, const char* a2);

	jmethodID FromReflectedMethod(jobject a0);
	jmethodID GetMethodID(jclass a0, const char* a1, const char* a2);
	jmethodID GetStaticMethodID(jclass a0, const char* a1, const char* a2);

	// Void
	void CallStaticVoidMethodA(jclass a0, jmethodID a1, jvalue* a2);
	void CallVoidMethodA(jobject a0, jmethodID a1, jvalue* a2);
	void CallVoidMethod(jobject a0, jmethodID a1);

	// Bool
	jboolean GetStaticBooleanField(jclass clazz, jfieldID fid);
	jboolean GetBooleanField(jobject clazz, jfieldID fid);
	void SetStaticBooleanField(jclass clazz, jfieldID fid, jboolean val);
	void SetBooleanField(jobject clazz, jfieldID fid, jboolean val);
	jboolean CallStaticBooleanMethodA(jclass clazz, jmethodID mid, jvalue* val);
	jboolean CallStaticBooleanMethod(jclass clazz, jmethodID mid);
	jboolean CallBooleanMethodA(jobject obj, jmethodID mid, jvalue* val);
	jboolean CallBooleanMethod(jobject obj, jmethodID mid);
	jboolean CallNonvirtualBooleanMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
	jboolean CallNonvirtualBooleanMethod(jobject obj, jclass claz, jmethodID mid);
	jbooleanArray NewBooleanArray(jsize len);
	void SetBooleanArrayRegion(jbooleanArray array, jsize start, jsize len, jboolean* vals);
	void GetBooleanArrayRegion(jbooleanArray array, jsize start, jsize len, jboolean* vals);
	jboolean* GetBooleanArrayElements(jbooleanArray array, jboolean* isCopy);
	void ReleaseBooleanArrayElements(jbooleanArray, jboolean* v, jint mode);

	// Byte
	jbyte GetStaticByteField(jclass clazz, jfieldID fid);
	jbyte GetByteField(jobject clazz, jfieldID fid);
	void SetStaticByteField(jclass clazz, jfieldID fid, jbyte val);
	void SetByteField(jobject clazz, jfieldID fid, jbyte val);
	jbyte CallStaticByteMethodA(jclass clazz, jmethodID mid, jvalue* val);
	jbyte CallStaticByteMethod(jclass clazz, jmethodID mid);
	jbyte CallByteMethodA(jobject obj, jmethodID mid, jvalue* val);
	jbyte CallByteMethod(jobject obj, jmethodID mid);
	jbyte CallNonvirtualByteMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
	jbyte CallNonvirtualByteMethod(jobject obj, jclass claz, jmethodID mid);
	jbyteArray NewByteArray(jsize len);
	void SetByteArrayRegion(jbyteArray array, jsize start, jsize len, jbyte* vals);
	void GetByteArrayRegion(jbyteArray array, jsize start, jsize len, jbyte* vals);
	jbyte* GetByteArrayElements(jbyteArray array, jboolean* isCopy);
	void ReleaseByteArrayElements(jbyteArray, jbyte* v, jint mode);

	// Char
	jchar GetStaticCharField(jclass clazz, jfieldID fid);
	jchar GetCharField(jobject clazz, jfieldID fid);
	void SetStaticCharField(jclass clazz, jfieldID fid, jchar val);
	void SetCharField(jobject clazz, jfieldID fid, jchar val);
	jchar CallStaticCharMethodA(jclass clazz, jmethodID mid, jvalue* val);
	jchar CallStaticCharMethod(jclass clazz, jmethodID mid);
	jchar CallCharMethodA(jobject obj, jmethodID mid, jvalue* val);
	jchar CallCharMethod(jobject obj, jmethodID mid);
	jchar CallNonvirtualCharMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
	jchar CallNonvirtualCharMethod(jobject obj, jclass claz, jmethodID mid);
	jcharArray NewCharArray(jsize len);
	void SetCharArrayRegion(jcharArray array, jsize start, jsize len, jchar* vals);
	void GetCharArrayRegion(jcharArray array, jsize start, jsize len, jchar* vals);
	jchar* GetCharArrayElements(jcharArray array, jboolean* isCopy);
	void ReleaseCharArrayElements(jcharArray, jchar* v, jint mode);

	// Short
	jshort GetStaticShortField(jclass clazz, jfieldID fid);
	jshort GetShortField(jobject clazz, jfieldID fid);
	void SetStaticShortField(jclass clazz, jfieldID fid, jshort val);
	void SetShortField(jobject clazz, jfieldID fid, jshort val);
	jshort CallStaticShortMethodA(jclass clazz, jmethodID mid, jvalue* val);
	jshort CallStaticShortMethod(jclass clazz, jmethodID mid);
	jshort CallShortMethodA(jobject obj, jmethodID mid, jvalue* val);
	jshort CallShortMethod(jobject obj, jmethodID mid);
	jshort CallNonvirtualShortMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
	jshort CallNonvirtualShortMethod(jobject obj, jclass claz, jmethodID mid);
	jshortArray NewShortArray(jsize len);
	void SetShortArrayRegion(jshortArray array, jsize start, jsize len, jshort* vals);
	void GetShortArrayRegion(jshortArray array, jsize start, jsize len, jshort* vals);
	jshort* GetShortArrayElements(jshortArray array, jboolean* isCopy);
	void ReleaseShortArrayElements(jshortArray, jshort* v, jint mode);

	// Integer
	jint GetStaticIntField(jclass clazz, jfieldID fid);
	jint GetIntField(jobject clazz, jfieldID fid);
	void SetStaticIntField(jclass clazz, jfieldID fid, jint val);
	void SetIntField(jobject clazz, jfieldID fid, jint val);
	jint CallStaticIntMethodA(jclass clazz, jmethodID mid, jvalue* val);
	jint CallStaticIntMethod(jclass clazz, jmethodID mid);
	jint CallIntMethodA(jobject obj, jmethodID mid, jvalue* val);
	jint CallIntMethod(jobject obj, jmethodID mid);
	jint CallNonvirtualIntMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
	jint CallNonvirtualIntMethod(jobject obj, jclass claz, jmethodID mid);
	jintArray NewIntArray(jsize len);
	void SetIntArrayRegion(jintArray array, jsize start, jsize len, jint* vals);
	void GetIntArrayRegion(jintArray array, jsize start, jsize len, jint* vals);
	jint* GetIntArrayElements(jintArray array, jboolean* isCopy);
	void ReleaseIntArrayElements(jintArray, jint* v, jint mode);

	// Long
	jlong GetStaticLongField(jclass clazz, jfieldID fid);
	jlong GetLongField(jobject clazz, jfieldID fid);
	void SetStaticLongField(jclass clazz, jfieldID fid, jlong val);
	void SetLongField(jobject clazz, jfieldID fid, jlong val);
	jlong CallStaticLongMethodA(jclass clazz, jmethodID mid, jvalue* val);
	jlong CallStaticLongMethod(jclass clazz, jmethodID mid);
	jlong CallLongMethodA(jobject obj, jmethodID mid, jvalue* val);
	jlong CallLongMethod(jobject obj, jmethodID mid);
	jlong CallNonvirtualLongMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
	jlong CallNonvirtualLongMethod(jobject obj, jclass claz, jmethodID mid);
	jfloat GetStaticFloatField(jclass clazz, jfieldID fid);
	jlongArray NewLongArray(jsize len);
	void SetLongArrayRegion(jlongArray array, jsize start, jsize len, jlong* vals);
	void GetLongArrayRegion(jlongArray array, jsize start, jsize len, jlong* vals);
	jlong* GetLongArrayElements(jlongArray array, jboolean* isCopy);
	void ReleaseLongArrayElements(jlongArray, jlong* v, jint mode);

	// Float
	jfloat GetFloatField(jobject clazz, jfieldID fid);
	void SetStaticFloatField(jclass clazz, jfieldID fid, jfloat val);
	void SetFloatField(jobject clazz, jfieldID fid, jfloat val);
	jfloat CallStaticFloatMethodA(jclass clazz, jmethodID mid, jvalue* val);
	jfloat CallStaticFloatMethod(jclass clazz, jmethodID mid);
	jfloat CallFloatMethodA(jobject obj, jmethodID mid, jvalue* val);
	jfloat CallFloatMethod(jobject obj, jmethodID mid);
	jfloat CallNonvirtualFloatMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
	jfloat CallNonvirtualFloatMethod(jobject obj, jclass claz, jmethodID mid);
	jfloatArray NewFloatArray(jsize len);
	void SetFloatArrayRegion(jfloatArray array, jsize start, jsize len, jfloat* vals);
	void GetFloatArrayRegion(jfloatArray array, jsize start, jsize len, jfloat* vals);
	jfloat* GetFloatArrayElements(jfloatArray array, jboolean* isCopy);
	void ReleaseFloatArrayElements(jfloatArray, jfloat* v, jint mode);

	// Double
	jdouble GetStaticDoubleField(jclass clazz, jfieldID fid);
	jdouble GetDoubleField(jobject clazz, jfieldID fid);
	void SetStaticDoubleField(jclass clazz, jfieldID fid, jdouble val);
	void SetDoubleField(jobject clazz, jfieldID fid, jdouble val);
	jdouble CallStaticDoubleMethodA(jclass clazz, jmethodID mid, jvalue* val);
	jdouble CallStaticDoubleMethod(jclass clazz, jmethodID mid);
	jdouble CallDoubleMethodA(jobject obj, jmethodID mid, jvalue* val);
	jdouble CallDoubleMethod(jobject obj, jmethodID mid);
	jdouble CallNonvirtualDoubleMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
	jdouble CallNonvirtualDoubleMethod(jobject obj, jclass claz, jmethodID mid);
	jdoubleArray NewDoubleArray(jsize len);
	void SetDoubleArrayRegion(jdoubleArray array, jsize start, jsize len, jdouble* vals);
	void GetDoubleArrayRegion(jdoubleArray array, jsize start, jsize len, jdouble* vals);
	jdouble* GetDoubleArrayElements(jdoubleArray array, jboolean* isCopy);
	void ReleaseDoubleArrayElements(jdoubleArray, jdouble* v, jint mode);

	// Object
	jobject GetStaticObjectField(jclass clazz, jfieldID fid);
	jobject GetObjectField(jobject clazz, jfieldID fid);
	void SetStaticObjectField(jclass clazz, jfieldID fid, jobject val);
	void SetObjectField(jobject clazz, jfieldID fid, jobject val);
	jobject CallStaticObjectMethodA(jclass clazz, jmethodID mid, jvalue* val);
	jobject CallStaticObjectMethod(jclass clazz, jmethodID mid);
	jobject CallObjectMethodA(jobject obj, jmethodID mid, jvalue* val);
	jobject CallObjectMethod(jobject obj, jmethodID mid);
	jobject CallNonvirtualObjectMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
	jobject CallNonvirtualObjectMethod(jobject obj, jclass claz, jmethodID mid);
	jobjectArray NewObjectArray(jsize a0, jclass a1, jobject a2);
	void SetObjectArrayElement(jobjectArray a0, jsize a1, jobject a2);

	// String
	jstring NewStringUTF(const char* a0);

	/** This returns a UTF16 surogate coded UTF-8 string.
	 */
	const char* GetStringUTFChars(jstring a0, jboolean* a1);
	void ReleaseStringUTFChars(jstring a0, const char* a1);
	const jchar* GetStringChars(jstring a0, jboolean* a1);
	void ReleaseStringChars(jstring a0, const jchar* a1);
	jsize GetStringLength(jstring a0);
} ;

/** JPClass is a bit heavy when we just need to hold a 
 * class reference.  It causes issues during bootstrap. Thus we
 * need a lightweight reference to a jclass.
 */
template <class jref>
class JPRef
{
private:
	jref m_Ref;

public:

	JPRef()
	{
		m_Ref = 0;
	}

	JPRef(jref obj)
	{
		JPJavaFrame frame;
		m_Ref = (jref) frame.NewGlobalRef((jobject) obj);
	}

	JPRef(const JPRef& other)
	{
		JPJavaFrame frame;
		m_Ref = (jref) frame.NewGlobalRef((jobject) other.m_Ref);
	}

	~JPRef()
	{
		if (m_Ref != 0)
			JPJavaFrame::ReleaseGlobalRef((jobject) m_Ref);
	}

	JPRef& operator=(const JPRef& other)
	{
		if (other.m_Ref == m_Ref)
			return *this;
		JPJavaFrame frame;
		if (m_Ref != 0)
			frame.DeleteGlobalRef((jobject) m_Ref);
		m_Ref = other.m_Ref;
		if (m_Ref != 0)
			m_Ref = (jclass) frame.NewGlobalRef((jobject) m_Ref);
		return *this;
	}

	jref get() const
	{
		return m_Ref;
	}
} ;

typedef JPRef<jclass> JPClassRef;
typedef JPRef<jobject> JPObjectRef;
typedef JPRef<jarray> JPArrayRef;
typedef JPRef<jthrowable> JPThrowableRef;


#endif // _JP_JAVA_FRAME_H_
