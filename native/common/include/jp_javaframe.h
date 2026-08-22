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
	JPContext* m_Context;
	bool m_Popped;
	bool m_Outer;
	// True only for a fast()-constructed frame: m_Popped is permanently
	// true for these from birth (fast() never pushes a real frame to pop),
	// which is their normal, valid resting state for their entire
	// lifetime -- not a "this frame was already used up" signal the way
	// m_Popped==true means for every other constructor. check()'s
	// JP_FRAME_CHECK() needs to tell the two apart; see its use below.
	bool m_Fast;

private:
	JPJavaFrame(JNIEnv* env, JPContext* ctx, int size, bool outer);
	JPJavaFrame(JNIEnv* env, JPContext* ctx);  // fast(): no PushLocalFrame

public:

	/** Create a new JavaFrame when called from Python.
	 *
	 * This method will automatically attach the thread
	 * if it is not already attached.
	 *
	 * @param size determines how many objects can be
	 * created in this scope without additional overhead.
	 *
	 * @throws JPBaseError if the jpype cannot
	 * acquire an env handle to work with jvm.
	 */
	static JPJavaFrame outer(JPContext* ctx, int size = LOCAL_FRAME_DEFAULT)
	{
		return {nullptr, ctx, size, true};
	}

	/** Create a new JavaFrame when called internal when
	 * there is an existing frame.
	 *
	 * @param size determines how many objects can be
	 * created in this scope without additional overhead.
	 *
	 * @throws JPBaseError if the jpype cannot
	 * acquire an env handle to work with jvm.
	 */
