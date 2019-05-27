/*****************************************************************************
   Copyright 2019 Karl Einar Nelson

   Licensed under the Apache License, Version 2.0 (the "License
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
#include <jp_thunk.h>
#include <jp_primitive_common.h>

void JPTypeFactory_rethrow(JPJavaFrame& frame)
{
	try {
		throw;
	} catch (JPypeException& ex)
	{
		ex.toJava();
	} catch (...)
	{
		frame.ThrowNew(JPJni::s_RuntimeExceptionClass, "unknown error occurred");
	}
}

JNIEXPORT void JNICALL JPTypeFactory_destroy(JNIEnv *env, jobject self, jlongArray resources, jint sz)
{
	JP_TRACE_IN("JPTypeFactory_destroy");
	JPJavaFrame frame(env);
	try {
		JPPrimitiveArrayAccessor<jlongArray, jlong*> accessor(frame, resources,
				&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);
	  jlong* values = accessor.get();
		for (int i=0; i<sz; ++i)
		{
			delete (JPResource*)values[i];
		}
		return;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return;
	JP_TRACE_OUT;
}


JNIEXPORT jlong JNICALL JPTypeFactory_defineMethodDispatch(JNIEnv *env, jobject self, jlong cls, jstring name, jlongArray overloadList, jlong modifiers)
{
	JP_TRACE_IN("JPTypeFactory_defineMethodDispatch");
	JPJavaFrame frame(env);
	try {


		return 0;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT;
}


JNIEXPORT jlong JNICALL JPTypeFactory_defineArrayClass(JNIEnv *env, jobject self, jclass cls, jstring name, jlong superClass, jlong componentPtr, jlong modifiers)
{
	JP_TRACE_IN("JPTypeFactory_defineArrayClass");
	JPJavaFrame frame(env);
	try {


		return 0;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT;
}


JNIEXPORT jlong JNICALL JPTypeFactory_defineObjectClass(JNIEnv *env, jobject self, jclass cls, jstring name, jlong superClass, jlongArray interfaces, jlong modifiers)
{
	JP_TRACE_IN("JPTypeFactory_defineObjectClass");
	JPJavaFrame frame(env);
	try {


		return 0;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT;
}


JNIEXPORT jlong JNICALL JPTypeFactory_definePrimitive(JNIEnv *env, jobject self, jint code, jclass cls, jlong boxedPtr, jlong modifiers)
{
	JP_TRACE_IN("JPTypeFactory_definePrimitive");
	JPJavaFrame frame(env);
	try {


		return 0;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT;
}


JNIEXPORT void JNICALL JPTypeFactory_assignMembers(JNIEnv *env, jobject self, jlong cls, jlong ctorMethod, jlongArray methodList, jlongArray fieldList)
{
	JP_TRACE_IN("JPTypeFactory_assignMembers");
	JPJavaFrame frame(env);
	try {


		return;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return;
	JP_TRACE_OUT;
}


JNIEXPORT jlong JNICALL JPTypeFactory_defineField(JNIEnv *env, jobject self, jlong cls, jstring name, jobject field, jlong fieldType, jlong modifiers)
{
	JP_TRACE_IN("JPTypeFactory_defineField");
	JPJavaFrame frame(env);
	try {


		return 0;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT;
}


JNIEXPORT jlong JNICALL JPTypeFactory_defineMethod(JNIEnv *env, jobject self, jlong cls, jstring name, jobject method, jlong returnType, jlongArray argumentTypes, jlongArray overloadList, jlong modifiers)
{
	JP_TRACE_IN("JPTypeFactory_defineMethod");
	JPJavaFrame frame(env);
	try {


		return 0;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT;
}



void JPTypeFactory::init()
{
	JPJavaFrame frame(32);
	JP_TRACE_IN("JPTypeFactory::init");

	jclass cls = JPClassLoader::findClass("org.jpype.manager.TypeFactoryNative");

	JNINativeMethod method[8];

	method[0].name = (char*) "destroy";
	method[0].signature = (char*) "([JI)V";
	method[0].fnPtr = (void*) &JPTypeFactory_destroy;

	method[1].name = (char*) "defineMethodDispatch";
	method[1].signature = (char*) "(JLjava.lang.String;[JJ)J";
	method[1].fnPtr = (void*) &JPTypeFactory_defineMethodDispatch;

	method[2].name = (char*) "defineArrayClass";
	method[2].signature = (char*) "(Ljava.lang.Class;Ljava.lang.String;JJJ)J";
	method[2].fnPtr = (void*) &JPTypeFactory_defineArrayClass;

	method[3].name = (char*) "defineObjectClass";
	method[3].signature = (char*) "(Ljava.lang.Class;Ljava.lang.String;J[JJ)J";
	method[3].fnPtr = (void*) &JPTypeFactory_defineObjectClass;

	method[4].name = (char*) "definePrimitive";
	method[4].signature = (char*) "(ILjava.lang.Class;JJ)J";
	method[4].fnPtr = (void*) &JPTypeFactory_definePrimitive;

	method[5].name = (char*) "assignMembers";
	method[5].signature = (char*) "(JJ[J[J)V";
	method[5].fnPtr = (void*) &JPTypeFactory_assignMembers;

	method[6].name = (char*) "defineField";
	method[6].signature = (char*) "(JLjava.lang.String;Ljava.lang.reflect.Field;JJ)J";
	method[6].fnPtr = (void*) &JPTypeFactory_defineField;

	method[7].name = (char*) "defineMethod";
	method[7].signature = (char*) "(JLjava.lang.String;Ljava.lang.reflect.Executable;J[J[JJ)J";
	method[7].fnPtr = (void*) &JPTypeFactory_defineMethod;


	frame.GetMethodID(cls, "<init>", "()V");
	frame.RegisterNatives(cls, method, 8);
	JP_TRACE_OUT;
}



