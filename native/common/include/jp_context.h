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
#ifndef JP_CONTEXT_H
#define JP_CONTEXT_H
#include <jpype.h>
#include <list>

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
		m_Ref = 0;
		JPJavaFrame frame = JPJavaFrame::outer();
		m_Ref = (jref) frame.NewGlobalRef((jobject) obj);
	}

	JPRef(JPJavaFrame& frame, jref obj)
	{

		m_Ref = 0;
		m_Ref = (jref) frame.NewGlobalRef((jobject) obj);
	}

	JPRef(const JPRef& other);

	~JPRef();

	JPRef& operator=(const JPRef& other);

	jref get() const
	{
		return m_Ref;
	}
} ;

using JPClassRef = JPRef<jclass>;
using JPObjectRef = JPRef<jobject>;
using JPArrayRef = JPRef<jarray>;
using JPThrowableRef = JPRef<jthrowable>;

class JPStackInfo;
class JPGarbageCollection;

void assertJVMRunning(JPContext* context, const JPStackInfo& info);

int hasInterrupt();

/**
 * A Context encapsulates the Java virtual machine, the Java classes required
 * to function, and the JPype services created for that machine.
 *
 * Frames, Environments and Contexts are different concepts.
 *  - Java context is shared with all objects that exist in a virtual machine.
 *  - Java environment exists for each thread for each machine.
 *  - Java frames exist in the stack holding the local variables that
 *    method.
 * Frames and Environments should never be held longer than the duration of
 * a method.
 *
 * The members in the Context are broken into
 * - JVM control functions
 * - JPype services
 * - Java functions
 * - Java type resources
 *
 * There are two critical phases in the context lifespan where things
 * are most likely to go wrong. During JVM startup, many vital functions are
 * not yet fully configured and thus an exception issued during that period
 * can lead to using a resource that is not yet configured.
 *
 * Second shutdown is a vital period. If the user calls shutdown from within
 * a callback in which there are still JPype context elements on the call
 * stack, it can lead to a crash.
 *
 * After shutdown, the JVM is set to NULL and all actions should assert that
 * it is running and thus fail. Aside from the context and the services, all
 * other JPype C++ resources are owned by Java. Java will delete them as needed.
 * The context itself belongs to Python.
 */
class JPContext
{
public:
	friend class JPJavaFrame;
	friend class JPypeException;
	friend class JPClass;

	JPContext();
	virtual ~JPContext();
    JPContext(const JPContext& orig) = delete;

    // JVM control functions
	bool isRunning();
	void startJVM(const string& vmPath, const StringVector& args,
			bool ignoreUnrecognized, bool convertStrings, bool interrupt);
	void attachJVM(JNIEnv* env);
	void initializeResources(JNIEnv* env, bool interrupt);
	void shutdownJVM(bool destroyJVM, bool freeJVM);
	void attachCurrentThread();
	void attachCurrentThreadAsDaemon();
	bool isThreadAttached();
	void detachCurrentThread();

	JNIEnv* getEnv();

	JavaVM* getJavaVM()
	{
		return m_JavaVM;
	}

	jobject getJavaContext()
	{
		return m_JavaContext.get();
	}

	/** Release a global reference checking for shutdown.
	 *
	 * This should be used in any calls to release resources from a destructor.
	 * It cannot fail even if the JVM is no longer operating.
	 */
	void ReleaseGlobalRef(jobject obj);

	// JPype services

	JPTypeManager* getTypeManager()
	{
		return m_TypeManager;
	}

	JPClassLoader* getClassLoader()
	{
		return m_ClassLoader;
	}

	bool getConvertStrings() const
	{
		return m_ConvertStrings;
	}

	// Java type resources
	JPPrimitiveType* _void{};
	JPPrimitiveType* _boolean{};
	JPPrimitiveType* _byte{};
	JPPrimitiveType* _char{};
	JPPrimitiveType* _short{};
	JPPrimitiveType* _int{};
	JPPrimitiveType* _long{};
	JPPrimitiveType* _float{};
	JPPrimitiveType* _double{};

	JPBoxedType* _java_lang_Void{};
	JPBoxedType* _java_lang_Boolean{};
	JPBoxedType* _java_lang_Byte{};
	JPBoxedType* _java_lang_Character{};
	JPBoxedType* _java_lang_Short{};
	JPBoxedType* _java_lang_Integer{};
	JPBoxedType* _java_lang_Long{};
	JPBoxedType* _java_lang_Float{};
	JPBoxedType* _java_lang_Double{};

