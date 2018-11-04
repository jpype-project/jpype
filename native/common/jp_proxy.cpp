/*****************************************************************************
   Copyright 2004 Steve M�nard

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

namespace { // impl detail, gets initialized by JPProxy::init()
	jclass handlerClass;
	jmethodID invocationHandlerConstructorID;
	jfieldID hostObjectID;

class JPCallback
{
public:
	PyGILState_STATE gstate;
	JPCallback()
	{
		gstate = PyGILState_Ensure();
	}
	~JPCallback()
	{
		PyGILState_Release(gstate);
	}
};

}


JNIEXPORT jobject JNICALL Java_jpype_JPypeInvocationHandler_hostInvoke(
	JNIEnv *env, jclass clazz, jstring name, 
	jlong hostObj, jobjectArray args, 
	jobjectArray types, jclass returnType)
{
	TRACE_IN("Java_jpype_JPypeInvocationHandler_hostInvoke");

	JPJavaFrame frame(env);
	JPCallback callback;
	JPCleaner cleaner;

	try {
		string cname = JPJni::asciiFromJava(name);

		HostRef* hostObjRef = (HostRef*)hostObj;

		HostRef* callable = JPEnv::getHost()->getCallableFrom(hostObjRef, cname);
		cleaner.add(callable);

		// If method can't be called, throw an exception
		if (callable == NULL || callable->isNull() || JPEnv::getHost()->isNone(callable))
		{
			frame.ThrowNew(JPJni::s_NoSuchMethodErrorClass, cname.c_str());
			return NULL;
		}
					
		// convert the arguments into a python list
		jsize argLen = frame.GetArrayLength(types);
		vector<HostRef*> hostArgs;

		for (int i = 0; i < argLen; i++)
		{
			jclass c = (jclass)frame.GetObjectArrayElement(types, i);
			JPTypeName t = JPJni::getName(c);

			jobject obj = frame.GetObjectArrayElement(args, i);

			jvalue v;
			v.l = obj;

			HostRef* o = JPTypeManager::getType(t)->asHostObjectFromObject(v);
			cleaner.add(o);
			hostArgs.push_back(o);

		}

		HostRef* returnValue = JPEnv::getHost()->callObject(callable, hostArgs);
		cleaner.add(returnValue);

		JPTypeName returnT = JPJni::getName(returnType);

		if (returnValue == NULL || returnValue->isNull() || JPEnv::getHost()->isNone(returnValue))
		{
			if (returnT.getType() != JPTypeName::_void && returnT.getType() < JPTypeName::_object)
			{
				frame.ThrowNew(JPJni::s_RuntimeExceptionClass, "Return value is None when it cannot be");
				return NULL;
			}
		}

		if (returnT.getType() == JPTypeName::_void)
		{
			return NULL;
		}

		JPType* rt = JPTypeManager::getType(returnT);
		if (rt->canConvertToJava(returnValue) == _none)
		{
			frame.ThrowNew(JPJni::s_RuntimeExceptionClass, "Return value is not compatible with required type.");
			return NULL;
		}
	
		jobject returnObj = rt->convertToJavaObject(returnValue);
		returnObj = frame.NewLocalRef(returnObj); // Add an extra local reference so returnObj survives cleaner
    
		return returnObj;

	}
	catch(HostException& ex)
	{ 
		JPEnv::getHost()->clearError();
		if (JPEnv::getHost()->isJavaException(&ex))
		{
			JPCleaner cleaner;
			HostRef* javaExcRef = JPEnv::getHost()->getJavaException(&ex);
			JPObject* javaExc = JPEnv::getHost()->asObject(javaExcRef);
			cleaner.add(javaExcRef);
			jobject obj = javaExc->getObject();
			frame.Throw((jthrowable)obj);
		}
		else
		{
			// Prepare a message
			string message = "Python exception thrown: ";
			message += ex.getMessage();
			frame.ThrowNew(JPJni::s_RuntimeExceptionClass, message.c_str());
		}
	} 
	catch(JavaException&)
	{ 
		cerr << "Java exception at " << __FILE__ << ":" << __LINE__ << endl; 
	}
	catch(JPypeException& ex)
	{
		frame.ThrowNew(JPJni::s_RuntimeExceptionClass, ex.getMsg());
	}

	return NULL;

	TRACE_OUT;
}



void JPProxy::init()
{
	JPJavaFrame frame(32);
	TRACE_IN("JPProxy::init");

	// build the proxy class ...
	jobject cl = JPJni::getSystemClassLoader();

	jclass handler = frame.DefineClass("jpype/JPypeInvocationHandler", cl, 
			JPThunk::_jpype_JPypeInvocationHandler, 
			JPThunk::_jpype_JPypeInvocationHandler_size);
	handlerClass = (jclass)frame.NewGlobalRef(handler);
	
	JNINativeMethod method[1];
	method[0].name = (char*) "hostInvoke";
	method[0].signature =(char*) "(Ljava/lang/String;J[Ljava/lang/Object;[Ljava/lang/Class;Ljava/lang/Class;)Ljava/lang/Object;";
	method[0].fnPtr = (void*) &Java_jpype_JPypeInvocationHandler_hostInvoke;


	hostObjectID = frame.GetFieldID(handler, "hostObject", "J");
	invocationHandlerConstructorID = frame.GetMethodID(handler, "<init>", "()V");

	frame.RegisterNatives(handlerClass, method, 1);

	TRACE_OUT;
}

JPProxy::JPProxy(HostRef* inst, vector<jclass>& intf)
{
	JPJavaFrame frame;
	m_Instance = inst->copy();
	
	jobjectArray ar = frame.NewObjectArray((int)intf.size(), JPJni::s_ClassClass, NULL);
	m_Interfaces = (jobjectArray)frame.NewGlobalRef(ar);

	for (unsigned int i = 0; i < intf.size(); i++)
	{
		m_InterfaceClasses.push_back((jclass)frame.NewGlobalRef(intf[i]));
		frame.SetObjectArrayElement(m_Interfaces, i, m_InterfaceClasses[i]);
	}
	
	m_Handler = frame.NewGlobalRef(frame.NewObject(handlerClass, invocationHandlerConstructorID));

	frame.SetLongField(m_Handler, hostObjectID, (jlong)inst->copy());
}

JPProxy::~JPProxy()
{
	if (m_Instance != NULL)
	{
		m_Instance->release();
	}
	JPJavaFrame::ReleaseGlobalRef(m_Handler);
	JPJavaFrame::ReleaseGlobalRef(m_Interfaces);

	for (unsigned int i = 0; i < m_InterfaceClasses.size(); i++)
	{
		JPJavaFrame::ReleaseGlobalRef(m_InterfaceClasses[i]);
	}

}

jobject JPProxy::getProxy()
{
	JPJavaFrame frame;
	jobject cl = JPJni::getSystemClassLoader();

	jvalue v[3];
	v[0].l = cl;
	v[1].l = m_Interfaces;
	v[2].l = m_Handler;

	return frame.keep(frame.CallStaticObjectMethodA(JPJni::s_ProxyClass, JPJni::s_NewProxyInstanceID, v));
}

