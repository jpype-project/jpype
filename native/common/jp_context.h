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
	JPContext(const JPContext& orig);
	virtual ~JPContext();

	// JVM control functions
	bool isInitialized();
	void assertJVMRunning(const char* function, const JPStackInfo& info);
	void startJVM(const string& vmPath, char ignoreUnrecognized, const StringVector& args);
	void shutdownJVM();
	void attachCurrentThread();
	void attachCurrentThreadAsDaemon();
	bool isThreadAttached();
	void detachCurrentThread();
	JavaVM* getJavaVM();

	// JPype services
	JPProxyFactory* getProxyFactory();
	JPTypeManager* getTypeManager();
	JPClassLoader* getClassLoader();

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

	JPBoxedClass* _java_lang_Void;
	JPBoxedClass* _java_lang_Boolean;
	JPBoxedClass* _java_lang_Byte;
	JPBoxedClass* _java_lang_Char;
	JPBoxedClass* _java_lang_Short;
	JPBoxedClass* _java_lang_Integer;
	JPBoxedClass* _java_lang_Long;
	JPBoxedClass* _java_lang_Float;
	JPBoxedClass* _java_lang_Double;

	JPClass* _java_lang_Object;
	JPClass* _java_lang_Class;
	JPStringClass* _java_lang_String;

	JPClassRef _java_lang_RuntimeException;
	JPClassRef _java_lang_NoSuchMethodError;
	
private:

	void loadEntryPoints(const string& path);
	void createJVM(void* arg);	// JVM

	JavaVM* m_JavaVM;

	jint(JNICALL * CreateJVM_Method)(JavaVM **pvm, void **penv, void *args);
	jint(JNICALL * GetCreatedJVMs_Method)(JavaVM **pvm, jsize size, jsize * nVms);

private:
	// Java half
	JPObjectRef m_JavaContext;

	// Serives
	JPTypeFactory* m_TypeFactory;
	JPTypeManager* m_TypeManager;
	JPClassLoader* m_ClassLoader;
	JPReferenceQueue* m_ReferenceQueue;
	JPProxyFactory* m_ProxyFactory;

	// Java Functions
	jmethodID m_Object_ToStringID;
	jmethodID m_ContextShutdownMethod;
} ;

#endif /* JP_CONTEXT_H */

