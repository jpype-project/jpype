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

/* 
 * File:   jp_context.cpp
 * Author: Karl Einar Nelson
 * 
 * Created on May 28, 2019, 12:15 AM
 */

#include "jp_context.h"


/*****************************************************************************/
// Platform handles the differences in dealing with shared libraries 
// on windows and unix variants.

#ifdef WIN32
#include "jp_platform_win32.h"
#define  PLATFORM_ADAPTER Win32PlatformAdapter
#else
#include "jp_platform_linux.h"
#define  PLATFORM_ADAPTER LinuxPlatformAdapter  
#endif
namespace
{

	JPPlatformAdapter* GetAdapter()
	{
		static JPPlatformAdapter* adapter = new PLATFORM_ADAPTER();
		return adapter;
	}
}


#define USE_JNI_VERSION JNI_VERSION_1_4


JPContext::JPContext()
{
}

JPContext::JPContext(const JPContext& orig)
{
}

JPContext::~JPContext()
{
}

bool JPContext::isInitialized()
{
	return m_JavaVM != NULL;
}

/**
	throw a JPypeException if the JVM is not started
 */
void JPContext::assertJVMRunning(const char* function, const JPStackInfo& info)
{
	// FIXME fit function names into raise
	if (!JPEnv::isInitialized())
	{
		throw JPypeException(JPError::_runtime_error, "Java Virtual Machine is not running", info);
	}
}


void JPContext::loadEntryPoints(const string& path)
{
	// Load symbols from the shared library	
	GetAdapter()->loadLibrary((char*) path.c_str());
	CreateJVM_Method = (jint(JNICALL *)(JavaVM **, void **, void *))GetAdapter()->getSymbol("JNI_CreateJavaVM");
	GetCreatedJVMs_Method = (jint(JNICALL *)(JavaVM **, jsize, jsize*))GetAdapter()->getSymbol("JNI_GetCreatedJavaVMs");
}

void JPContext::startJVM(const string& vmPath, char ignoreUnrecognized, const StringVector& args)
{
	JP_TRACE_IN("JPEnv::loadJVM");

	// Get the entry points in the shared library
	loadEntryPoints(vmPath);

	JavaVMInitArgs jniArgs;
	jniArgs.options = NULL;

	// prepare this ...
	jniArgs.version = USE_JNI_VERSION;
	jniArgs.ignoreUnrecognized = ignoreUnrecognized;

	jniArgs.nOptions = (jint) args.size();
	jniArgs.options = (JavaVMOption*) malloc(sizeof (JavaVMOption) * jniArgs.nOptions);
	memset(jniArgs.options, 0, sizeof (JavaVMOption) * jniArgs.nOptions);
	for (int i = 0; i < jniArgs.nOptions; i++)
	{
		jniArgs.options[i].optionString = (char*) args[i].c_str();
	}
	JPEnv::CreateJavaVM((void*) &jniArgs);
	free(jniArgs.options);

	if (m_JavaVM == NULL)
	{
		JP_TRACE("Unable to start");
		JP_RAISE_RUNTIME_ERROR("Unable to start JVM");
	}

	JP_TRACE("Initialize");
	JPJni::init();
	JPTypeFactory::init();
	JPClassLoader::init();
	JPTypeManager::init();
	JPReferenceQueue::init();
	JPProxy::init();
	JPReferenceQueue::startJPypeReferenceQueue(true);
	JP_TRACE_OUT;
}

void JPContext::shutdownJVM()
{
	JP_TRACE_IN("JPEnv::shutdown");
	if (m_JavaVM == NULL)
		JP_RAISE_RUNTIME_ERROR("Attempt to shutdown without a live JVM");

	// Reference queue has to be be shutdown first as we need to close resources
	JPReferenceQueue::shutdown();

	// Then the type manager has to be shut down so that we can't create any more classes
	JPTypeManager::shutdown();

	// Wait for all non-demon threads to terminate
	// DestroyJVM is rather misnamed.  It is simply a wait call
	// FIXME our reference queue thunk does not appear to have properly set 
	// as daemon so we hang here
	JP_TRACE("Destroy JVM");
	//	s_JavaVM->functions->DestroyJavaVM(s_JavaVM);

	// unload the jvm library
	JP_TRACE("Unload JVM");
	GetAdapter()->unloadLibrary();
	m_JavaVM = NULL;
	JP_TRACE_OUT;
}

void JPContext::createJVM(void* arg)
{
	JP_TRACE_IN("JPEnv::CreateJavaVM");
	m_JavaVM = NULL;
	JNIEnv* env;
	CreateJVM_Method(&m_JavaVM, (void**) &env, arg);
	JP_TRACE_OUT;
}


/*****************************************************************************/
// Thread code

void JPContext::attachCurrentThread()
{
	JNIEnv* env;
	jint res = m_JavaVM->functions->AttachCurrentThread(m_JavaVM, (void**) &env, NULL);
	if (res != JNI_OK)
		JP_RAISE_RUNTIME_ERROR("Unable to attach to thread");
}

void JPContext::attachCurrentThreadAsDaemon()
{
	JNIEnv* env;
	jint res = m_JavaVM->functions->AttachCurrentThreadAsDaemon(m_JavaVM, (void**) &env, NULL);
	if (res != JNI_OK)
		JP_RAISE_RUNTIME_ERROR("Unable to attach to thread as daemon");
}

bool JPContext::isThreadAttached()
{
	JNIEnv* env;
	return JNI_OK == m_JavaVM->functions->GetEnv(m_JavaVM, (void**) &env, USE_JNI_VERSION);
}

void JPContext::detachCurrentThread()
{
	m_JavaVM->functions->DetachCurrentThread(m_JavaVM);
}