//	static JPJavaFrame inner(int size = LOCAL_FRAME_DEFAULT)
//	{
//		return {nullptr, size, false};
//	}

	/** Create a new JavaFrame when called from Java.
	 *
	 * The thread was attached by definition.
	 *
	 * @param size determines how many objects can be
	 * created in this scope without additional overhead.
	 *
	 * @throws JPBaseError if the jpype cannot
	 * acquire an env handle to work with jvm.
	 */
	static JPJavaFrame external(JNIEnv* env, JPContext* ctx, int size = LOCAL_FRAME_DEFAULT)
	{
		return {env, ctx, size, false};
	}

	/** Create a lightweight frame that does not push a JNI local frame.
	 *
	 * Only valid at call sites that are provably local-reference-free for
	 * their entire duration (no `New*`-family JNI call, no
	 * jobject-returning JNI call, no re-entry into Python). It borrows
	 * whatever real frame already exists further up the call stack --
	 * using fast() does not create a new safety scope of its own. See the
	 * JP_ASSERT_FAST_FRAMES build (JP_ASSERT_HAS_FRAME/g_frameDepth in
	 * jp_javaframe.cpp) for the mechanism that checks this contract.
	 *
	 * context is whatever the caller already has in hand (e.g.
	 * JPJavaAccess::getContext()) -- there is no ambient fallback here,
	 * for the same reason JPJavaAccess itself requires one explicitly.
	 */
	static JPJavaFrame fast(JNIEnv* env, JPContext* context)
	{
		return JPJavaFrame(env, context);
	}

	JPJavaFrame(const JPJavaFrame& frame);

	/** Transfer ownership of an already-pushed frame without pushing a
	 * second one.
	 *
	 * Required because the copy constructor above pushes a brand new real
	 * JNI local frame rather than sharing the source's -- correct for an
	 * explicit, intentional copy, but C++14 (this project's standard, see
	 * CMAKE_CXX_STANDARD) only makes copy elision for a function's
	 * by-value return *encouraged*, not mandatory the way C++17 does. A
	 * `JPJavaFrame frame = JPJavaFrame::outer(ctx);`-style call whose
	 * compiler/optimization level doesn't elide the temporary falls back
	 * to constructing `frame` from that temporary -- via this move
	 * constructor if one exists, otherwise silently via the copy
	 * constructor instead, pushing an extra, unaccounted-for real JNI
	 * frame every such call makes. That extra frame was invisible before
	 * the JPJavaFrame destructor fix that made ~JPJavaFrame() always pop
	 * an unpopped frame regardless of m_Outer -- the accidental frame
	 * used to just leak instead of ever being popped. Once real leaks
	 * started being caught, unoptimized (-O0) builds started reliably
	 * hitting "local reference outside of frame" instead, because the
	 * extra copy-constructed frame's own destructor now pops a real JNI
	 * frame layer code elsewhere doesn't know exists, invalidating local
	 * refs created in what calling code assumes is still the same,
	 * still-open frame.
	 */
	JPJavaFrame(JPJavaFrame&& frame) noexcept;

	/** Exit the local scope and clean up all java
	 * objects.
	 *
	 * Only the one local object passed to outer scope
	 * by the keep method will be kept alive.
	 */
	~JPJavaFrame();

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

	/** Stores obj in this frame's interpreter's Java-side GlobalPool
	 * (org.jpype.internal.NativeContext#storeGlobal) and returns a handle
	 * for it, in place of a JNI NewGlobalRef.
	 */
	jref storeGlobal(jobject obj);

	/** Resolves a handle from storeGlobal() back to a local reference
	 * scoped to this frame (org.jpype.internal.NativeContext#retrieveGlobal),
	 * or nullptr if it's stale/foreign/already released.
	 */
	jobject retrieveGlobal(jref ref);

	JNIEnv* getEnv() const
	{
		return m_Env;
	}
	JPContext* getContext() const
	{
		// We can add guard statements here.
		return m_Context;
	}

	string toString(jobject o);
	string toStringUTF8(jstring str);

	bool equals(jobject o1, jobject o2);
	jint hashCode(jobject o);
	jobject collectRectangular(jarray obj);
	jobject assemble(jobject dims, jobject parts);

	// Buffer-handoff multi-dim push/pull -- single JNI entry into
	// org.jpype.internal.Support, everything after that is plain Java (no
	// per-leaf-array JNI calls), including the serial-vs-parallel
	// decision -- Java already knows the total element count once the
	// shape is in hand and has no less insight into IntStream/
	// ForkJoinPool dispatch cost than C++ would, so that decision isn't
	// threaded across the JNI boundary at all. `typeCode` is the JNI
	// primitive type signature character (see
	// JPPrimitiveType::getTypeCode()); `buf` must be a direct
	// java.nio.ByteBuffer.
	jobject fillMultiArrayFromBuffer(char typeCode, jint mode, jobject buf, jintArray shape);
	void collectMultiArrayToBuffer(char typeCode, jobject collected, jobject buf);

	// Push-side mirror of collectMultiArrayToBuffer above -- writes buf's
	// contents into collected's existing leaf arrays in place (JArray::
	// pushFrom's N-D case), rather than reading them out. `collected` must
	// be the result of collectRectangular against the array being pushed
	// into, so its leaf-array references are the array's own -- this never
	// allocates a new array, preserving the target's identity.
	void fillBufferIntoMultiArray(char typeCode, jobject collected, jobject buf);

	// Ragged-native nested-list push -- same shape as
	// fillMultiArrayFromBuffer above (single JNI entry into
	// org.jpype.internal.Support, everything after that plain Java), but
	// for a ragged tree (one int32 length marker per node, depth-first
	// pre-order) rather than a fixed rectangular shape array. `buf` must
	// be a direct java.nio.ByteBuffer holding the encoded tree; `dims` is
	// the array's static nesting depth.
	jobject fillRaggedFromBuffer(char typeCode, jint dims, jobject buf);

	// Flat (1D) buffer-handoff push -- JPConversionBuffer's fast path
	// (jp_classhints.cpp). Unlike fillMultiArrayFromBuffer, dtype coercion
	// (srcKind/srcSize/swapped) and a non-unit strideBytes are handled
	// directly on the Java side, so this covers both a raw reinterpret and
	// a genuine coercing/non-contiguous push in the same single JNI call.
	jobject fillFlatFromBuffer(char typeCode, char srcKind, jint srcSize, jboolean swapped,
			jobject buf, jint length, jint strideBytes);

	// Write-into sibling of fillFlatFromBuffer -- writes directly into an
	// existing Java array's [destStart, destStart+destStep*length) range
	// (JPArray::setRange/clone's fast path, jp_convert.cpp's
	// tryFastBufferPush) instead of allocating and returning a fresh one.
	void fillFlatIntoArray(char typeCode, char srcKind, jint srcSize, jboolean swapped,
			jobject buf, jint length, jint strideBytes,
			jarray dest, jint destStart, jint destStep);

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
	PyObject* getFunctional(jclass c);

	JPClass *findClass(jclass obj);
	JPClass *findClassByName(const string& name);
	JPClass *findClassForObject(jobject obj);

	// not implemented
	JPJavaFrame& operator= (const JPJavaFrame& frame) = delete;

