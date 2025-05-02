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
 * good to size the frame appropriately.
 *
 * A JavaFrame should not be used in a destructor as it can
 * throw. The most common use of JavaFrame is to delete a
 * global reference.  See the ReleaseGlobalReference for
 * this purpose.
 */
static const int LOCAL_FRAME_DEFAULT = 8;

class JPJavaFrame
{
	JNIEnv* m_Env;
	bool m_Popped;
	bool m_Outer;

private:
	JPJavaFrame(JNIEnv* env, int size, bool outer);

public:

	/** Create a new JavaFrame when called from Python.
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
	static JPJavaFrame outer(int size = LOCAL_FRAME_DEFAULT)
	{
		return {nullptr, size, true};
	}

	/** Create a new JavaFrame when called internal when
	 * there is an existing frame.
	 *
	 * @param size determines how many objects can be
	 * created in this scope without additional overhead.
	 *
	 * @throws JPypeException if the jpype cannot
	 * acquire an env handle to work with jvm.
	 */
	static JPJavaFrame inner(int size = LOCAL_FRAME_DEFAULT)
	{
		return {nullptr, size, false};
	}

	/** Create a new JavaFrame when called from Java.
	 *
	 * The thread was attached by definition.
	 *
	 * @param size determines how many objects can be
	 * created in this scope without additional overhead.
	 *
	 * @throws JPypeException if the jpype cannot
	 * acquire an env handle to work with jvm.
	 */
	static JPJavaFrame external(JNIEnv* env, int size = LOCAL_FRAME_DEFAULT)
	{
		return {env, size, false};
	}

	JPJavaFrame(const JPJavaFrame& frame);

	/** Exit the local scope and clean up all java
	 * objects.
	 *
	 * Only the one local object passed to outer scope
	 * by the keep method will be kept alive.
	 */
	~JPJavaFrame();

	JPContext* getContext();

	void check();

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

	jweak NewWeakGlobalRef(jobject obj);
	void DeleteWeakGlobalRef(jweak obj);

	JNIEnv* getEnv() const
	{
		return m_Env;
	}

	string toString(jobject o);
	string toStringUTF8(jstring str);

	bool equals(jobject o1, jobject o2);
	jint hashCode(jobject o);
	jobject collectRectangular(jarray obj);
	jobject assemble(jobject dims, jobject parts);

	jobject newArrayInstance(jclass c, jintArray dims);
	jthrowable getCause(jthrowable th);
	jstring getMessage(jthrowable th);
	jint compareTo(jobject obj, jobject obj2);

	/**
	 * Convert a UTF8 encoded string into Java.
	 *
	 * This returns a local reference.
	 * @param str
	 * @return
	 */
	jstring fromStringUTF8(const string& str);
	jobject callMethod(jobject method, jobject obj, jobject args);
	jobject toCharArray(jstring jstr);
	string getFunctional(jclass c);

	JPClass *findClass(jclass obj);
	JPClass *findClassByName(const string& name);
	JPClass *findClassForObject(jobject obj);

    // not implemented
    JPJavaFrame& operator= (const JPJavaFrame& frame) = delete;

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

	// Monitor
	int MonitorEnter(jobject a0);
	int MonitorExit(jobject a0);

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
	void CallNonvirtualVoidMethodA(jobject a0, jclass a1, jmethodID a2, jvalue* a3);

	// Bool
	jboolean GetStaticBooleanField(jclass clazz, jfieldID fid);
	jboolean GetBooleanField(jobject clazz, jfieldID fid);
	void SetStaticBooleanField(jclass clazz, jfieldID fid, jboolean val);
	void SetBooleanField(jobject clazz, jfieldID fid, jboolean val);
	jboolean CallStaticBooleanMethodA(jclass clazz, jmethodID mid, jvalue* val);
	jboolean CallBooleanMethodA(jobject obj, jmethodID mid, jvalue* val);
	jboolean CallNonvirtualBooleanMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
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
	jbyte CallByteMethodA(jobject obj, jmethodID mid, jvalue* val);
	jbyte CallNonvirtualByteMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
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
	jchar CallCharMethodA(jobject obj, jmethodID mid, jvalue* val);
	jchar CallNonvirtualCharMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
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
	jshort CallShortMethodA(jobject obj, jmethodID mid, jvalue* val);
	jshort CallNonvirtualShortMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
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
	jint CallIntMethodA(jobject obj, jmethodID mid, jvalue* val);
	jint CallNonvirtualIntMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
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
	jlong CallLongMethodA(jobject obj, jmethodID mid, jvalue* val);
	jlong CallNonvirtualLongMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
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
	jfloat CallFloatMethodA(jobject obj, jmethodID mid, jvalue* val);
	jfloat CallNonvirtualFloatMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
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
	jdouble CallDoubleMethodA(jobject obj, jmethodID mid, jvalue* val);
	jdouble CallNonvirtualDoubleMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
	jdoubleArray NewDoubleArray(jsize len);
	void SetDoubleArrayRegion(jdoubleArray array, jsize start, jsize len, jdouble* vals);
	void GetDoubleArrayRegion(jdoubleArray array, jsize start, jsize len, jdouble* vals);
	jdouble* GetDoubleArrayElements(jdoubleArray array, jboolean* isCopy);
	void ReleaseDoubleArrayElements(jdoubleArray, jdouble* v, jint mode);

	// Object
	jclass GetObjectClass(jobject obj);
	jobject GetStaticObjectField(jclass clazz, jfieldID fid);
	jobject GetObjectField(jobject clazz, jfieldID fid);
	void SetStaticObjectField(jclass clazz, jfieldID fid, jobject val);
	void SetObjectField(jobject clazz, jfieldID fid, jobject val);
	jobject CallStaticObjectMethodA(jclass clazz, jmethodID mid, jvalue* val);
	jobject CallObjectMethodA(jobject obj, jmethodID mid, jvalue* val);
	jobject CallNonvirtualObjectMethodA(jobject obj, jclass claz, jmethodID mid, jvalue* val);
	jobjectArray NewObjectArray(jsize a0, jclass a1, jobject a2);
	void SetObjectArrayElement(jobjectArray a0, jsize a1, jobject a2);

	// String
	jstring NewStringUTF(const char* a0);

	void* GetDirectBufferAddress(jobject obj);
	jlong GetDirectBufferCapacity(jobject obj);
	jboolean isBufferReadOnly(jobject obj);
	jobject asReadOnlyBuffer(jobject obj);
	jboolean orderBuffer(jobject obj);
	jclass getClass(jobject obj);

	/** This returns a UTF16 surogate coded UTF-8 string.
	 */
	const char* GetStringUTFChars(jstring a0, jboolean* a1);
	void ReleaseStringUTFChars(jstring a0, const char* a1);
	jsize GetStringUTFLength(jstring a0);

	jboolean isPackage(const string& str);
	jobject getPackage(const string& str);
	jobject getPackageObject(jobject pkg, const string& str);
	jarray getPackageContents(jobject pkg);

	void newWrapper(JPClass* cls);
	void registerRef(jobject obj, PyObject* hostRef);
	void registerRef(jobject obj, void* ref, JCleanupHook cleanup);

	void clearInterrupt(bool throws);

} ;

#endif // _JP_JAVA_FRAME_H_
