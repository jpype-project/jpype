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
	JP_TRACE_IN("JPTypeManager::init");
	jclass cls = frame.getContext()->getClassLoader()->findClass(frame, "org.jpype.manager.TypeManager");
	m_FindClass = frame.GetMethodID(cls, "findClass", "(Ljava/lang/Class;)J");
	m_FindClassByName = frame.GetMethodID(cls, "findClassByName", "(Ljava/lang/String;)J");
	m_FindClassForObject = frame.GetMethodID(cls, "findClassForObject", "(Ljava/lang/Object;)J");
	m_PopulateMethod = frame.GetMethodID(cls, "populateMethod", "(JLjava/lang/reflect/Executable;)V");
	m_PopulateMembers = frame.GetMethodID(cls, "populateMembers", "(Ljava/lang/Class;)V");
    m_InterfaceParameterCount = frame.GetMethodID(cls, "interfaceParameterCount", "(Ljava/lang/Class;)I");

	// The object instance will be loaded later
	JP_TRACE_OUT;
}

JPClass* JPTypeManager::findClass(jclass obj)
{
	JP_TRACE_IN("JPTypeManager::findClass");
	JPJavaFrame frame = JPJavaFrame::outer();
	jvalue val;
	val.l = obj;
	return (JPClass*) (frame.CallLongMethodA(m_JavaTypeManager.get(), m_FindClass, &val));
	JP_TRACE_OUT;
}

JPClass* JPTypeManager::findClassByName(const string& name)
{
	JP_TRACE_IN("JPTypeManager::findClassByName");
	JPJavaFrame frame = JPJavaFrame::outer();
	jvalue val;
	val.l = (jobject) frame.fromStringUTF8(name);
	auto* out = (JPClass*) (frame.CallLongMethodA(m_JavaTypeManager.get(), m_FindClassByName, &val));
	if (out == nullptr)
	{
		std::stringstream err;
		err << "Class " << name << " is not found";
		JP_RAISE(PyExc_TypeError, err.str());
	}
	return out;
	JP_TRACE_OUT;
}

JPClass* JPTypeManager::findClassForObject(jobject obj)
{
	JP_TRACE_IN("JPTypeManager::findClassForObject");
	JPJavaFrame frame = JPJavaFrame::outer();
	jvalue val;
	val.l = obj;
	auto *cls = (JPClass*) (frame.CallLongMethodA(m_JavaTypeManager.get(), m_FindClassForObject, &val));
	frame.check();
	JP_TRACE("ClassName", cls == NULL ? "null" : cls->getCanonicalName());
	return cls;
	JP_TRACE_OUT;
}

void JPTypeManager::populateMethod(void* method, jobject obj)
{
	JP_TRACE_IN("JPTypeManager::populateMethod");
	JPJavaFrame frame = JPJavaFrame::outer();
	jvalue val[2];
	val[0].j = (jlong) method;
	val[1].l = obj;
	JP_TRACE("Method", method);
	frame.CallVoidMethodA(m_JavaTypeManager.get(), m_PopulateMethod, val);
	JP_TRACE_OUT;
}

void JPTypeManager::populateMembers(JPClass* cls)
{
	JP_TRACE_IN("JPTypeManager::populateMembers");
	JPJavaFrame frame = JPJavaFrame::outer();
	jvalue val[1];
	val[0].l = (jobject) cls->getJavaClass();
	frame.CallVoidMethodA(m_JavaTypeManager.get(), m_PopulateMembers, val);
	JP_TRACE_OUT;
}

int JPTypeManager::interfaceParameterCount(JPClass *cls)
{
	JP_TRACE_IN("JPTypeManager::interfaceParameterCount");
	JPJavaFrame frame = JPJavaFrame::outer();
	jvalue val[1];
	val[0].l = (jobject) cls->getJavaClass();
	return frame.CallIntMethodA(m_JavaTypeManager.get(), m_InterfaceParameterCount, val);
	JP_TRACE_OUT;
}
