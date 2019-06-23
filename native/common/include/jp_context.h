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

#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

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
		JPJavaFrame frame(m_Context);
		m_Ref = (jref) frame.NewGlobalRef((jobject) obj);
	}

	JPRef(const JPRef& other)
	{
		m_Context = other.m_Context;
		JPJavaFrame frame(m_Context);
		m_Ref = (jref) frame.NewGlobalRef((jobject) other.m_Ref);
	}

	~JPRef();

	JPRef& operator=(const JPRef& other)
	{
		if (other.m_Ref == m_Ref)
			return *this;
		if (m_Context != 0 && m_Ref != 0)
		{
			JPJavaFrame frame(m_Context);
			if (m_Ref != 0)
				frame.DeleteGlobalRef((jobject) m_Ref);
		}
		m_Context = other.m_Context;
		m_Ref = other.m_Ref;
		if (m_Context != 0 && m_Ref != 0)
		{
			JPJavaFrame frame(m_Context);
			m_Ref = (jref) frame.NewGlobalRef((jobject) m_Ref);
		}
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

class JPStackInfo;

/**
 * A Context encapsulates the Java virtual machine, the Java classes required
 * to function, and the JPype services created for that machine.
 *
 * Environments and Contexts are different concepts. A separate Java environment
 * exists for each thread for each machine. The Java context is shared with
 * all objects that share the same virtual machine.
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
	JPContext();
	virtual ~JPContext();

	// JVM control functions
	bool isRunning();
	void assertJVMRunning(const JPStackInfo& info);
	void startJVM(const string& vmPath, const StringVector& args, char ignoreUnrecognized, char convertStrings);
	void shutdownJVM();
	void attachCurrentThread();
	void attachCurrentThreadAsDaemon();
	bool isThreadAttached();
	void detachCurrentThread();

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

	// Java functions
	string toString(jobject o);
	string toStringUTF8(jstring str);

	/**
	 * Convert a UTF8 encoded string into Java.
	 *
	 * This returns a local reference.
	 * @param str
	 * @return
	 */
	jstring fromStringUTF8(const string& str);

	// Java type resources
	JPVoidType* _void;
	JPBooleanType* _boolean;
	JPByteType* _byte;
	JPCharType* _char;
	JPShortType* _short;
	JPIntType* _int;
	JPLongType* _long;
	JPFloatType* _float;
	JPDoubleType* _double;

	JPBoxedType* _java_lang_Void;
	JPBoxedType* _java_lang_Boolean;
	JPBoxedType* _java_lang_Byte;
	JPBoxedType* _java_lang_Char;
	JPBoxedType* _java_lang_Short;
	JPBoxedType* _java_lang_Integer;
	JPBoxedType* _java_lang_Long;
	JPBoxedType* _java_lang_Float;
	JPBoxedType* _java_lang_Double;

	JPClass* _java_lang_Object;
	JPClass* _java_lang_Class;
	JPStringType* _java_lang_String;

	JPClassRef _java_lang_RuntimeException;
	JPClassRef _java_lang_NoSuchMethodError;

	void setHost(PyObject* host)
	{
		m_Host = host;
	}

	PyObject* getHost()
	{
		return m_Host;
	}

private:

	void loadEntryPoints(const string& path);
	void createJVM(void* arg);	// JVM


	jint(JNICALL * CreateJVM_Method)(JavaVM **pvm, void **penv, void *args);
	jint(JNICALL * GetCreatedJVMs_Method)(JavaVM **pvm, jsize size, jsize * nVms);

private:
	JPContext(const JPContext& orig);

	JavaVM* m_JavaVM;

	// Java half
	JPObjectRef m_JavaContext;

	// Services
	JPTypeFactory* m_TypeFactory;
	JPTypeManager* m_TypeManager;
	JPClassLoader* m_ClassLoader;
	JPReferenceQueue* m_ReferenceQueue;
	JPProxyFactory* m_ProxyFactory;

	// Java Functions
	jmethodID m_Object_ToStringID;
	jmethodID m_ContextShutdownMethod;
	bool m_IsShutdown;
	bool m_IsInitialized;
	PyObject* m_Host;
	bool m_ConvertStrings;
} ;

template<class jref>
JPRef<jref>::~JPRef()
{
	if (m_Ref != 0 && m_Context != 0)
	{
		m_Context->ReleaseGlobalRef((jobject) m_Ref);
	}
}

#endif /* JP_CONTEXT_H */

