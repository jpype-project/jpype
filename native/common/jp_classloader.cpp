/*****************************************************************************
   Copyright 2018 Karl Nelson

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
#include <Python.h>
#include <jpype.h>
#include <jp_classloader.h>
#include <jp_thunk.h>

jobject JPClassLoader::getSystemClassLoader()
{
	return m_SystemClassLoader.get();
}

jobject JPClassLoader::getBootLoader()
{
	return m_BootLoader.get();
}

JPClassLoader::JPClassLoader(JPContext* context, bool useSystem)
{
	m_Context = context;
	JPJavaFrame frame(context);
	JP_TRACE_IN("JPClassLoader::init");

	// Define the class loader
	jclass classLoaderClass = (jclass) frame.FindClass("java/lang/ClassLoader");
	jmethodID getSystemClassLoader
			= frame.GetStaticMethodID(classLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
	m_SystemClassLoader = JPObjectRef(context, frame.CallStaticObjectMethod(classLoaderClass, getSystemClassLoader));
	m_UseSystem = useSystem;
	// Set up the loader
	m_FindClass = frame.GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	if (useSystem)
	{
		// Boot loader and system class loader are now the same
		m_BootLoader = m_SystemClassLoader;
		return;
	}
	else
	{
		jclass cls = frame.DefineClass("org/jpype/classloader/JPypeClassLoader", m_SystemClassLoader.get(),
				JPThunk::_org_jpype_classloader_JPypeClassLoader,
				JPThunk::_org_jpype_classloader_JPypeClassLoader_size);


		// Set up class loader
		frame.GetMethodID(cls, "<init>", "(Ljava/lang/ClassLoader;)V");

		jmethodID getInstanceID = frame.GetStaticMethodID(cls, "getInstance", "()Lorg/jpype/classloader/JPypeClassLoader;");
		m_BootLoader = JPObjectRef(context, frame.NewGlobalRef(frame.CallStaticObjectMethod(cls, getInstanceID)));

		// Load the jar
		jbyteArray jar = frame.NewByteArray(JPThunk::_org_jpype_size);
		frame.SetByteArrayRegion(jar, 0, JPThunk::_org_jpype_size, JPThunk::_org_jpype);

		jvalue v;
		v.l = jar;
		jmethodID importJarID = frame.GetMethodID(cls, "importJar", "([B)V");
		frame.CallVoidMethodA(m_BootLoader.get(), importJarID, &v);

	}
	JP_TRACE_OUT;
}

jclass JPClassLoader::findClass(string name)
{
	JPJavaFrame frame(m_Context);
	jvalue v;
	v.l = frame.NewStringUTF(name.c_str());
	return (jclass) frame.keep(frame.CallObjectMethodA(m_BootLoader.get(), m_FindClass, &v));
}