private:
	jint PushLocalFrame(jint);
	jobject PopLocalFrame(jobject);

public:

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
	jboolean IsSameObject(jobject ref1, jobject ref2);
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

/** A compile-time-narrow companion to JPJavaFrame for call sites that are
 * provably local-reference-free on their own happy path (no `New*`, no
 * `CallObjectMethodA` family, no re-entry into Python) -- e.g. a primitive
 * array element read (`Get<Type>ArrayRegion`). Unlike JPJavaFrame::fast(),
 * this is not the same C++ type as JPJavaFrame at all: it has no
 * reference-creating methods to call in the first place, so a future edit
 * that tries to reach for one here is a compile error in every build, not
 * a runtime assertion gated behind JP_ASSERT_FAST_FRAMES. It never pushes a
 * JNI local frame and never needs to -- it makes no local references.
 *
 * The one thing every wrapped JNI call still needs is the *exception*
 * check every JAVA_CHECK/JAVA_RETURN pays after any JNI call (even a call
 * that is documented as not throwing can still observe an
 * already-pending exception from something earlier in the call chain).
 * checkFast() is that check's frame-less equivalent: on the (overwhelming)
 * common case of no pending exception it touches nothing but
 * ExceptionCheck(), a boolean query. Only on the rare exception branch
 * does it escalate to a real JPJavaFrame::inner(), specifically because
 * ExceptionOccurred() returns a local jthrowable reference (and building
 * the JPJavaError from it may create more), which needs somewhere real to
 * be popped from.
 */
/** RAII guard for a single local reference obtained on a JPJavaAccess/
 * fast() call path that pushes no frame of its own (see JPJavaFrame::fast's
 * ctor comment) -- deletes the reference when it goes out of scope,
 * including via an exception unwinding past it, unlike a bare
 * DeleteLocalRef() call placed after the code that uses the reference
 * (which is skipped if that code throws). Not needed on a path with a
 * real enclosing frame (outer()/inner()/external()), which already
 * reclaims every local reference created under it on its own.
 */
class JPLocalRef
{
	JNIEnv* m_Env;
	jobject m_Ref;
public:
	JPLocalRef(JNIEnv* env, jobject ref) : m_Env(env), m_Ref(ref)
	{
	}

	JPLocalRef(const JPLocalRef&) = delete;
	JPLocalRef& operator=(const JPLocalRef&) = delete;

	~JPLocalRef()
	{
		if (m_Ref != nullptr)
			m_Env->DeleteLocalRef(m_Ref);
	}

	jobject get() const
	{
		return m_Ref;
	}
} ;

class JPJavaAccess
{
	JNIEnv* m_Env;
	JPContext* m_Context;

public:
	/** context is whatever the caller already has in hand (e.g. a
	 * JPClass::getContext()) -- there is no ambient fallback, since this
	 * class exists precisely for hot per-element call sites that must not
	 * silently resolve the wrong sub-interpreter's context.
	 */
	explicit JPJavaAccess(JPContext* context);

	void checkFast();

	/** For a caller that needs to escalate to a real JPJavaFrame (e.g. to
	 * call the shared convertToPythonObject) -- pass to
	 * JPJavaFrame::fast(env, context) so it doesn't redundantly re-fetch
	 * this thread's JNIEnv*, which is a real JNI call, not free.
	 */
	JNIEnv* getEnv() const
	{
		return m_Env;
	}

	JPContext* getContext() const
	{
		return m_Context;
	}

	jsize GetArrayLength(jarray a0);

	void GetBooleanArrayRegion(jbooleanArray array, jsize start, jsize len, jboolean* vals);
	void GetByteArrayRegion(jbyteArray array, jsize start, jsize len, jbyte* vals);
	void GetCharArrayRegion(jcharArray array, jsize start, jsize len, jchar* vals);
	void GetShortArrayRegion(jshortArray array, jsize start, jsize len, jshort* vals);
	void GetIntArrayRegion(jintArray array, jsize start, jsize len, jint* vals);
	void GetLongArrayRegion(jlongArray array, jsize start, jsize len, jlong* vals);
	void GetFloatArrayRegion(jfloatArray array, jsize start, jsize len, jfloat* vals);
	void GetDoubleArrayRegion(jdoubleArray array, jsize start, jsize len, jdouble* vals);
} ;

#endif // _JP_JAVA_FRAME_H_