	JPClass* _java_lang_Object{};
	JPClass* _java_lang_Class{};
	JPClass* _java_lang_reflect_Field{};
	JPClass* _java_lang_reflect_Method{};
	JPClass* _java_lang_Throwable{};
	JPStringType* _java_lang_String{};
	JPClass* _java_nio_ByteBuffer{};

private:

	void loadEntryPoints(const string& path);

	jint(JNICALL * CreateJVM_Method)(JavaVM **pvm, void **penv, void *args){};
	jint(JNICALL * GetCreatedJVMs_Method)(JavaVM **pvm, jsize size, jsize * nVms){};

private:

	JavaVM *m_JavaVM{};

	// Java half
	JPObjectRef m_JavaContext;

	// Services
	JPTypeManager *m_TypeManager{};
	JPClassLoader *m_ClassLoader{};

public:
	JPClassRef m_ContextClass;
	JPClassRef m_RuntimeException;

private:
	JPClassRef m_Array;
	JPObjectRef m_Reflector;

	// Java Functions
	jmethodID m_Object_ToStringID{};
	jmethodID m_Object_EqualsID{};
	jmethodID m_Object_HashCodeID{};
	jmethodID m_CallMethodID{};
	jmethodID m_Class_GetNameID{};
	jmethodID m_Context_collectRectangularID{};
	jmethodID m_Context_assembleID{};
	jmethodID m_String_ToCharArrayID{};
	jmethodID m_Context_CreateExceptionID{};
	jmethodID m_Context_GetExcClassID{};
	jmethodID m_Context_GetExcValueID{};
	jmethodID m_Context_ClearInterruptID{};
	jmethodID m_CompareToID{};
	jmethodID m_Buffer_IsReadOnlyID{};
	jmethodID m_Buffer_AsReadOnlyID{};
	jmethodID m_Context_OrderID{};
	jmethodID m_Object_GetClassID{};
	jmethodID m_Array_NewInstanceID{};
	jmethodID m_Throwable_GetCauseID{};
	jmethodID m_Throwable_GetMessageID{};
	jmethodID m_Context_GetFunctionalID{};
	friend class JPProxy;
	JPClassRef m_ProxyClass;
	jmethodID m_Proxy_NewID{};
	jmethodID m_Proxy_NewInstanceID{};

	jmethodID m_Context_IsPackageID{};
	jmethodID m_Context_GetPackageID{};
	jmethodID m_Package_GetObjectID{};
	jmethodID m_Package_GetContentsID{};
	jmethodID m_Context_NewWrapperID{};
public:
	jmethodID m_Context_GetStackFrameID{};
	void onShutdown();

private:
	bool m_Running{};
	bool m_ConvertStrings{};
	bool m_Embedded;
public:
	JPGarbageCollection *m_GC;

	// This will gather C++ resources to clean up after shutdown.
	std::list<JPResource*> m_Resources;
} ;

extern "C" JPContext* JPContext_global;

extern void JPRef_failed();

// GCOVR_EXCL_START
// Not currently used

template<class jref>
JPRef<jref>::JPRef(const JPRef& other)
{
	if (JPContext_global != nullptr)
	{
		JPJavaFrame frame = JPJavaFrame::external(JPContext_global->getEnv());
		m_Ref = (jref) frame.NewGlobalRef((jobject) other.m_Ref);
	} else
	{
		JPRef_failed();
	}
}
// GCOVR_EXCL_STOP

template<class jref>
JPRef<jref>::~JPRef()
{
	if (m_Ref != 0)
	{
		JPContext_global->ReleaseGlobalRef((jobject) m_Ref);
	}
}

template<class jref>
JPRef<jref>& JPRef<jref>::operator=(const JPRef<jref>& other)
{
	if (other.m_Ref == m_Ref)
		return *this;
	// m_Context may or may not be set up here, so we need to use a
	// different frame for unreferencing and referencing
	if (JPContext_global != nullptr && m_Ref != 0)
	{  // GCOVR_EXCL_START
		// This code is not currently used.
		JPJavaFrame frame = JPJavaFrame::external(JPContext_global->getEnv());
		if (m_Ref != 0)
			frame.DeleteGlobalRef((jobject) m_Ref);
	}  // GCOVR_EXCL_STOP
	m_Ref = other.m_Ref;
	if (JPContext_global != nullptr && m_Ref != 0)
	{
		JPJavaFrame frame = JPJavaFrame::external(JPContext_global->getEnv());
		m_Ref = (jref) frame.NewGlobalRef((jobject) m_Ref);
	}
	return *this;
}

#endif /* JP_CONTEXT_H */
