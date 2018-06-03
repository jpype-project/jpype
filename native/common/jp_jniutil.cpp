/*****************************************************************************
   Copyright 2004-2008 Steve Menard

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

namespace { // impl detail
	jclass objectClass;
	jmethodID getClassID;
	jmethodID toStringID;
	jmethodID hashCodeID;

	jmethodID getNameID;
	jmethodID getDeclaredFieldsID;
	jmethodID getDeclaredMethodsID;
	jmethodID getInterfacesID;
	jmethodID getFieldsID;
	jmethodID getMethodsID;
	jmethodID getDeclaredConstructorsID;
	jmethodID getConstructorsID;
	jmethodID isInterfaceID;
	jmethodID getClassModifiersID;

	jclass modifierClass;
	jmethodID isStaticID;
	jmethodID isPublicID;
	jmethodID isAbstractID;
	jmethodID isFinalID;

	jclass classLoaderClass;
	jmethodID getSystemClassLoaderID;

	jclass memberClass;
	jmethodID getModifiersID;
	jmethodID getMemberNameID;

	jclass fieldClass;
	jmethodID getTypeID;

	jclass methodClass;
	jclass constructorClass;
	jmethodID getReturnTypeID;
	jmethodID isSyntheticMethodID;
	jmethodID isVarArgsMethodID;
	jmethodID getParameterTypesID;
	jmethodID getConstructorParameterTypesID;

	jclass throwableClass;
	jmethodID getMessageID;
	jmethodID printStackTraceID;
	jclass stringWriterClass;
	jclass printWriterClass;
	jmethodID stringWriterID;
	jmethodID printWriterID;
	jmethodID flushID;

	jclass numberClass;
	jclass booleanClass;
	jclass charClass;
	jclass byteClass;
	jclass shortClass;
	jclass intClass;
	jclass floatClass;
	jmethodID intValueID;
	jmethodID longValueID;
	jmethodID doubleValueID;
	jmethodID booleanValueID;
	jmethodID charValueID;
}

namespace JPJni {
	jclass s_ClassClass;
	jclass s_StringClass;
	jclass s_NoSuchMethodErrorClass;
	jclass s_RuntimeExceptionClass;
	jclass s_ProxyClass = 0;
	jmethodID s_NewProxyInstanceID;

	jlong s_minByte;
	jlong s_maxByte;
	jlong s_minShort;
	jlong s_maxShort;
	jlong s_minInt;
	jlong s_maxInt;
	jfloat s_minFloat;
	jfloat s_maxFloat;

void init()
{
	TRACE_IN("JPJni::init");
	JPJavaFrame frame(32);
	objectClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/Object"));
	s_StringClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/String"));
	getClassID = frame.GetMethodID(objectClass, "getClass", "()Ljava/lang/Class;");
	toStringID = frame.GetMethodID(objectClass, "toString", "()Ljava/lang/String;");
	hashCodeID = frame.GetMethodID(objectClass, "hashCode", "()I");

	s_ClassClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/Class"));
	getNameID = frame.GetMethodID(s_ClassClass, "getName", "()Ljava/lang/String;");
	getDeclaredFieldsID = frame.GetMethodID(s_ClassClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
	getDeclaredMethodsID = frame.GetMethodID(s_ClassClass, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
	getMethodsID = frame.GetMethodID(s_ClassClass, "getMethods", "()[Ljava/lang/reflect/Method;");
	getFieldsID = frame.GetMethodID(s_ClassClass, "getFields", "()[Ljava/lang/reflect/Field;");
	getDeclaredConstructorsID = frame.GetMethodID(s_ClassClass, "getDeclaredConstructors", "()[Ljava/lang/reflect/Constructor;");
	getConstructorsID = frame.GetMethodID(s_ClassClass, "getConstructors", "()[Ljava/lang/reflect/Constructor;");
	isInterfaceID = frame.GetMethodID(s_ClassClass, "isInterface", "()Z");
	getClassModifiersID = frame.GetMethodID(s_ClassClass, "getModifiers", "()I");
	getInterfacesID = frame.GetMethodID(s_ClassClass, "getInterfaces", "()[Ljava/lang/Class;");

	modifierClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/reflect/Modifier"));
	isStaticID = frame.GetStaticMethodID(modifierClass, "isStatic", "(I)Z");
	isPublicID = frame.GetStaticMethodID(modifierClass, "isPublic", "(I)Z");
	isAbstractID = frame.GetStaticMethodID(modifierClass, "isAbstract", "(I)Z");
	isFinalID = frame.GetStaticMethodID(modifierClass, "isFinal", "(I)Z");

	classLoaderClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/ClassLoader"));
	getSystemClassLoaderID = frame.GetStaticMethodID(classLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");

	s_NoSuchMethodErrorClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/NoSuchMethodError") );
	s_RuntimeExceptionClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/RuntimeException") );

	s_ProxyClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/reflect/Proxy") );
	s_NewProxyInstanceID = frame.GetStaticMethodID(s_ProxyClass, "newProxyInstance", "(Ljava/lang/ClassLoader;[Ljava/lang/Class;Ljava/lang/reflect/InvocationHandler;)Ljava/lang/Object;");

	memberClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/reflect/Member"));
	getModifiersID = frame.GetMethodID(memberClass, "getModifiers", "()I");
	getMemberNameID = frame.GetMethodID(memberClass, "getName", "()Ljava/lang/String;");

	fieldClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/reflect/Field"));
	getTypeID = frame.GetMethodID(fieldClass, "getType", "()Ljava/lang/Class;");

	methodClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/reflect/Method"));
	constructorClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/reflect/Constructor"));
	getReturnTypeID = frame.GetMethodID(methodClass, "getReturnType", "()Ljava/lang/Class;");
	getParameterTypesID = frame.GetMethodID(methodClass, "getParameterTypes", "()[Ljava/lang/Class;");
	isSyntheticMethodID = frame.GetMethodID(methodClass, "isSynthetic", "()Z");
	isVarArgsMethodID = frame.GetMethodID(methodClass, "isVarArgs", "()Z");
	getConstructorParameterTypesID = frame.GetMethodID(constructorClass, "getParameterTypes", "()[Ljava/lang/Class;");

	throwableClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/Throwable"));
	getMessageID = frame.GetMethodID(throwableClass, "getMessage", "()Ljava/lang/String;");
	printStackTraceID = frame.GetMethodID(throwableClass, "printStackTrace", "(Ljava/io/PrintWriter;)V");
	stringWriterClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/io/StringWriter"));
	printWriterClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/io/PrintWriter"));
	stringWriterID = frame.GetMethodID(stringWriterClass, "<init>", "()V");
	printWriterID = frame.GetMethodID(printWriterClass, "<init>", "(Ljava/io/Writer;)V");
	flushID = frame.GetMethodID(printWriterClass, "flush", "()V");

	numberClass = (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/Number"));
	booleanClass= (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/Boolean"));
	charClass= (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/Character"));
	intValueID = frame.GetMethodID(numberClass, "intValue", "()I");
	longValueID = frame.GetMethodID(numberClass, "longValue", "()J");
	doubleValueID = frame.GetMethodID(numberClass, "doubleValue", "()D");
	booleanValueID = frame.GetMethodID(booleanClass, "booleanValue", "()Z");
	charValueID = frame.GetMethodID(charClass, "charValue", "()C");

	byteClass= (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/Byte"));
	shortClass= (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/Short"));
	intClass= (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/Integer"));
	floatClass= (jclass)frame.NewGlobalRef(frame.FindClass("java/lang/Float"));

	jfieldID fid = frame.GetStaticFieldID(byteClass, "MIN_VALUE", "B");
	s_minByte = frame.GetStaticByteField(byteClass, fid);
	fid = frame.GetStaticFieldID(byteClass, "MAX_VALUE", "B");
	s_maxByte = frame.GetStaticByteField(byteClass, fid);

	fid = frame.GetStaticFieldID(shortClass, "MIN_VALUE", "S");
	s_minShort = frame.GetStaticShortField(shortClass, fid);
	fid = frame.GetStaticFieldID(shortClass, "MAX_VALUE", "S");
	s_maxShort = frame.GetStaticShortField(shortClass, fid);

	fid = frame.GetStaticFieldID(intClass, "MIN_VALUE", "I");
	s_minInt = frame.GetStaticIntField(intClass, fid);
	fid = frame.GetStaticFieldID(intClass, "MAX_VALUE", "I");
	s_maxInt = frame.GetStaticIntField(intClass, fid);

	fid = frame.GetStaticFieldID(floatClass, "MIN_VALUE", "F");
	s_minFloat = frame.GetStaticFloatField(floatClass, fid);
	fid = frame.GetStaticFieldID(floatClass, "MAX_VALUE", "F");
	s_maxFloat = frame.GetStaticFloatField(floatClass, fid);
	TRACE_OUT;
}

string asciiFromJava(jstring str)
{
	JPJavaFrame frame;
	const char* cstr = NULL;
	jboolean isCopy;
	cstr = frame.GetStringUTFChars(str, &isCopy);
	int length = frame.GetStringLength(str);

	string res;
	for (int i = 0; i < length; i++)
	{
		res += (char)cstr[i];
	}

	frame.ReleaseStringUTFChars(str, cstr);

	return res;
}

JCharString unicodeFromJava(jstring str)
{
	JPJavaFrame frame;
	const jchar* cstr = NULL;
	jboolean isCopy;
	cstr = frame.GetStringChars(str, &isCopy);

	JCharString res = cstr;

	frame.ReleaseStringChars(str, cstr);

	return res;
}

jstring javaStringFromJCharString(JCharString& wstr)
{
	JPJavaFrame frame;
	return frame.NewString(wstr.c_str(), (jint)wstr.length());
}

JPTypeName getClassName(jobject o)
{
	if (o == NULL)
	{
		return JPTypeName::fromSimple("java.lang.Object");
	}

	JPJavaFrame frame;
	jclass c = getClass(o);
	return getName(c);
}

jclass getClass(jobject o)
{
	JPJavaFrame frame;
	return (jclass)frame.CallObjectMethod(o, getClassID);
}

jstring toString(jobject o)
{
	JPJavaFrame frame;
	jstring str = (jstring)frame.CallObjectMethod(o, toStringID);
	return str;
}

static string convertToSimpleName(jclass c)
{
	JPJavaFrame frame;
	jstring jname = (jstring)frame.CallObjectMethod(c, getNameID);
	string name = asciiFromJava(jname);

	// Class.getName returns something weird for arrays ...
	if (name[0] == '[')
	{
		// perform a little cleanup of the name ...
		unsigned int arrayCount = 0;
		for (unsigned int i = 0; i < name.length(); i++)
		{
			if (name[i] == '[')
			{
				arrayCount++;
			}
		}

		name = name.substr(arrayCount, name.length()-(arrayCount));

		// Now, let's convert the "native" part
		switch(name[0])
		{
			case 'B' :
				name = "byte";
				break;
			case 'S' :
				name = "short";
				break;
			case 'I' :
				name = "int";
				break;
			case 'J' :
				name = "long";
				break;
			case 'F' :
				name = "float";
				break;
			case 'D' :
				name = "double";
				break;
			case 'C' :
				name = "char";
				break;
			case 'Z' :
				name = "boolean";
				break;
			case 'L' :
				name = name.substr(1, name.length()-2);
				for (unsigned int i = 0; i < name.length(); i++)
				{
					if (name[i] == '/')
					{
						name[i] = '.';
					}
				}
				break;
		}

		for (unsigned int j = 0; j < arrayCount; j++)
		{
			name = name + "[]";
		}
	}

	return name;
}

bool isInterface(jclass clazz)
{
	JPJavaFrame frame;
	jboolean b = frame.CallBooleanMethod(clazz, isInterfaceID);
	return (b ? true : false);
}

bool isAbstract(jclass clazz)
{
	JPJavaFrame frame;
	jvalue modif;
	modif.i = frame.CallIntMethod(clazz, getClassModifiersID);
	jboolean res = frame.CallStaticBooleanMethodA(modifierClass, isAbstractID, &modif);

	return (res ? true : false);
}

long getClassModifiers(jclass clazz)
{
	JPJavaFrame frame;
	return frame.CallIntMethod(clazz, getClassModifiersID);
}


bool isFinal(jclass clazz)
{
	JPJavaFrame frame;
	jvalue modif;
	modif.i = frame.CallIntMethod(clazz, getClassModifiersID);
	jboolean res = frame.CallStaticBooleanMethodA(modifierClass, isFinalID, &modif);

	return (res ? true : false);
}

JPTypeName getName(jclass clazz)
{
	string simpleName = convertToSimpleName(clazz);
	return JPTypeName::fromSimple(simpleName.c_str());
}

// Returns multiple local references,  must have a suitable local frame
vector<jclass> getInterfaces(JPJavaFrame& frame, jclass clazz)
{
	jobjectArray interfaces = (jobjectArray)frame.CallObjectMethod(clazz, getInterfacesID);

	int len = frame.GetArrayLength(interfaces);
	vector<jclass> res;
	for (int i = 0; i < len; i++)
	{
		jclass c = (jclass)frame.GetObjectArrayElement(interfaces, i);
		res.push_back(c);
	}

	return res;
}

// Returns multiple local references,  must have a suitable local frame
vector<jobject> getDeclaredFields(JPJavaFrame& frame, jclass clazz)
{
	jobjectArray fields = (jobjectArray)frame.CallObjectMethod(clazz, getDeclaredFieldsID);

	int len = frame.GetArrayLength(fields);
	vector<jobject> res;
	for (int i = 0; i < len; i++)
	{
		jobject c = frame.GetObjectArrayElement(fields, i);
		res.push_back(c);
	}

	return res;
}

// Returns multiple local references,  must have a suitable local frame
vector<jobject> getFields(JPJavaFrame& frame, jclass clazz)
{
	jobjectArray fields = (jobjectArray)frame.CallObjectMethod(clazz, getFieldsID);

	int len = frame.GetArrayLength(fields);
	vector<jobject> res;
	for (int i = 0; i < len; i++)
	{
		jobject c = frame.GetObjectArrayElement(fields, i);
		res.push_back(c);
	}

	return res;
}

// Returns multiple local references,  must have a suitable local frame
vector<jobject> getDeclaredMethods(JPJavaFrame& frame, jclass clazz)
{
	jobjectArray methods = (jobjectArray)frame.CallObjectMethod(clazz, getDeclaredMethodsID);

	int len = frame.GetArrayLength(methods);
	vector<jobject> res;
	for (int i = 0; i < len; i++)
	{
		jobject c = frame.GetObjectArrayElement(methods, i);
		res.push_back(c);
	}

	return res;
}


// Returns multiple local references,  must have a suitable local frame
vector<jobject> getDeclaredConstructors(JPJavaFrame& frame, jclass clazz)
{
	jobjectArray methods = (jobjectArray)frame.CallObjectMethod(clazz, getDeclaredConstructorsID);

	int len = frame.GetArrayLength(methods);
	vector<jobject> res;
	for (int i = 0; i < len; i++)
	{
		jobject c = frame.GetObjectArrayElement(methods, i);
		res.push_back(c);
	}

	return res;
}

// Returns multiple local references,  must have a suitable local frame
vector<jobject> getConstructors(JPJavaFrame& frame, jclass clazz)
{
	jobjectArray methods = (jobjectArray)frame.CallObjectMethod(clazz, getConstructorsID);

	int len = frame.GetArrayLength(methods);
	vector<jobject> res;
	for (int i = 0; i < len; i++)
	{
		jobject c = frame.GetObjectArrayElement(methods, i);
		res.push_back(c);
	}

	return res;
}

// Returns multiple local references,  must have a suitable local frame
vector<jobject> getMethods(JPJavaFrame& frame, jclass clazz)
{
	jobjectArray methods = (jobjectArray)frame.CallObjectMethod(clazz, getMethodsID);

	int len = frame.GetArrayLength(methods);
	vector<jobject> res;
	for (int i = 0; i < len; i++)
	{
		jobject c = frame.GetObjectArrayElement(methods, i);
		res.push_back(c);
	}

	return res;
}

// Returns local reference
jobject getSystemClassLoader()
{
	JPJavaFrame frame;
	return frame.keep(frame.CallStaticObjectMethod(classLoaderClass, getSystemClassLoaderID)) ;
}

string getMemberName(jobject o)
{
	JPJavaFrame frame;
	jstring name = (jstring)frame.CallObjectMethod(o, getMemberNameID);

	string simpleName = asciiFromJava(name);
	return simpleName;
}

bool isMemberPublic(jobject o)
{
	JPJavaFrame frame;
	jvalue modif;
	modif.i = frame.CallIntMethod(o, getModifiersID);
	jboolean res = frame.CallStaticBooleanMethodA(modifierClass, isPublicID, &modif);

	return (res ? true : false);
}

bool isMemberStatic(jobject o)
{
	JPJavaFrame frame;
	jvalue modif;
	modif.i = frame.CallIntMethod(o, getModifiersID);
	jboolean res = frame.CallStaticBooleanMethodA(modifierClass, isStaticID, &modif);

	return (res ? true : false);
}

bool isMemberFinal(jobject o)
{
	JPJavaFrame frame;
	jvalue modif;
	modif.i = frame.CallIntMethod(o, getModifiersID);
	jboolean res = frame.CallStaticBooleanMethodA(modifierClass, isFinalID, &modif);

	return (res ? true : false);
}

bool isMemberAbstract(jobject o)
{
	JPJavaFrame frame;
	jvalue modif;
	modif.i = frame.CallIntMethod(o, getModifiersID);
	jboolean res = frame.CallStaticBooleanMethodA(modifierClass, isAbstractID, &modif);

	return (res ? true : false);
}

jint hashCode(jobject obj)
{
	JPJavaFrame frame;
	return frame.CallIntMethod(obj, hashCodeID);
}

JPTypeName getType(jobject fld)
{
	JPJavaFrame frame;
	TRACE_IN("getType");

	jclass c = (jclass)frame.CallObjectMethod(fld, getTypeID);

	return getName(c);
	TRACE_OUT;
}

JPTypeName getReturnType(jobject o)
{
	JPJavaFrame frame;
	jclass c = (jclass)frame.CallObjectMethod(o, getReturnTypeID);
	return getName(c);
}

bool isMethodSynthetic(jobject o)
{
	JPJavaFrame frame;
	jboolean res = frame.CallBooleanMethod(o, isSyntheticMethodID);
  	return (res ? true : false);
}

bool isVarArgsMethod(jobject o)
{
	JPJavaFrame frame;
  	jboolean res = frame.CallBooleanMethod(o, isVarArgsMethodID);
  	return (res ? true : false);
}

vector<JPTypeName> getParameterTypes(jobject o, bool isConstructor)
{
	JPJavaFrame frame;
	vector<JPTypeName> args;

	jobjectArray types ;
	if (isConstructor)
	{
		types = (jobjectArray)frame.CallObjectMethod(o, getConstructorParameterTypesID);
	}
	else
	{
		types = (jobjectArray)frame.CallObjectMethod(o, getParameterTypesID);
	}

	int len = frame.GetArrayLength(types);
	{
		JPJavaFrame frame2(4+len);
		for (int i = 0; i < len; i++)
		{
			jclass c = (jclass)frame.GetObjectArrayElement(types, i);

			JPTypeName name = getName(c);
			args.push_back(name);
		}
	}
	return args;
}

bool isConstructor(jobject obj)
{
	JPJavaFrame frame;
	return frame.IsInstanceOf(obj, constructorClass)!=0;
}

string getStackTrace(jthrowable th)
{
	JPJavaFrame frame;
	jobject strWriter = frame.NewObject(stringWriterClass, stringWriterID);

	jvalue v;
	v.l = strWriter;
	jobject printWriter = frame.NewObjectA(printWriterClass, printWriterID, &v);

	v.l = printWriter;
	frame.CallVoidMethodA(th, printStackTraceID, &v);
	frame.CallVoidMethod(printWriter, flushID);
	return asciiFromJava(toString(strWriter));
}

string getMessage(jthrowable th)
{
	JPJavaFrame frame;
	jstring jstr = (jstring)frame.CallObjectMethod(th, getMessageID);

	return asciiFromJava(jstr);
}

bool isThrowable(jclass c)
{
  	JPJavaFrame frame;
	jboolean res = frame.IsAssignableFrom(c, throwableClass);
	if (res)
	{
		return true;
	}
	return false;
}

long intValue(jobject obj)
{
	JPJavaFrame frame;
	return frame.CallIntMethod(obj, intValueID);
}

jlong longValue(jobject obj)
{
	JPJavaFrame frame;
	return frame.CallLongMethod(obj, longValueID);
}

double doubleValue(jobject obj)
{
	JPJavaFrame frame;
	return frame.CallDoubleMethod(obj, doubleValueID);
}

bool booleanValue(jobject obj)
{
	JPJavaFrame frame;
	return frame.CallBooleanMethod(obj, booleanValueID) ? true : false;
}

jchar charValue(jobject obj)
{
	JPJavaFrame frame;
	return frame.CallCharMethod(obj, charValueID);
}

jclass getPrimitiveClass(const char* name)
{
	JPJavaFrame frame;
	jclass clz = (jclass)frame.FindClass(name);
	jfieldID fid = frame.GetStaticFieldID(clz, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)frame.GetStaticObjectField(clz, fid);
	return res;
}


jclass getByteClass()
{
	return getPrimitiveClass("java/lang/Byte");
}

jclass getShortClass()
{
	return getPrimitiveClass("java/lang/Short");
}

jclass getIntegerClass()
{
	return getPrimitiveClass("java/lang/Integer");
}

jclass getLongClass()
{
	return getPrimitiveClass("java/lang/Long");
}

jclass getFloatClass()
{
	return getPrimitiveClass("java/lang/Float");
}

jclass getDoubleClass()
{
	return getPrimitiveClass("java/lang/Double");
}

jclass getCharacterClass()
{
	return getPrimitiveClass("java/lang/Character");
}

jclass getBooleanClass()
{
	return getPrimitiveClass("java/lang/Boolean");
}

jclass getVoidClass()
{
	return getPrimitiveClass("java/lang/Void");
}

} // end of namespace JNIEnv
