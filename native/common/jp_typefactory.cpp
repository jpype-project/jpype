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
//#include <jp_thunk.h>
#include <jp_primitive_common.h>

void JPTypeFactory_rethrow(JPJavaFrame& frame)
{
	try
	{
		throw;
	} catch (JPypeException& ex)
	{
		ex.toJava();
	} catch (...)
	{
		frame.ThrowNew(JPJni::s_RuntimeExceptionClass, "unknown error occurred");
	}
}

template <class T> convert(JPJavaFrame& frame, jlongArray array, vector<T>& out)
{
	JPPrimitiveArrayAccessor<jlongArray, jlong*> accessor(frame, array,
			&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);
	jlong* values = accessor.get();
	jsize sz = frame.GetArrayLength(array);
	out.resize(sz);
	for (int i = 0; i < sz; ++i)
	{
		out[i] = (T*) values[i];
	}
	return;
}

JNIEXPORT void JNICALL JPTypeFactory_destroy(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlongArray resources,
		jint sz)
{
	JP_TRACE_IN("JPTypeFactory_destroy");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(env);
	try
	{
		JPPrimitiveArrayAccessor<jlongArray, jlong*> accessor(frame, resources,
				&JPJavaFrame::GetLongArrayElements, &JPJavaFrame::ReleaseLongArrayElements);
		jlong* values = accessor.get();
		for (int i = 0; i < sz; ++i)
		{
			delete (JPResource*) values[i];
		}
		return;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return;
	JP_TRACE_OUT;
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineMethodDispatch(
		JNIEnv *env, jobject self, jlong contextPtr,
		jstring nameJString,
		jlongArray overloadPtrs,
		jint modifiers)
{
	JP_TRACE_IN("JPTypeFactory_defineMethodDispatch");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(env);
	try
	{
		JPClass* cls = (JPClass*) clsPtr;
		JPMethodList overloadList;
		convert(overloadPtrs, overloadList);
		string name = JPJni::toStringUTF8(nameJString);
		JPMethodDispatch* dispatch = new JPMethodDispatch(cls, name, overloadList, modifiers);
		return (jlong) dispatch;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT;
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineArrayClass(
		JNIEnv *env, jobject self, jlong contextPtr,
		jstring name,
		jlong superClass,
		jlong componentClass,
		jint modifiers)
{
	JP_TRACE_IN("JPTypeFactory_defineArrayClass");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(env);
	try
	{
		string className = JPJni::toStringUTF8(name);
		JPArrayClass* result = new JPArrayClass(cls,
				className,
				(JPClass*) superClass,
				(JPClass*) componentClass,
				modifiers);
		return (jlong) result;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT;
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineObjectClass(
		JNIEnv *env, jobject self, jlong contextPtr,
		jclass cls,
		jstring name,
		jlong superClass,
		jlongArray interfacePtrs,
		jint modifiers)
{
	JP_TRACE_IN("JPTypeFactory_defineObjectClass");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(env);
	try
	{
		string className = JPJni::toStringUTF8(name);
		JPMethodList interfaces;
		convert(interfacePtrs, interfaces);
		JPClass* result = NULL;
		if (JPModifier::isSpecial(modifiers))
		{
			// Certain classes require special implementations
			if (className == "java.lang.Object")
				return (jlong) (JPTypeManager::_java_lang_Object
					= new JPObjectType(cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.Class")
				return (jlong) (JPTypeManager::_java_lang_Class
					= new JPClassType(cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.Void")
				return (jlong) (JPTypeManager::_java_lang_Class
					= new JPBoxedVoidType(cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.Boolean")
				return (jlong) (JPTypeManager::_java_lang_Class
					= new JPBoxedBooleanType(cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.Byte")
				return (jlong) (JPTypeManager::_java_lang_Class
					= new JPBoxedByteType(cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.Character")
				return (jlong) (JPTypeManager::_java_lang_Class
					= new JPBoxedCharacterType(cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.Short")
				return (jlong) (JPTypeManager::_java_lang_Class
					= new JPBoxedShortType(cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.Integer")
				return (jlong) (JPTypeManager::_java_lang_Class
					= new JPBoxedIntegerType(cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.Long")
				return (jlong) (JPTypeManager::_java_lang_Class
					= new JPBoxedLongType(cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.Float")
				return (jlong) (JPTypeManager::_java_lang_Class
					= new JPBoxedFloatType(cls, className, (JPClass*) superClass, interfaces, modifiers));
			if (className == "java.lang.Double")
				return (jlong) new JPBoxedDoubleType(cls, className, (JPClass*) superClass, interfaces, modifiers);
		}
		else
			// Otherwise create a normal class
			result = new JPClass(cls, className, (JPClass*) superClass, interfaces, modifiers);
		return (jlong) result;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT;
}

JNIEXPORT jlong JNICALL JPTypeFactory_definePrimitive(
		JNIEnv *env, jobject self, jlong contextPtr,
		jstring name,
		jclass cls,
		jlong boxedPtr,
		jint modifiers)
{
	JP_TRACE_IN("JPTypeFactory_definePrimitive");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(env);
	try
	{
		string className = JPJni::toStringUTF8(name);
		if (className == "void")
			return (jlong) (JPTypeManager::_void = new JPVoidType(name, cls, (JPBoxedClass*) boxedPtr, modifiers));
		if (className == "byte")
			return (jlong) (JPTypeManager::_byte = new JPByteType(name, cls, (JPBoxedClass*) boxedPtr, modifiers));
		if (className == "boolean")
			return (jlong) (JPTypeManager::_boolean = new JPBooleanType(name, cls, (JPBoxedClass*) boxedPtr, modifiers));
		if (className == "char")
			return (jlong) (JPTypeManager::_char = new JPCharType(name, cls, (JPBoxedClass*) boxedPtr, modifiers));
		if (className == "short")
			return (jlong) (JPTypeManager::_short = new JPShortType(name, cls, (JPBoxedClass*) boxedPtr, modifiers));
		if (className == "int")
			return (jlong) (JPTypeManager::_int = new JPIntType(name, cls, (JPBoxedClass*) boxedPtr, modifiers));
		if (className == "long")
			return (jlong) (JPTypeManager::_long = new JPLongType(name, cls, (JPBoxedClass*) boxedPtr, modifiers));
		if (className == "float")
			return (jlong) (JPTypeManager::_float = new JPFloatType(name, cls, (JPBoxedClass*) boxedPtr, modifiers));
		if (className == "double")
			return (jlong) (JPTypeManager::_double = new JPDoubleType(name, cls, (JPBoxedClass*) boxedPtr, modifiers));
		return 0;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT;
}

JNIEXPORT void JNICALL JPTypeFactory_assignMembers(JNIEnv *env, jobject self,
		jlong contextPtr,
		jlong clsPtr,
		jlong ctorMethod,
		jlongArray methodPtrs,
		jlongArray fieldPtrs)
{
	JP_TRACE_IN("JPTypeFactory_assignMembers");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(env);
	try
	{
		JPClass* cls = (JPClass*) clsPtr;
		JPMethodDispatch methodList;
		convert(methodPtrs, methodList);

		JPFieldList fieldList;
		convert(fieldPtrs, fieldList);
		cls->assignMembers(
				(JPMethodDispatch*) ctorMethod,
				methodList,
				fieldList);


		return;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return;
	JP_TRACE_OUT;
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineField(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong cls,
		jstring name,
		jobject field,
		jlong fieldType,
		jint modifiers)
{
	JP_TRACE_IN("JPTypeFactory_defineField");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(env);
	try
	{


		return 0;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT;
}

JNIEXPORT jlong JNICALL JPTypeFactory_defineMethod(
		JNIEnv *env, jobject self, jlong contextPtr,
		jlong cls, jstring name,
		jobject method,
		jlong returnType,
		jlongArray argumentTypes,
		jlongArray overloadList, jint modifiers)
{
	JP_TRACE_IN("JPTypeFactory_defineMethod");
	JPContext* context = (JPContext*) contextPtr;
	JPJavaFrame frame(env);
	try
	{


		return 0;
	} catch (...)
	{
		JPTypeFactory_rethrow(frame);
	}
	return 0;
	JP_TRACE_OUT;
}

void JPTypeFactory::init(JPContext* context)
{
	JPJavaFrame frame(context, 32);
	JP_TRACE_IN("JPTypeFactory::init");

	jclass cls = JPClassLoader::findClass("org.jpype.manager.TypeFactoryNative");

	JNINativeMethod method[8];

	method[0].name = (char*) "destroy";
	method[0].signature = (char*) "(J[JI)V";
	method[0].fnPtr = (void*) &JPTypeFactory_destroy;

	method[1].name = (char*) "defineMethodDispatch";
	method[1].signature = (char*) "(JJLjava.lang.String;[JI)J";
	method[1].fnPtr = (void*) &JPTypeFactory_defineMethodDispatch;

	method[2].name = (char*) "defineArrayClass";
	method[2].signature = (char*) "(JLjava.lang.Class;Ljava.lang.String;JJI)J";
	method[2].fnPtr = (void*) &JPTypeFactory_defineArrayClass;

	method[3].name = (char*) "defineObjectClass";
	method[3].signature = (char*) "(JLjava.lang.Class;Ljava.lang.String;J[JI)J";
	method[3].fnPtr = (void*) &JPTypeFactory_defineObjectClass;

	method[4].name = (char*) "definePrimitive";
	method[4].signature = (char*) "(JLjava.lang.String;Ljava.lang.Class;JI)J";
	method[4].fnPtr = (void*) &JPTypeFactory_definePrimitive;

	method[5].name = (char*) "assignMembers";
	method[5].signature = (char*) "(JJJ[J[J)V";
	method[5].fnPtr = (void*) &JPTypeFactory_assignMembers;

	method[6].name = (char*) "defineField";
	method[6].signature = (char*) "(JJLjava.lang.String;Ljava.lang.reflect.Field;JI)J";
	method[6].fnPtr = (void*) &JPTypeFactory_defineField;

	method[7].name = (char*) "defineMethod";
	method[7].signature = (char*) "(JJLjava.lang.String;Ljava.lang.reflect.Executable;J[J[JI)J";
	method[7].fnPtr = (void*) &JPTypeFactory_defineMethod;


	frame.GetMethodID(cls, "<init>", "()V");
	frame.RegisterNatives(cls, method, 8);
	JP_TRACE_OUT;
}



