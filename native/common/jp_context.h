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

class JPContext
{
public:
	JPContext();
	JPContext(const JPContext& orig);
	virtual ~JPContext();

	bool isInitialized();
	void assertJVMRunning(const char* function, const JPStackInfo& info);
	void startJVM(const string& vmPath, char ignoreUnrecognized, const StringVector& args);
	void shutdownJVM();
	void attachCurrentThread();
	void attachCurrentThreadAsDaemon();
	bool isThreadAttached();
	void detachCurrentThread();

private:

	void loadEntryPoints(const string& path);	
	void createJVM(void* arg);	// JVM
	
	JavaVM* m_JavaVM;

	jint(JNICALL * CreateJVM_Method)(JavaVM **pvm, void **penv, void *args);
	jint(JNICALL * GetCreatedJVMs_Method)(JavaVM **pvm, jsize size, jsize * nVms);

public:
	// Class resources defined by TypeManager
	JPVoidType* _void;
	JPBooleanType* _boolean;
	JPByteType* _byte;
	JPCharType* _char;
	JPShortType* _short;
	JPIntType* _int;
	JPLongType* _long;
	JPFloatType* _float;
	JPDoubleType* _double;
	JPClass* _java_lang_Object;
	JPClass* _java_lang_Class;
	JPStringClass* _java_lang_String;

	JPBoxedClass* _java_lang_Void;
	JPBoxedClass* _java_lang_Boolean;
	JPBoxedClass* _java_lang_Byte;
	JPBoxedClass* _java_lang_Char;
	JPBoxedClass* _java_lang_Short;
	JPBoxedClass* _java_lang_Integer;
	JPBoxedClass* _java_lang_Long;
	JPBoxedClass* _java_lang_Float;
	JPBoxedClass* _java_lang_Double;
private:
	JPObjectRef m_JavaContext;
	JPTypeFactory m_TypeFactory;
	JPTypeManager m_TypeManager;
	JPReferenceQueue m_ReferenceQueue;
	
} ;

#endif /* JP_CONTEXT_H */

