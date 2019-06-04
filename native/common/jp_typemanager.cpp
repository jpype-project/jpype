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
#include <jpype.h>

JPTypeManager::JPTypeManager(JPContext* context)
{
	m_Context = context;
	// Everything that requires specialization must be created here.
	JPJavaFrame frame(context);
	JP_TRACE_IN("JPTypeManager::init");

	jclass cls = context->getClassLoader()->findClass("org.jpype.manager.TypeManager");
	m_FindClass = frame.GetMethodID(cls, "findClass", "(Ljava/lang/Class;)J");
	m_FindClassByName = frame.GetMethodID(cls, "findClassByName", "(Ljava/lang/String;)J");
	m_FindClassForObject = frame.GetMethodID(cls, "findClassForObject", "(Ljava/lang/Object;)J");

	// The object instance will be loaded later
	JP_TRACE_OUT;
}

JPTypeManager::~JPTypeManager()
{
}

JPClass* JPTypeManager::findClass(jclass obj)
{
	JP_TRACE_IN("JPTypeManager::findClass");
	JPJavaFrame frame(m_Context);
	jvalue val;
	val.l = obj;
	return (JPClass*) (frame.CallLongMethodA(m_JavaTypeManager.get(), m_FindClass, &val));
	JP_TRACE_OUT;
}

JPClass* JPTypeManager::findClassByName(const string& name)
{
	JP_TRACE_IN("JPTypeManager::findClassByName");
	JPJavaFrame frame(m_Context);
	jvalue val;
	val.l = (jobject) m_Context->fromStringUTF8(name);
	return (JPClass*) (frame.CallLongMethodA(m_JavaTypeManager.get(), m_FindClassByName, &val));
	JP_TRACE_OUT;
}

JPClass* JPTypeManager::findClassForObject(jobject obj)
{
	JP_TRACE_IN("JPTypeManager::findClass");
	JPJavaFrame frame(m_Context);
	jvalue val;
	val.l = obj;
	return (JPClass*) (frame.CallLongMethodA(m_JavaTypeManager.get(), m_FindClassForObject, &val));
	JP_TRACE_OUT;
}

