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

	jclass    JPypeReferenceClass;
	jmethodID JPypeReferenceConstructorMethod;
	jclass    JPypeReferenceQueueClass;
	jmethodID JPypeReferenceQueueConstructorMethod;
	jmethodID JPypeReferenceQueueRegisterMethod;
	jmethodID JPypeReferenceQueueStartMethod;
	jmethodID JPypeReferenceQueueRunMethod;
	jmethodID JPypeReferenceQueueStopMethod;
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
	objectClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Object"));
	s_StringClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/String"));
	getClassID = JPEnv::getJava()->GetMethodID(objectClass, "getClass", "()Ljava/lang/Class;");
	toStringID = JPEnv::getJava()->GetMethodID(objectClass, "toString", "()Ljava/lang/String;");
	hashCodeID = JPEnv::getJava()->GetMethodID(objectClass, "hashCode", "()I");

	s_ClassClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Class"));
	getNameID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getName", "()Ljava/lang/String;");
	getDeclaredFieldsID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
	getDeclaredMethodsID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
	getMethodsID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getMethods", "()[Ljava/lang/reflect/Method;");
	getDeclaredConstructorsID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getDeclaredConstructors", "()[Ljava/lang/reflect/Constructor;");
	getConstructorsID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getConstructors", "()[Ljava/lang/reflect/Constructor;");
	isInterfaceID = JPEnv::getJava()->GetMethodID(s_ClassClass, "isInterface", "()Z");
	getClassModifiersID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getModifiers", "()I");
	getInterfacesID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getInterfaces", "()[Ljava/lang/Class;");

	modifierClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/reflect/Modifier"));
	isStaticID = JPEnv::getJava()->GetStaticMethodID(modifierClass, "isStatic", "(I)Z");
	isPublicID = JPEnv::getJava()->GetStaticMethodID(modifierClass, "isPublic", "(I)Z");
	isAbstractID = JPEnv::getJava()->GetStaticMethodID(modifierClass, "isAbstract", "(I)Z");
	isFinalID = JPEnv::getJava()->GetStaticMethodID(modifierClass, "isFinal", "(I)Z");

	classLoaderClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/ClassLoader"));
	getSystemClassLoaderID = JPEnv::getJava()->GetStaticMethodID(classLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");  

	s_NoSuchMethodErrorClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/NoSuchMethodError") );
	s_RuntimeExceptionClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/RuntimeException") );

	s_ProxyClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/reflect/Proxy") );
	s_NewProxyInstanceID = JPEnv::getJava()->GetStaticMethodID(s_ProxyClass, "newProxyInstance", "(Ljava/lang/ClassLoader;[Ljava/lang/Class;Ljava/lang/reflect/InvocationHandler;)Ljava/lang/Object;");

	memberClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/reflect/Member"));
	getModifiersID = JPEnv::getJava()->GetMethodID(memberClass, "getModifiers", "()I");
	getMemberNameID = JPEnv::getJava()->GetMethodID(memberClass, "getName", "()Ljava/lang/String;");

	fieldClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/reflect/Field"));
	getTypeID = JPEnv::getJava()->GetMethodID(fieldClass, "getType", "()Ljava/lang/Class;");

	methodClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/reflect/Method"));
	constructorClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/reflect/Constructor"));
	getReturnTypeID = JPEnv::getJava()->GetMethodID(methodClass, "getReturnType", "()Ljava/lang/Class;");
	getParameterTypesID = JPEnv::getJava()->GetMethodID(methodClass, "getParameterTypes", "()[Ljava/lang/Class;");
	getConstructorParameterTypesID = JPEnv::getJava()->GetMethodID(constructorClass, "getParameterTypes", "()[Ljava/lang/Class;");

	throwableClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Throwable"));
	getMessageID = JPEnv::getJava()->GetMethodID(throwableClass, "getMessage", "()Ljava/lang/String;");
	printStackTraceID = JPEnv::getJava()->GetMethodID(throwableClass, "printStackTrace", "(Ljava/io/PrintWriter;)V");
	stringWriterClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/io/StringWriter"));
	printWriterClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/io/PrintWriter"));
	stringWriterID = JPEnv::getJava()->GetMethodID(stringWriterClass, "<init>", "()V");
	printWriterID = JPEnv::getJava()->GetMethodID(printWriterClass, "<init>", "(Ljava/io/Writer;)V");
	flushID = JPEnv::getJava()->GetMethodID(printWriterClass, "flush", "()V");
	
	numberClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Number"));
	booleanClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Boolean"));
	charClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Character"));
	intValueID = JPEnv::getJava()->GetMethodID(numberClass, "intValue", "()I");
	longValueID = JPEnv::getJava()->GetMethodID(numberClass, "longValue", "()J");
	doubleValueID = JPEnv::getJava()->GetMethodID(numberClass, "doubleValue", "()D");
	booleanValueID = JPEnv::getJava()->GetMethodID(booleanClass, "booleanValue", "()Z");
	charValueID = JPEnv::getJava()->GetMethodID(charClass, "charValue", "()C");

	byteClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Byte"));
	shortClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Short"));
	intClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Integer"));
	floatClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Float"));

	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(byteClass, "MIN_VALUE", "B");
	s_minByte = JPEnv::getJava()->GetStaticByteField(byteClass, fid);
	fid = JPEnv::getJava()->GetStaticFieldID(byteClass, "MAX_VALUE", "B");
	s_maxByte = JPEnv::getJava()->GetStaticByteField(byteClass, fid);

	fid = JPEnv::getJava()->GetStaticFieldID(shortClass, "MIN_VALUE", "S");
	s_minShort = JPEnv::getJava()->GetStaticShortField(shortClass, fid);
	fid = JPEnv::getJava()->GetStaticFieldID(shortClass, "MAX_VALUE", "S");
	s_maxShort = JPEnv::getJava()->GetStaticShortField(shortClass, fid);

	fid = JPEnv::getJava()->GetStaticFieldID(intClass, "MIN_VALUE", "I");
	s_minInt = JPEnv::getJava()->GetStaticIntField(intClass, fid);
	fid = JPEnv::getJava()->GetStaticFieldID(intClass, "MAX_VALUE", "I");
	s_maxInt = JPEnv::getJava()->GetStaticIntField(intClass, fid);
	
	fid = JPEnv::getJava()->GetStaticFieldID(floatClass, "MIN_VALUE", "F");
	s_minFloat = JPEnv::getJava()->GetStaticFloatField(floatClass, fid);
	fid = JPEnv::getJava()->GetStaticFieldID(floatClass, "MAX_VALUE", "F");
	s_maxFloat = JPEnv::getJava()->GetStaticFloatField(floatClass, fid);
}

string asciiFromJava(jstring str)
{
	const char* cstr = NULL;
	jboolean isCopy;
	cstr = JPEnv::getJava()->GetStringUTFChars(str, &isCopy);
	int length = JPEnv::getJava()->GetStringLength(str);

	string res;
	for (int i = 0; i < length; i++)
	{
		res += (char)cstr[i];
	}

	JPEnv::getJava()->ReleaseStringUTFChars(str, cstr);

	return res;	
}

JCharString unicodeFromJava(jstring str)
{
	const jchar* cstr = NULL;
	jboolean isCopy;
	cstr = JPEnv::getJava()->GetStringChars(str, &isCopy);

	JCharString res = cstr;

	JPEnv::getJava()->ReleaseStringChars(str, cstr);

	return res;	
}

jstring javaStringFromJCharString(JCharString& wstr)
{
	jstring result = JPEnv::getJava()->NewString(wstr.c_str(), (jint)wstr.length());
	
	return result;
}

JPTypeName getClassName(jobject o)
{
	if (o == NULL)
	{
		return JPTypeName::fromSimple("java.lang.Object");
	}

	JPCleaner cleaner;
	jclass c = getClass(o);
	cleaner.addLocal(c);
	return getName(c);
}

jclass getClass(jobject o)
{
	return (jclass)JPEnv::getJava()->CallObjectMethod(o, getClassID);
}

jstring toString(jobject o)
{
	jstring str = (jstring)JPEnv::getJava()->CallObjectMethod(o, toStringID);
	return str;
}

static string convertToSimpleName(jclass c)  
{
	JPCleaner cleaner;
	jstring jname = (jstring)JPEnv::getJava()->CallObjectMethod(c, getNameID);
	cleaner.addLocal(jname);
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
	jboolean b = JPEnv::getJava()->CallBooleanMethod(clazz, isInterfaceID);
	return (b ? true : false);
}

bool isAbstract(jclass clazz)
{
	jvalue modif;
	modif.i = JPEnv::getJava()->CallIntMethod(clazz, getClassModifiersID);
	jboolean res = JPEnv::getJava()->CallStaticBooleanMethodA(modifierClass, isAbstractID, &modif);
	
	return (res ? true : false);
}

long getClassModifiers(jclass clazz)
{
	long res = JPEnv::getJava()->CallIntMethod(clazz, getClassModifiersID);
	
	return res;
}


bool isFinal(jclass clazz)
{
	jvalue modif;
	modif.i = JPEnv::getJava()->CallIntMethod(clazz, getClassModifiersID);
	jboolean res = JPEnv::getJava()->CallStaticBooleanMethodA(modifierClass, isFinalID, &modif);
	
	return (res ? true : false);
}

JPTypeName getName(jclass clazz)
{
	string simpleName = convertToSimpleName(clazz);
	
	return JPTypeName::fromSimple(simpleName.c_str());
}

vector<jclass> getInterfaces(jclass clazz)
{
	JPCleaner cleaner;
	jobjectArray interfaces = (jobjectArray)JPEnv::getJava()->CallObjectMethod(clazz, getInterfacesID);
	cleaner.addLocal(interfaces);

	int len = JPEnv::getJava()->GetArrayLength(interfaces);
	vector<jclass> res;
	for (int i = 0; i < len; i++)
	{
		jclass c = (jclass)JPEnv::getJava()->GetObjectArrayElement(interfaces, i);
		res.push_back(c);
	}
	
	return res;
}

vector<jobject> getDeclaredFields(jclass clazz)
{
	JPCleaner cleaner;
	jobjectArray fields = (jobjectArray)JPEnv::getJava()->CallObjectMethod(clazz, getDeclaredFieldsID);
	cleaner.addLocal(fields);

	int len = JPEnv::getJava()->GetArrayLength(fields);
	vector<jobject> res;
	for (int i = 0; i < len; i++)
	{
		jobject c = JPEnv::getJava()->GetObjectArrayElement(fields, i);
		res.push_back(c);
	}
	
	return res;
}

vector<jobject> getFields(jclass clazz)
{
	JPCleaner cleaner;
	jobjectArray fields = (jobjectArray)JPEnv::getJava()->CallObjectMethod(clazz, getFieldsID);
	cleaner.addLocal(fields);

	int len = JPEnv::getJava()->GetArrayLength(fields);
	vector<jobject> res;
	for (int i = 0; i < len; i++)
	{
		jobject c = JPEnv::getJava()->GetObjectArrayElement(fields, i);
		res.push_back(c);
	}
	
	return res;
}

vector<jobject> getDeclaredMethods(jclass clazz)
{
	JPCleaner cleaner;
	jobjectArray methods = (jobjectArray)JPEnv::getJava()->CallObjectMethod(clazz, getDeclaredMethodsID);
	cleaner.addLocal(methods);

	int len = JPEnv::getJava()->GetArrayLength(methods);
	vector<jobject> res;
	for (int i = 0; i < len; i++)
	{
		jobject c = JPEnv::getJava()->GetObjectArrayElement(methods, i);
		res.push_back(c);
	}
	
	return res;
}


vector<jobject> getDeclaredConstructors(jclass clazz)
{
	JPCleaner cleaner;
	jobjectArray methods = (jobjectArray)JPEnv::getJava()->CallObjectMethod(clazz, getDeclaredConstructorsID);
	cleaner.addLocal(methods);

	int len = JPEnv::getJava()->GetArrayLength(methods);
	vector<jobject> res;
	for (int i = 0; i < len; i++)
	{
		jobject c = JPEnv::getJava()->GetObjectArrayElement(methods, i);
		res.push_back(c);
	}
	
	return res;
}

vector<jobject> getConstructors(jclass clazz)
{
	JPCleaner cleaner;
	jobjectArray methods = (jobjectArray)JPEnv::getJava()->CallObjectMethod(clazz, getConstructorsID);
	cleaner.addLocal(methods);

	int len = JPEnv::getJava()->GetArrayLength(methods);
	vector<jobject> res;
	for (int i = 0; i < len; i++)
	{
		jobject c = JPEnv::getJava()->GetObjectArrayElement(methods, i);
		res.push_back(c);
	}
	
	return res;
}

vector<jobject> getMethods(jclass clazz)
{
	JPCleaner cleaner;
	jobjectArray methods = (jobjectArray)JPEnv::getJava()->CallObjectMethod(clazz, getMethodsID);
	cleaner.addLocal(methods);

	int len = JPEnv::getJava()->GetArrayLength(methods);
	vector<jobject> res;
	for (int i = 0; i < len; i++)
	{
		jobject c = JPEnv::getJava()->GetObjectArrayElement(methods, i);
		res.push_back(c);
	}
	
	return res;
}

jobject getSystemClassLoader()
{
	return JPEnv::getJava()->CallStaticObjectMethod(classLoaderClass, getSystemClassLoaderID) ; 
}

string getMemberName(jobject o)
{
	JPCleaner cleaner;
	jstring name = (jstring)JPEnv::getJava()->CallObjectMethod(o, getMemberNameID);
	cleaner.addLocal(name);

	string simpleName = asciiFromJava(name);
	return simpleName;
}
	
bool isMemberPublic(jobject o)
{
	jvalue modif;
	modif.i = JPEnv::getJava()->CallIntMethod(o, getModifiersID);
	jboolean res = JPEnv::getJava()->CallStaticBooleanMethodA(modifierClass, isPublicID, &modif);
	
	return (res ? true : false);
}

bool isMemberStatic(jobject o)
{
	jvalue modif;
	modif.i = JPEnv::getJava()->CallIntMethod(o, getModifiersID);
	jboolean res = JPEnv::getJava()->CallStaticBooleanMethodA(modifierClass, isStaticID, &modif);
	
	return (res ? true : false);
}

bool isMemberFinal(jobject o)
{
	jvalue modif;
	modif.i = JPEnv::getJava()->CallIntMethod(o, getModifiersID);
	jboolean res = JPEnv::getJava()->CallStaticBooleanMethodA(modifierClass, isFinalID, &modif);
	
	return (res ? true : false);
}

bool isMemberAbstract(jobject o)
{
	jvalue modif;
	modif.i = JPEnv::getJava()->CallIntMethod(o, getModifiersID);
	jboolean res = JPEnv::getJava()->CallStaticBooleanMethodA(modifierClass, isAbstractID, &modif);
	
	return (res ? true : false);
}

jint hashCode(jobject obj)
{
	jint res = JPEnv::getJava()->CallIntMethod(obj, hashCodeID);
	return res;
}

JPTypeName getType(jobject fld)
{
	TRACE_IN("getType");
	
	JPCleaner cleaner;
	jclass c = (jclass)JPEnv::getJava()->CallObjectMethod(fld, getTypeID);
	cleaner.addLocal(c);
	
	return getName(c);
	TRACE_OUT;
}

JPTypeName getReturnType(jobject o)
{
	JPCleaner cleaner;
	jclass c = (jclass)JPEnv::getJava()->CallObjectMethod(o, getReturnTypeID);
	cleaner.addLocal(c);
	
	return getName(c);
}

vector<JPTypeName> getParameterTypes(jobject o, bool isConstructor)
{
	JPCleaner cleaner;
	vector<JPTypeName> args;
	
	jobjectArray types ;
	if (isConstructor)
	{
		types = (jobjectArray)JPEnv::getJava()->CallObjectMethod(o, getConstructorParameterTypesID);
	}
	else
	{
		types = (jobjectArray)JPEnv::getJava()->CallObjectMethod(o, getParameterTypesID);
	}

	cleaner.addLocal(types);

	int len = JPEnv::getJava()->GetArrayLength(types);
	
	for (int i = 0; i < len; i++)
	{
		jclass c = (jclass)JPEnv::getJava()->GetObjectArrayElement(types, i);
		cleaner.addLocal(c);

		JPTypeName name = getName(c);
		args.push_back(name);
	}
	
	return args;
}

bool isConstructor(jobject obj)
{
	jboolean r = JPEnv::getJava()->IsInstanceOf(obj, constructorClass);
	if (r)
	{
		return true;
	}
	return false;
}

string getStackTrace(jthrowable th)
{
	
	JPCleaner cleaner;
	jobject strWriter = JPEnv::getJava()->NewObject(stringWriterClass, stringWriterID);
	cleaner.addLocal(strWriter);

	jvalue v;
	v.l = strWriter;
	jobject printWriter = JPEnv::getJava()->NewObjectA(printWriterClass, printWriterID, &v);
	cleaner.addLocal(printWriter);

	v.l = printWriter;
	JPEnv::getJava()->CallVoidMethodA(th, printStackTraceID, &v);

	JPEnv::getJava()->CallVoidMethod(printWriter, flushID);

	jstring res = toString(strWriter);
	cleaner.addLocal(res);

	return asciiFromJava(res);
}

string getMessage(jthrowable th)
{
	JPCleaner cleaner;
	jstring jstr = (jstring)JPEnv::getJava()->CallObjectMethod(th, getMessageID);
	cleaner.addLocal(jstr);

	return asciiFromJava(jstr);
}

bool isThrowable(jclass c)
{
	jboolean res = JPEnv::getJava()->IsAssignableFrom(c, throwableClass);
	if (res)
	{
		return true;
	}
	return false;
}

long intValue(jobject obj)
{
	return JPEnv::getJava()->CallIntMethod(obj, intValueID);	
}

jlong longValue(jobject obj)
{
	return JPEnv::getJava()->CallLongMethod(obj, longValueID);	
}

double doubleValue(jobject obj)
{
	return JPEnv::getJava()->CallDoubleMethod(obj, doubleValueID);	
}

bool booleanValue(jobject obj)
{
	return JPEnv::getJava()->CallBooleanMethod(obj, booleanValueID) ? true : false;	
}

jchar charValue(jobject obj)
{
	return JPEnv::getJava()->CallCharMethod(obj, charValueID);	
}

jclass getByteClass()
{
	jclass byteClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Byte"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(byteClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(byteClass, fid);
	JPEnv::getJava()->DeleteGlobalRef(byteClass);
	return res;
}

jclass getShortClass()
{
	jclass shortClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Short"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(shortClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(shortClass, fid);
	JPEnv::getJava()->DeleteGlobalRef(shortClass);
	return res;
}

jclass getIntegerClass()
{
	jclass intClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Integer"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(intClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(intClass, fid);
	JPEnv::getJava()->DeleteGlobalRef(intClass);
	return res;
}

jclass getLongClass()
{
	jclass longClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Long"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(longClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(longClass, fid);
	JPEnv::getJava()->DeleteGlobalRef(longClass);
	return res;
}

jclass getFloatClass()
{
	jclass floatClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Float"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(floatClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(floatClass, fid);
	JPEnv::getJava()->DeleteGlobalRef(floatClass);
	return res;
}

jclass getDoubleClass()
{
	jclass doubleClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Double"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(doubleClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(doubleClass, fid);
	JPEnv::getJava()->DeleteGlobalRef(doubleClass);
	return res;
}

jclass getCharacterClass()
{
	jclass charClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Character"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(charClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(charClass, fid);
	JPEnv::getJava()->DeleteGlobalRef(charClass);
	return res;
}

jclass getBooleanClass()
{
	jclass booleanClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Boolean"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(booleanClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(booleanClass, fid);
	JPEnv::getJava()->DeleteGlobalRef(booleanClass);
	return res;
}

jclass getVoidClass()
{
	jclass voidClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/Void"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(voidClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(voidClass, fid);
	JPEnv::getJava()->DeleteGlobalRef(voidClass);
	return res;
}

void startJPypeReferenceQueue(bool useJavaThread)
{
	JPCleaner cleaner;

	JPypeReferenceQueueClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("jpype/ref/JPypeReferenceQueue"));
	JPypeReferenceQueueConstructorMethod = JPEnv::getJava()->GetMethodID(JPypeReferenceQueueClass, "<init>", "()V");
	JPypeReferenceQueueRegisterMethod = JPEnv::getJava()->GetMethodID(JPypeReferenceQueueClass, "registerRef", "(Ljpype/ref/JPypeReference;J)V");
	JPypeReferenceQueueStartMethod = JPEnv::getJava()->GetMethodID(JPypeReferenceQueueClass, "startManaging", "()V");
	JPypeReferenceQueueRunMethod = JPEnv::getJava()->GetMethodID(JPypeReferenceQueueClass, "run", "()V");
	JPypeReferenceQueueStopMethod = JPEnv::getJava()->GetMethodID(JPypeReferenceQueueClass, "stop", "()V");

	JPypeReferenceClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("jpype/ref/JPypeReference"));
	JPypeReferenceConstructorMethod = JPEnv::getJava()->GetMethodID(JPypeReferenceClass, "<init>", "(Ljava/lang/Object;Ljava/lang/ref/ReferenceQueue;)V");

	jobject obj = JPEnv::getJava()->NewObject(JPypeReferenceQueueClass, JPypeReferenceQueueConstructorMethod);
	cleaner.addLocal(obj);
	JPEnv::getJava()->setReferenceQueue(obj);

	if (useJavaThread)
	{
		JPEnv::getJava()->CallVoidMethod(obj, JPypeReferenceQueueStartMethod); 
	}
	else
	{
		JPEnv::getJava()->CallVoidMethod(obj, JPypeReferenceQueueRunMethod); 
	}

}

void stopJPypeReferenceQueue()
{
	JPEnv::getJava()->CallVoidMethod(JPEnv::getJava()->getReferenceQueue(), JPypeReferenceQueueStopMethod);
}

void registerRef(jobject refQueue, jobject obj, jlong hostRef)
{
	TRACE_IN("registerRef");
	// create the ref ...
	jvalue args[2];
	args[0].l = obj;
	args[1].l = refQueue;

	JPCleaner cleaner;

	jobject refObj = JPEnv::getJava()->NewObjectA(JPypeReferenceClass, JPypeReferenceConstructorMethod, args);
	cleaner.addLocal(refObj);

	args[0].l = refObj;
	args[1].j = hostRef;

	JPEnv::getJava()->CallVoidMethodA(refQueue, JPypeReferenceQueueRegisterMethod, args);
	TRACE_OUT;
}

} // end of namespace JNIEnv
