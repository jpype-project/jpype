/*****************************************************************************
   Copyright 2019 Karl Einar Nelson

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
#ifndef JP_CONTEXT_H
#define JP_CONTEXT_H
#include <jpype.h>

/** JPClass is a bit heavy when we just need to hold a
 * class reference.  It causes issues during bootstrap. Thus we
 * need a lightweight reference to a jclass.
 */
template <class jref>
class JPRef
{
private:
	JPContext* m_Context;
	jref m_Ref;

public:

	JPRef()
	{
		m_Context = 0;
		m_Ref = 0;
	}

	JPRef(JPContext* context, jref obj)
	{
		m_Context = context;
		m_Ref = 0;
		if (context == 0)
			return;
		JPJavaFrame frame(m_Context);
		m_Ref = (jref) frame.NewGlobalRef((jobject) obj);
	}

	JPRef(JPJavaFrame& frame, jref obj)
	{

		m_Context = frame.getContext();
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

typedef JPRef<jclass> JPClassRef;
typedef JPRef<jobject> JPObjectRef;
typedef JPRef<jarray> JPArrayRef;
typedef JPRef<jthrowable> JPThrowableRef;

class JPStackInfo;
class JPGarbageCollection;

void assertJVMRunning(JPContext* context, const JPStackInfo& info);

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

	// JVM control functions
	bool isRunning();
	void startJVM(const string& vmPath, const StringVector& args, bool ignoreUnrecognized, bool convertStrings);
	void shutdownJVM();
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

	bool isShutdown()
	{
		return m_IsShutdown;
	}

	/** Release a global reference checking for shutdown.
	 *
	 * This should be used in any calls to release resources from a destructor.
	 * It cannot fail even if the JVM is no longer operating.
	 */
	void ReleaseGlobalRef(jobject obj);

	// JPype services

	JPProxyFactory* getProxyFactory()
	{
		return m_ProxyFactory;
	}

	JPTypeManager* getTypeManager()
	{
		return m_TypeManager;
	}

	JPClassLoader* getClassLoader()
	{
		return m_ClassLoader;
	}

	JPReferenceQueue* getReferenceQueue()
	{
		return m_ReferenceQueue;
	}

	bool getConvertStrings() const
	{
		return m_ConvertStrings;
	}

	// Java type resources
	JPPrimitiveType* _void;
	JPPrimitiveType* _boolean;
	JPPrimitiveType* _byte;
	JPPrimitiveType* _char;
	JPPrimitiveType* _short;
	JPPrimitiveType* _int;
	JPPrimitiveType* _long;
	JPPrimitiveType* _float;
	JPPrimitiveType* _double;

	JPClass* _java_lang_Void;
	JPClass* _java_lang_Boolean;
	JPClass* _java_lang_Byte;
	JPClass* _java_lang_Character;
	JPClass* _java_lang_Short;
	JPClass* _java_lang_Integer;
	JPClass* _java_lang_Long;
	JPClass* _java_lang_Float;
	JPClass* _java_lang_Double;

	JPClass* _java_lang_Object;
	JPClass* _java_lang_Class;
	JPClass* _java_lang_reflect_Field;
	JPClass* _java_lang_reflect_Method;
	JPClass* _java_lang_Throwable;
	JPStringType* _java_lang_String;

	jmethodID m_BooleanValueID;
	jmethodID m_ByteValueID;
	jmethodID m_CharValueID;
	jmethodID m_ShortValueID;
	jmethodID m_IntValueID;
	jmethodID m_LongValueID;
	jmethodID m_FloatValueID;
	jmethodID m_DoubleValueID;

private:

	void loadEntryPoints(const string& path);

	jint(JNICALL * CreateJVM_Method)(JavaVM **pvm, void **penv, void *args);
	jint(JNICALL * GetCreatedJVMs_Method)(JavaVM **pvm, jsize size, jsize * nVms);

private:
	JPContext(const JPContext& orig);

	JavaVM *m_JavaVM;

	// Java half
	JPObjectRef m_JavaContext;

	// Services
	JPTypeFactory *m_TypeFactory;
	JPTypeManager *m_TypeManager;
	JPClassLoader *m_ClassLoader;
	JPReferenceQueue *m_ReferenceQueue;
	JPProxyFactory *m_ProxyFactory;

public:
	JPClassRef m_RuntimeException;
	JPClassRef m_NoSuchMethodError;

private:
	// Java Functions
	jmethodID m_Object_ToStringID;
	jmethodID m_Object_EqualsID;
	jmethodID m_Object_HashCodeID;
	jmethodID m_ShutdownMethodID;
	jmethodID m_CallMethodID;
	jmethodID m_Class_GetNameID;
	jmethodID m_Context_collectRectangularID;
	jmethodID m_Context_assembleID;
	jmethodID m_String_ToCharArrayID;
	jmethodID m_Context_CreateExceptionID;
	jmethodID m_Context_GetExcClassID;
	jmethodID m_Context_GetExcValueID;
	jmethodID m_CompareToID;
	jmethodID m_Buffer_IsReadOnlyID;
	jmethodID m_Context_OrderID;
	jmethodID m_Object_GetClassID;
private:
	bool m_IsShutdown;
	bool m_IsInitialized;
	bool m_ConvertStrings;
public:
	JPGarbageCollection *m_GC;
} ;

extern void JPRef_failed();

template<class jref>
JPRef<jref>::JPRef(const JPRef& other)
{
	m_Context = other.m_Context;
	if (m_Context != NULL)
	{
		JPJavaFrame frame(m_Context, m_Context->getEnv());
		m_Ref = (jref) frame.NewGlobalRef((jobject) other.m_Ref);
	} else
	{
		JPRef_failed();
	}
}

template<class jref>
JPRef<jref>::~JPRef()
{
	if (m_Ref != 0 && m_Context != 0)
	{
		m_Context->ReleaseGlobalRef((jobject) m_Ref);
	}
}

template<class jref>
JPRef<jref>& JPRef<jref>::operator=(const JPRef<jref>& other)
{
	if (other.m_Ref == m_Ref)
		return *this;
	// m_Context may or may not be set up here, so we need to use a
	// different frame for unreferencing and referencing
	if (m_Context != 0 && m_Ref != 0)
	{
		JPJavaFrame frame(m_Context, m_Context->getEnv());
		if (m_Ref != 0)
			frame.DeleteGlobalRef((jobject) m_Ref);
	}
	m_Context = other.m_Context;
	m_Ref = other.m_Ref;
	if (m_Context != 0 && m_Ref != 0)
	{
		JPJavaFrame frame(m_Context, m_Context->getEnv());
		m_Ref = (jref) frame.NewGlobalRef((jobject) m_Ref);
	}
	return *this;
}

#endif /* JP_CONTEXT_H */
