/*****************************************************************************
   Copyright 2004 Steve Ménard

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
#ifndef _JAVA_ENV_H_
#define _JAVA_ENV_H_

/**
 * Simple exception class to wrap java-sourced exceptions
 */
class JavaException
{
public :
	JavaException(const char* msg, const char* f, int l) : file(f), line(l) {message = msg;}
	JavaException(JavaException& ex) : file(ex.file), line(ex.line) {message = ex.message;}

	virtual ~JavaException() {}

	const char* file;
	int line;

	string message;
};

class HostException
{
public :
	HostException() {}
	virtual ~HostException() {}

	virtual const char* getFile() { return "";}
	virtual int getLine() {return 0;}

	virtual string getMessage() { return ""; }
};

/**
 *  the platform adapter's implementation is chosen by the JPYPE_??? macros
 */
class JPPlatformAdapter
{
public :
	virtual ~JPPlatformAdapter() {};
	virtual void loadLibrary(const char* path) = 0;
	virtual void* getSymbol(const char* name)= 0;
};

/**
 * Wrap all the needed parts of JNI to provide centralized error detection and dynamic loading
 */
class JPJavaEnv
{
public :
	JPJavaEnv(JavaVM* vm)
	{
		jvm = vm;
		convertStringObjects = true;
	}	

	virtual ~JPJavaEnv() {}
	
private :
	static JPPlatformAdapter* GetAdapter();
//	static JPPlatformAdapter* adapter;
	static jint (JNICALL *CreateJVM_Method)(JavaVM **pvm, void **penv, void *args);
	static jint (JNICALL *GetCreatedJVMs_Method)(JavaVM **pvm, jsize size, jsize* nVms);

	JavaVM* jvm;
   jobject referenceQueue;	
	bool convertStringObjects;

	JNIEnv* getJNIEnv();			

public :	
	static void load(const string& path);
	
	static JPJavaEnv* CreateJavaVM(void* arg);
	static JPJavaEnv* GetCreatedJavaVM();
	jint AttachCurrentThread();
	jint AttachCurrentThreadAsDaemon();
	static bool isThreadAttached();
	
	void setConvertStringObjects(bool flag)
	{
		convertStringObjects = flag;
	}

	bool getConvertStringObjects()
	{
		return convertStringObjects;
	}

	
	void setReferenceQueue(jobject obj)
	{
		referenceQueue = NewGlobalRef(obj);
	}

	jobject getReferenceQueue()
	{
		return referenceQueue;
	}
	
	void shutdown();
	void checkInitialized();

	int DestroyJavaVM();
	
	jobject NewGlobalRef(jobject obj);
	void DeleteGlobalRef(jobject obj);

	jobject NewLocalRef(jobject obj);
	void DeleteLocalRef(jobject obj);

	bool ExceptionCheck();
	void ExceptionDescribe();
	void ExceptionClear();
	jthrowable ExceptionOccurred();
	
	jint DetachCurrentThread();
	jint GetEnv(JNIEnv** env);
	jint ThrowNew(jclass clazz, const char* msg);

	jint Throw(jthrowable th);

	jobject NewDirectByteBuffer(void* address, jlong capacity);

	/** NewObjectA */
	jobject NewObjectA(jclass a0, jmethodID a1, jvalue* a2);

	/** NewObject */
	jobject NewObject(jclass a0, jmethodID a1);

	#include "jp_javaenv_autogen.h"

};

#endif // _JAVA_ENV_H_
