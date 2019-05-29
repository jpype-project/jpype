/*****************************************************************************
   Copyright 2004-2008 Steve Ménard

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

namespace
{ // impl detail
	jmethodID s_Object_ToStringID;
	jclass s_ClassLoaderClass;
	
	jmethodID s_ClassLoader_GetSystemClassLoaderID;
	jclass s_ThrowableClass;
	jmethodID s_Throwable_GetMessageID;
	jmethodID s_Throwable_PrintStackTraceID;

	jclass s_StringWriterClass;
	jclass s_PrintWriterClass;
	jmethodID s_StringWriterID;
	jmethodID s_PrintWriterID;
	jmethodID s_FlushID;

	jmethodID s_Number_IntValueID;
	jmethodID s_Number_LongValueID;
	jmethodID s_Number_DoubleValueID;
	jmethodID s_BooleanValueID;
	jmethodID s_CharValueID;
	
	jmethodID s_String_ToCharArrayID;
}

namespace JPJni
{
	jclass s_ProxyClass = 0;
	jmethodID s_NewProxyInstanceID;

} // end of namespace JNIEnv

void JPJni::init()
{
	JP_TRACE_IN("JPJni::init");
	JPJavaFrame frame(32);

	s_StringClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/String"));
	s_String_ToCharArrayID = frame.GetMethodID(s_StringClass, "toCharArray", "()[C");

	s_ClassClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/Class"));
	s_Class_GetNameID = frame.GetMethodID(s_ClassClass, "getName", "()Ljava/lang/String;");

	s_Class_GetComponentTypeID = frame.GetMethodID(s_ClassClass, "getComponentType", "()Ljava/lang/Class;");
	s_Class_GetDeclaredFieldsID = frame.GetMethodID(s_ClassClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
	s_Class_GetDeclaredMethodsID = frame.GetMethodID(s_ClassClass, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
	s_Class_GetMethodsID = frame.GetMethodID(s_ClassClass, "getMethods", "()[Ljava/lang/reflect/Method;");
	s_Class_GetFieldsID = frame.GetMethodID(s_ClassClass, "getFields", "()[Ljava/lang/reflect/Field;");
	s_Class_GetDeclaredConstructorsID = frame.GetMethodID(s_ClassClass, "getDeclaredConstructors", "()[Ljava/lang/reflect/Constructor;");
	s_Class_GetConstructorsID = frame.GetMethodID(s_ClassClass, "getConstructors", "()[Ljava/lang/reflect/Constructor;");
	s_Class_IsArrayID = frame.GetMethodID(s_ClassClass, "isArray", "()Z");
	s_Class_IsInterfaceID = frame.GetMethodID(s_ClassClass, "isInterface", "()Z");
	s_Class_GetModifiersID = frame.GetMethodID(s_ClassClass, "getModifiers", "()I");
	s_Class_GetInterfacesID = frame.GetMethodID(s_ClassClass, "getInterfaces", "()[Ljava/lang/Class;");
	s_Class_GetCanonicalNameID = frame.GetMethodID(s_ClassClass, "getCanonicalName", "()Ljava/lang/String;");

	s_ModifierClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/reflect/Modifier"));
	s_Modifier_IsStaticID = frame.GetStaticMethodID(s_ModifierClass, "isStatic", "(I)Z");
	s_Modifier_IsPublicID = frame.GetStaticMethodID(s_ModifierClass, "isPublic", "(I)Z");
	s_Modifier_IsAbstractID = frame.GetStaticMethodID(s_ModifierClass, "isAbstract", "(I)Z");
	s_Modifier_IsFinalID = frame.GetStaticMethodID(s_ModifierClass, "isFinal", "(I)Z");

	s_ClassLoaderClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/ClassLoader"));
	s_ClassLoader_GetSystemClassLoaderID = frame.GetStaticMethodID(s_ClassLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");


	s_ProxyClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/reflect/Proxy"));
	s_NewProxyInstanceID = frame.GetStaticMethodID(s_ProxyClass, "newProxyInstance", "(Ljava/lang/ClassLoader;[Ljava/lang/Class;Ljava/lang/reflect/InvocationHandler;)Ljava/lang/Object;");


	s_ThrowableClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/Throwable"));
	s_Throwable_GetMessageID = frame.GetMethodID(s_ThrowableClass, "getMessage", "()Ljava/lang/String;");
	s_Throwable_PrintStackTraceID = frame.GetMethodID(s_ThrowableClass, "printStackTrace", "(Ljava/io/PrintWriter;)V");

	s_StringWriterClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/io/StringWriter"));
	s_PrintWriterClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/io/PrintWriter"));
	s_StringWriterID = frame.GetMethodID(s_StringWriterClass, "<init>", "()V");
	s_PrintWriterID = frame.GetMethodID(s_PrintWriterClass, "<init>", "(Ljava/io/Writer;)V");
	s_FlushID = frame.GetMethodID(s_PrintWriterClass, "flush", "()V");

	s_NumberClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/Number"));
	s_Number_IntValueID = frame.GetMethodID(s_NumberClass, "intValue", "()I");
	s_Number_LongValueID = frame.GetMethodID(s_NumberClass, "longValue", "()J");
	s_Number_DoubleValueID = frame.GetMethodID(s_NumberClass, "doubleValue", "()D");

	s_BooleanClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/Boolean"));
	s_BooleanValueID = frame.GetMethodID(s_BooleanClass, "booleanValue", "()Z");

	s_CharClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/Character"));
	s_CharValueID = frame.GetMethodID(s_CharClass, "charValue", "()C");

	s_ByteClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/Byte"));
	s_ShortClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/Short"));
	s_IntClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/Integer"));
	s_FloatClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/Float"));

	jfieldID fid = frame.GetStaticFieldID(s_ByteClass, "MIN_VALUE", "B");
	s_CharClass = (jclass) frame.NewGlobalRef(frame.FindClass("java/lang/Character"));
	s_Byte_Min = frame.GetStaticByteField(s_ByteClass, fid);
	fid = frame.GetStaticFieldID(s_ByteClass, "MAX_VALUE", "B");
	s_Byte_Max = frame.GetStaticByteField(s_ByteClass, fid);

	fid = frame.GetStaticFieldID(s_ShortClass, "MIN_VALUE", "S");
	s_Short_Min = frame.GetStaticShortField(s_ShortClass, fid);
	fid = frame.GetStaticFieldID(s_ShortClass, "MAX_VALUE", "S");
	s_Short_Max = frame.GetStaticShortField(s_ShortClass, fid);

	fid = frame.GetStaticFieldID(s_IntClass, "MIN_VALUE", "I");
	s_Int_Min = frame.GetStaticIntField(s_IntClass, fid);
	fid = frame.GetStaticFieldID(s_IntClass, "MAX_VALUE", "I");
	s_Int_Max = frame.GetStaticIntField(s_IntClass, fid);

	fid = frame.GetStaticFieldID(s_FloatClass, "MIN_VALUE", "F");
	s_Float_Min = frame.GetStaticFloatField(s_FloatClass, fid);
	fid = frame.GetStaticFieldID(s_FloatClass, "MAX_VALUE", "F");
	s_Float_Max = frame.GetStaticFloatField(s_FloatClass, fid);
	JP_TRACE_OUT;
}


string JPJni::getStackTrace(jthrowable th)
{
	JPJavaFrame frame;
	jobject strWriter = frame.NewObject(s_StringWriterClass, s_StringWriterID);

	jvalue v;
	v.l = strWriter;
	jobject printWriter = frame.NewObjectA(s_PrintWriterClass, s_PrintWriterID, &v);

	v.l = printWriter;
	frame.CallVoidMethodA(th, s_Throwable_PrintStackTraceID, &v);
	frame.CallVoidMethod(printWriter, s_FlushID);
	return toString(strWriter);
}

string JPJni::getMessage(jthrowable th)
{
	JPJavaFrame frame;
	jstring jstr = (jstring) frame.CallObjectMethod(th, s_Throwable_GetMessageID);

	return toStringUTF8(jstr);
}
