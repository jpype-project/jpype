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

class JPStackInfo;
class JPGarbageCollection;

void assertJVMRunning(JPContext* context, const JPStackInfo& info);

struct PyJPModuleState;
/**
 * A Context encapsulates the Java virtual machine, the Java classes required
 * to function, and the JPype services created for that machine.
 *
 * Frames, Environments and Contexts are different concepts.
 *  - Java context is shared with all objects that exist in a virtual machine.
 *  - Java environment exists for each thread for each machine.
 *  - Java frames exist in the stack holding the local variables that
 *	method.
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

	JPContext(PyJPModuleState *state);
	virtual ~JPContext();
	JPContext(const JPContext& orig) = delete;

	// JVM control functions
	bool isRunning();
	void startJVM(const string& vmPath, const StringVector& args,
			bool ignoreUnrecognized, bool convertStrings, bool interrupt);
	void attachJVM(JNIEnv* env);
	void detachJVM();
	void initializeResources(JNIEnv* env, bool interrupt);
	void shutdownJVM(bool destroyJVM, bool freeJVM);
	void attachCurrentThread();
	void attachCurrentThreadAsDaemon();
	bool isThreadAttached();
	void detachCurrentThread();

	PyJPModuleState *modulestate;

	JNIEnv* getEnv();
	JavaVM* getJavaVM()
	{
		return m_JavaVM;
	}

	jobject getJavaContext()
	{
		return m_JavaContext;
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
	//JPClass* _java_nio_ByteBuffer{};
	JPClass* _python_lang_PyObject{};

private:

	// Launch facilities
	void loadEntryPoints(const string& path);
	jint(JNICALL * CreateJVM_Method)(JavaVM **pvm, void **penv, void *args){};
	jint(JNICALL * GetCreatedJVMs_Method)(JavaVM **pvm, jsize size, jsize * nVms){};
	jint(JNICALL * GetDefaultJavaVMInitArgs_Method)(void *args){};
	JavaVM *m_JavaVM{};

	// Services
	JPTypeManager *m_TypeManager{};
	JPClassLoader *m_ClassLoader{};

public:
	jobject m_Interpreter;
	jclass m_RuntimeException;
	jclass m_Array;

	// Java Functions
	jmethodID m_Array_NewInstanceID{};
	jmethodID m_Buffer_IsReadOnlyID{};
	jmethodID m_Buffer_AsReadOnlyID{};
	jmethodID m_Class_GetNameID{};
	jmethodID m_CompareToID{};
	jmethodID m_Object_ToStringID{};
	jmethodID m_Object_EqualsID{};
	jmethodID m_Object_HashCodeID{};
	jmethodID m_Object_GetClassID{};
	jmethodID m_String_ToCharArrayID{};
	jmethodID m_Throwable_GetCauseID{};
	jmethodID m_Throwable_GetMessageID{};

	// --- Package Bindings ---
	jobject m_JavaContext;
	jclass m_ContextClass;
	jmethodID m_Context_ClearInterruptID{};
	jmethodID m_Context_GetFunctionalID{};
	jmethodID m_Context_IsPackageID{};
	jmethodID m_Context_GetPackageID{};
	jmethodID m_Package_GetObjectID{};
	jmethodID m_Package_GetContentsID{};
	jmethodID m_Context_NewWrapperID{};
	jmethodID m_Context_StoreGlobalID{};
	jmethodID m_Context_RetrieveGlobalID{};

	// --- Support Utilities ---
	jclass	m_SupportClass;
	jmethodID m_Support_GetStackFrameID{};
	jmethodID m_Support_collectRectangularID{};
	jmethodID m_Support_assembleID{};
	jmethodID m_Support_OrderID{};
	jmethodID m_Support_GetTotalMemoryID{};
	jmethodID m_Support_GetFreeMemoryID{};
	jmethodID m_Support_GetMaxMemoryID{};
	jmethodID m_Support_GetUsedMemoryID{};
	jmethodID m_Support_GetHeapMemoryID{};

	// --- Proxy Management (Refactored for NativeContext_2.java) ---
	friend class JPProxy;
	friend class JPypeException;
	jobject   m_JavaProxyFactory; // Holds the instance extracted from the context getter
	jclass	m_ProxyFactoryClass;
	jmethodID m_ProxyFactory_getProxyTypeID{};
	jclass	m_ProxyTypeClass;
	jmethodID m_ProxyType_newInstanceID{};
	jmethodID m_ProxyType_UnwrapPythonExceptionID{};
	jmethodID m_ProxyType_GetInstanceID{};

	jobject m_Reflector;
	jmethodID m_Reflector_CallMethodID{};  // Cleaned up naming match
	void onShutdown();

	jclass m_PyJavaObjectClass;
	jmethodID m_PyJavaObject_wrap{};

	// --- Reference Queue (per-context, see jp_reference_queue.cpp) ---
	jobject m_ReferenceQueue;
	jmethodID m_ReferenceQueueRegisterMethod{};

private:
	bool m_Running{};
	bool m_ConvertStrings{};
	bool m_Embedded;


public:
	JPGarbageCollection *m_GC;

	// This will gather C++ resources to clean up after shutdown.
	std::list<JPResource*> m_Resources;
	PyObject* m_PyExcConvert{};

	int interruptState;
	bool hasInterrupt()
	{
		return interruptState!=0;
	}

};

extern void tryRelease(jref ref) ;
extern void JPRef_failed();

// GCOVR_EXCL_START
// Not currently used

#endif /* JP_CONTEXT_H */
