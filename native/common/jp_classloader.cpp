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

jobject JPClassLoader::getBootLoader()
{
	return m_BootLoader.get();
}

JPClassLoader::JPClassLoader(JPJavaFrame& frame)
{
	JP_TRACE_IN("JPClassLoader::JPClassLoader");
	m_Context = frame.getContext();

	// Define the class loader
	jclass classLoaderClass = (jclass) frame.FindClass("java/lang/ClassLoader");
	jmethodID getSystemClassLoader
			= frame.GetStaticMethodID(classLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
	m_SystemClassLoader = JPObjectRef(frame,
			frame.CallStaticObjectMethodA(classLoaderClass, getSystemClassLoader, 0));
	// Set up the loader
	m_FindClass = frame.GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	jclass cls = frame.DefineClass("org/jpype/classloader/JPypeClassLoader", m_SystemClassLoader.get(),
			JPThunk::_org_jpype_classloader_JPypeClassLoader,
			JPThunk::_org_jpype_classloader_JPypeClassLoader_size);


	// Set up class loader
	frame.GetMethodID(cls, "<init>", "(Ljava/lang/ClassLoader;)V");

	jmethodID getInstanceID = frame.GetStaticMethodID(cls, "getInstance", "()Lorg/jpype/classloader/JPypeClassLoader;");
	m_BootLoader = JPObjectRef(frame, frame.NewGlobalRef(
			frame.CallStaticObjectMethodA(cls, getInstanceID, 0)));

	// Load the jar
	jbyteArray jar = frame.NewByteArray(JPThunk::_org_jpype_size);
	frame.SetByteArrayRegion(jar, 0, JPThunk::_org_jpype_size, JPThunk::_org_jpype);

	jvalue v;
	v.l = jar;
	jmethodID importJarID = frame.GetMethodID(cls, "importJar", "([B)V");
	frame.CallVoidMethodA(m_BootLoader.get(), importJarID, &v);
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}

jclass JPClassLoader::findClass(JPJavaFrame& frame, string name)
{
	jvalue v;
	v.l = frame.NewStringUTF(name.c_str());
	return (jclass) frame.CallObjectMethodA(m_BootLoader.get(), m_FindClass, &v);
}
