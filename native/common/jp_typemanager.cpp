// --- file: common/jp_typemanager.cpp ---
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
#include "jpype.h"
#include "jp_classloader.h"

JPTypeManager::JPTypeManager(JPJavaFrame& frame)
{
	m_JavaTypeManager = nullptr;
	JP_TRACE_IN("JPTypeManager::init");
	jclass cls = frame.getContext()->getClassLoader()->findClass(frame, "org.jpype.manager.TypeManager");
	m_FindClass = frame.GetMethodID(cls, "findClass", "(Ljava/lang/Class;)J");
	m_FindClassByName = frame.GetMethodID(cls, "findClassByName", "(Ljava/lang/String;)J");
	m_FindClassForObject = frame.GetMethodID(cls, "findClassForObject", "(Ljava/lang/Object;)J");
	m_PopulateMethod = frame.GetMethodID(cls, "populateMethod", "(JLjava/lang/reflect/Executable;)V");
	m_PopulateMembers = frame.GetMethodID(cls, "populateMembers", "(Ljava/lang/Class;)V");
	m_InterfaceParameterCount = frame.GetMethodID(cls, "interfaceParameterCount", "(Ljava/lang/Class;)I");
	m_MethodFromReflect = frame.GetMethodID(cls, "methodFromReflect", "(Ljava/lang/reflect/Method;)J");

	// The object instance will be loaded later
	JP_TRACE_OUT;
}

JPClass* JPTypeManager::findClass(JPJavaFrame &frame, jclass obj)
{
	JP_TRACE_IN("JPTypeManager::findClass");
	jvalue val;
	val.l = obj;
	JPPyCallRelease release;
	return (JPClass*) (frame.CallLongMethodA(m_JavaTypeManager, m_FindClass, &val));
	JP_TRACE_OUT;
}

JPClass* JPTypeManager::findClassByName(JPJavaFrame& frame, const string& name)
{
	JP_TRACE_IN("JPTypeManager::findClassByName");
	jvalue val;
	val.l = (jobject) frame.fromStringUTF8(name);
	JPPyCallRelease release;
	auto* out = (JPClass*) (frame.CallLongMethodA(m_JavaTypeManager, m_FindClassByName, &val));
	if (out == nullptr)
	{
		std::stringstream err;
		err << "Class " << name << " is not found";
		JP_RAISE(PyExc_TypeError, err.str());
	}
	return out;
	JP_TRACE_OUT;
}

JPClass* JPTypeManager::findClassForObject(JPJavaFrame &frame, jobject obj)
{
	JP_TRACE_IN("JPTypeManager::findClassForObject");
	jvalue val;
	val.l = obj;
	JPPyCallRelease release;
	auto *cls = (JPClass*) (frame.CallLongMethodA(m_JavaTypeManager, m_FindClassForObject, &val));
	frame.check();
	JP_TRACE("ClassName", cls == NULL ? "null" : cls->getCanonicalName());
	return cls;
	JP_TRACE_OUT;
}

void JPTypeManager::populateMethod(JPJavaFrame& frame, void* method, jobject obj)
{
	JP_TRACE_IN("JPTypeManager::populateMethod");
	jvalue val[2];
	val[0].j = (jlong) method;
	val[1].l = obj;
	JP_TRACE("Method", method);
	JPPyCallRelease release;
	frame.CallVoidMethodA(m_JavaTypeManager, m_PopulateMethod, val);
	JP_TRACE_OUT;
}

void JPTypeManager::populateMembers(JPJavaFrame& frame, JPClass* cls)
{
	JP_TRACE_IN("JPTypeManager::populateMembers");
	jvalue val[1];
	val[0].l = (jobject) cls->getJavaClass(frame);
	JPPyCallRelease release;
	frame.CallVoidMethodA(m_JavaTypeManager, m_PopulateMembers, val);
	JP_TRACE_OUT;
}

int JPTypeManager::interfaceParameterCount(JPJavaFrame& frame, JPClass *cls)
{
	JP_TRACE_IN("JPTypeManager::interfaceParameterCount");
	jvalue val[1];
	val[0].l = (jobject) cls->getJavaClass(frame);
	return frame.CallIntMethodA(m_JavaTypeManager, m_InterfaceParameterCount, val);
	JP_TRACE_OUT;
}

JPMethodDispatch* JPTypeManager::methodFromReflect(JPJavaFrame& frame, jobject method)
{
	JP_TRACE_IN("JPTypeManager::methodFromReflect");
	jvalue val;
	val.l = method;
	JPPyCallRelease release;
	auto* out = (JPMethodDispatch*) (frame.CallLongMethodA(m_JavaTypeManager, m_MethodFromReflect, &val));
	frame.check();
	return out;
	JP_TRACE_OUT;
}
