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

static jobject toURL(JPJavaFrame &frame, const string& path)
{
	//  file = new File("org.jpype.jar");
	jclass fileClass = frame.FindClass("java/io/File");
	jmethodID newFile = frame.GetMethodID(fileClass, "<init>", "(Ljava/lang/String;)V");
	jvalue v[3];
	v[0].l = frame.NewStringUTF(path.c_str());
	jobject file = frame.NewObjectA(fileClass, newFile, v);

	// url = file.toURI().toURL();
	jmethodID toURI = frame.GetMethodID(fileClass, "toURI", "()Ljava/net/URI;");
	jobject uri = frame.CallObjectMethodA(file, toURI, NULL);
	jclass uriClass = frame.GetObjectClass(uri);
	jmethodID toURL = frame.GetMethodID(uriClass, "toURL", "()Ljava/net/URL;");
	return frame.CallObjectMethodA(uri, toURL, NULL);
}

JPClassLoader::JPClassLoader(JPJavaFrame& frame)
{
	JP_TRACE_IN("JPClassLoader::JPClassLoader");
	m_Context = frame.getContext();

	// Define the class loader
	m_ClassClass = JPClassRef(frame, frame.FindClass("java/lang/Class"));
	m_ForNameID = frame.GetStaticMethodID(m_ClassClass.get(), "forName",
			"(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
	jclass classLoaderClass = frame.FindClass("java/lang/ClassLoader");
	jmethodID getSystemClassLoader
			= frame.GetStaticMethodID(classLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
	m_SystemClassLoader = JPObjectRef(frame,
			frame.CallStaticObjectMethodA(classLoaderClass, getSystemClassLoader, 0));

	jclass dynamicLoaderClass = frame.getEnv()->FindClass("org/jpype/classloader/DynamicClassLoader");
	if (dynamicLoaderClass != NULL)
	{
		// Easy the Dynamic loader is already in the path, so just use it as the bootloader
		jmethodID newDyLoader = frame.GetMethodID(dynamicLoaderClass, "<init>",
				"(Ljava/lang/ClassLoader;)V");
		jvalue v;
		v.l = m_SystemClassLoader.get();
		m_BootLoader = JPObjectRef(frame, frame.NewObjectA(dynamicLoaderClass, newDyLoader, &v));
		return;
	}
	frame.ExceptionClear();

	// Harder, we need to find the _jpype module and use __file__ to obtain a
	// path.
	JPPyObject pypath = JPPyObject::call(PyObject_GetAttrString(PyJPModule, "__file__"));
	string path = JPPyString::asStringUTF8(pypath.get());
	string::size_type i = path.find_last_of('\\');
	if (i == string::npos)
		i = path.find_last_of('/');
	if (i == string::npos)
		JP_RAISE(PyExc_RuntimeError, "Can't find jar path");
	path = path.substr(0, i + 1);
	jobject url1 = toURL(frame, path + "org.jpype.jar");
	//	jobject url2 = toURL(frame, path + "lib/asm-8.0.1.jar");

	// urlArray = new URL[]{url};
	jclass urlClass = frame.GetObjectClass(url1);
	jobjectArray urlArray = frame.NewObjectArray(1, urlClass, NULL);
	frame.SetObjectArrayElement(urlArray, 0, url1);
	//	frame.SetObjectArrayElement(urlArray, 1, url2);

	// cl = new URLClassLoader(urlArray);
	jclass urlLoaderClass = frame.FindClass("java/net/URLClassLoader");
	jmethodID newURLClassLoader = frame.GetMethodID(urlLoaderClass, "<init>", "([Ljava/net/URL;Ljava/lang/ClassLoader;)V");
	jvalue v[3];
	v[0].l = (jobject) urlArray;
	v[1].l = (jobject) m_SystemClassLoader.get();
	jobject cl = frame.NewObjectA(urlLoaderClass, newURLClassLoader, v);

	// Class dycl = Class.forName("org.jpype.classloader.DynamicClassLoader", true, cl);
	v[0].l = frame.NewStringUTF("org.jpype.classloader.DynamicClassLoader");
	v[1].z = true;
	v[2].l = cl;
	jclass dyClass = (jclass) frame.CallStaticObjectMethodA(m_ClassClass.get(), m_ForNameID, v);

	// dycl.newInstance(systemClassLoader);
	jmethodID newDyLoader = frame.GetMethodID(dyClass, "<init>", "(Ljava/lang/ClassLoader;)V");
	v[0].l = cl;
	m_BootLoader = JPObjectRef(frame, frame.NewObjectA(dyClass, newDyLoader, v));

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
