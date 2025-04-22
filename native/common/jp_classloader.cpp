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
#include <jpype.h>
#include <pyjp.h>
#include <jp_classloader.h>

jobject JPClassLoader::getBootLoader()
{
	return m_BootLoader.get();
}

JPClassLoader::JPClassLoader(JPJavaFrame& frame)
{
	JP_TRACE_IN("JPClassLoader::JPClassLoader");

	// Define the class loader
	m_ClassClass = JPClassRef(frame, frame.FindClass("java/lang/Class"));
	m_ForNameID = frame.GetStaticMethodID(m_ClassClass.get(), "forName",
			"(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
	jclass classLoaderClass = frame.FindClass("java/lang/ClassLoader");
	jmethodID getSystemClassLoader
			= frame.GetStaticMethodID(classLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
	m_SystemClassLoader = JPObjectRef(frame,
			frame.CallStaticObjectMethodA(classLoaderClass, getSystemClassLoader, nullptr));

	jclass dynamicLoaderClass = frame.getEnv()->FindClass("org/jpype/JPypeClassLoader");
	if (dynamicLoaderClass != nullptr)
	{
		// Use the one in place already
		if (frame.IsInstanceOf(m_SystemClassLoader.get(), dynamicLoaderClass))
		{
			m_BootLoader = m_SystemClassLoader;
			return;
		}

		// Easy the Dynamic loader is already in the path, so just use it as the bootloader
		jmethodID newDyLoader = frame.GetMethodID(dynamicLoaderClass, "<init>",
				"(Ljava/lang/ClassLoader;)V");
		jvalue v;
		v.l = m_SystemClassLoader.get();
		m_BootLoader = JPObjectRef(frame, frame.NewObjectA(dynamicLoaderClass, newDyLoader, &v));
		return;
	}
	frame.ExceptionClear();

	// org.jpype was not loaded already so we can't proceed
	JP_RAISE(PyExc_RuntimeError, "Can't find org.jpype.jar support library");
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
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
	v[2].l = m_BootLoader.get();
	return (jclass) frame.CallStaticObjectMethodA(m_ClassClass.get(), m_ForNameID, v);
#endif
}
