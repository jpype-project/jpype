// --- file: common/jp_classloader.cpp ---
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
#include <Python.h>
#include <mutex>
#include <jpype.h>
#include <pyjp.h>
#include <jp_classloader.h>
#include <jp_context.h>

// True process-wide globals (like s_GlobalPoolClass in jp_context.cpp),
// resolved once regardless of how many JPContext/subinterpreters come and
// go - there is one JVM, so java.lang.Class and the system classloader are
// the same object every time.  Never released - they live for the process.
static jclass s_ClassClass = nullptr;
static jobject s_SystemClassLoader = nullptr;
static jmethodID s_ForNameID = nullptr;

static void bindImmortals(JPJavaFrame& frame)
{
	static std::once_flag lookupOnce;
	std::call_once(lookupOnce, [&frame]()
	{
		s_ClassClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/Class"));
		s_ForNameID = frame.GetStaticMethodID(s_ClassClass, "forName",
				"(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
		jclass classLoaderClass = frame.FindClass("java/lang/ClassLoader");
		jmethodID getSystemClassLoader
				= frame.GetStaticMethodID(classLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
		s_SystemClassLoader = frame.NewGlobalRef(
				frame.CallStaticObjectMethodA(classLoaderClass, getSystemClassLoader, nullptr));
	});
}

jobject JPClassLoader::getBootLoader()
{
	return m_BootLoader;
}

JPClassLoader::JPClassLoader(JPJavaFrame& frame)
{
	JP_TRACE_IN("JPClassLoader::JPClassLoader");

	bindImmortals(frame);

	jclass dynamicLoaderClass = frame.getEnv()->FindClass("org/jpype/internal/DynamicClassLoader");
	if (dynamicLoaderClass != nullptr)
	{
		// Use the one in place already
		if (frame.IsInstanceOf(s_SystemClassLoader, dynamicLoaderClass))
		{
			m_BootLoader = frame.NewGlobalRef(s_SystemClassLoader);
			return;
		}

		// Easy the Dynamic loader is already in the path, so just use it as the bootloader
		jmethodID newDyLoader = frame.GetMethodID(dynamicLoaderClass, "<init>",
				"(Ljava/lang/ClassLoader;)V");
		jvalue v;
		v.l = s_SystemClassLoader;
		m_BootLoader = frame.NewGlobalRef(frame.NewObjectA(dynamicLoaderClass, newDyLoader, &v));
		return;
	}
	frame.ExceptionClear();

	// org.jpype was not loaded already so we can't proceed
	JP_RAISE(PyExc_RuntimeError, "Can't find org.jpype.jar support library");
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}

void JPClassLoader::release(JPContext* context)
{
	context->ReleaseGlobalRef(m_BootLoader);
	m_BootLoader = nullptr;
}

jclass JPClassLoader::findClass(JPJavaFrame& frame, const string& name)
{
#ifdef ANDROID
	string cname = name;
	for (int i = 0; i < cname.size(); ++i)
		if (cname[i] == '.')
			cname[i] = '/';
	return frame.FindClass(cname);
#else
	jvalue v[3];
	v[0].l = frame.NewStringUTF(name.c_str());
	v[1].z = true;
	v[2].l = m_BootLoader;
	return (jclass) frame.CallStaticObjectMethodA(s_ClassClass, s_ForNameID, v);
#endif
}
