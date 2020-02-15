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
#include <jp_thunk.h>

namespace
{ // impl detail, gets initialized by JPProxy::init()
jobject classLoader;
jmethodID findClassID;
}

void JPClassLoader::init()
{
	JPJavaFrame frame;
	JP_TRACE_IN("JPClassLoader::init");

	// Define the class loader
	jobject cl = JPJni::getSystemClassLoader();
	jclass cls = frame.DefineClass("org/jpype/classloader/JPypeClassLoader", cl,
			JPThunk::_org_jpype_classloader_JPypeClassLoader,
			JPThunk::_org_jpype_classloader_JPypeClassLoader_size);

	jvalue v;

	// Set up class loader
	jmethodID ctorID = frame.GetMethodID(cls, "<init>", "(Ljava/lang/ClassLoader;)V");
	if (ctorID == NULL)
		JP_RAISE(PyExc_RuntimeError, "JPypeClassLoader ctor not found");
	//v.l = cl;

	jmethodID getInstanceID = frame.GetStaticMethodID(cls, "getInstance", "()Lorg/jpype/classloader/JPypeClassLoader;");
	classLoader = frame.NewGlobalRef(frame.CallStaticObjectMethod(cls, getInstanceID));

	// Load the jar
	jbyteArray jar = frame.NewByteArray(JPThunk::_org_jpype_size);
	frame.SetByteArrayRegion(jar, 0, JPThunk::_org_jpype_size, JPThunk::_org_jpype);
	v.l = jar;

	jmethodID importJarID = frame.GetMethodID(cls, "importJar", "([B)V");
	frame.CallVoidMethodA(classLoader, importJarID, &v);

	// Set up the loader
	findClassID = frame.GetMethodID(cls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

	JP_TRACE_OUT;
}

jclass JPClassLoader::findClass(string name)
{
	JP_TRACE_IN("JPClassLoader::findClass");
	JPJavaFrame frame;
	jvalue v;
	v.l = frame.NewStringUTF(name.c_str());
	return (jclass) frame.keep(frame.CallObjectMethodA(classLoader, findClassID, &v));
	JP_TRACE_OUT;
}
