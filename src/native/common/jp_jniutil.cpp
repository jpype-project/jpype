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

jclass JPJni::s_ClassClass;
jclass JPJni::s_StringClass;
jclass JPJni::s_NoSuchMethodErrorClass;
jclass JPJni::s_RuntimeExceptionClass;
jclass JPJni::s_ProxyClass;
jmethodID JPJni::s_NewProxyInstanceID;

jlong   JPJni::s_minByte;
jlong   JPJni::s_maxByte;
jlong   JPJni::s_minShort;
jlong   JPJni::s_maxShort;
jlong   JPJni::s_minInt;
jlong   JPJni::s_maxInt;
jfloat   JPJni::s_minFloat;
jfloat   JPJni::s_maxFloat;

static jclass objectClass;
static jmethodID getClassID;
static jmethodID toStringID;
static jmethodID hashCodeID;

static jmethodID getNameID;
static jmethodID getDeclaredFieldsID;
static jmethodID getDeclaredMethodsID;
static jmethodID getInterfacesID;
static jmethodID getFieldsID;
static jmethodID getMethodsID;
static jmethodID getDeclaredConstructorsID;
static jmethodID getConstructorsID;
static jmethodID isInterfaceID;
static jmethodID getClassModifiersID;

static jclass modifierClass;
static jmethodID isStaticID;
static jmethodID isPublicID;
static jmethodID isAbstractID;
static jmethodID isFinalID;

static jclass classLoaderClass;
static jmethodID getSystemClassLoaderID;

static jclass memberClass;
static jmethodID getModifiersID;
static jmethodID getMemberNameID;

static jclass fieldClass;
static jmethodID getTypeID;

static jclass methodClass;
static jclass constructorClass;
static jmethodID getReturnTypeID;
static jmethodID getParameterTypesID;
static jmethodID getConstructorParameterTypesID;

static jclass throwableClass;
static jmethodID getMessageID;
static jmethodID printStackTraceID;
static jclass stringWriterClass;
static jclass printWriterClass;
static jmethodID stringWriterID;
static jmethodID printWriterID;
static jmethodID flushID;

static jclass numberClass;
static jclass booleanClass;
static jclass charClass;
static jclass byteClass;
static jclass shortClass;
static jclass intClass;
static jclass floatClass;
static jmethodID intValueID;
static jmethodID longValueID;
static jmethodID doubleValueID;
static jmethodID booleanValueID;
static jmethodID charValueID;

static jclass    JPypeReferenceClass;
static jmethodID JPypeReferenceConstructorMethod;
static jclass    JPypeReferenceQueueClass;
static jmethodID JPypeReferenceQueueConstructorMethod;
static jmethodID JPypeReferenceQueueRegisterMethod;
static jmethodID JPypeReferenceQueueStartMethod;
static jmethodID JPypeReferenceQueueRunMethod;
static jmethodID JPypeReferenceQueueStopMethod;



void JPJni::init() 
{
	objectClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Object;"));
	s_StringClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/String;"));
	getClassID = JPEnv::getJava()->GetMethodID(objectClass, "getClass", "()Ljava/lang/Class;");
	toStringID = JPEnv::getJava()->GetMethodID(objectClass, "toString", "()Ljava/lang/String;");
	hashCodeID = JPEnv::getJava()->GetMethodID(objectClass, "hashCode", "()I");

	s_ClassClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Class;"));
	getNameID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getName", "()Ljava/lang/String;");
	getDeclaredFieldsID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
	getDeclaredMethodsID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
	getMethodsID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getMethods", "()[Ljava/lang/reflect/Method;");
	getDeclaredConstructorsID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getDeclaredConstructors", "()[Ljava/lang/reflect/Constructor;");
	getConstructorsID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getConstructors", "()[Ljava/lang/reflect/Constructor;");
	isInterfaceID = JPEnv::getJava()->GetMethodID(s_ClassClass, "isInterface", "()Z");
	getClassModifiersID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getModifiers", "()I");
	getInterfacesID = JPEnv::getJava()->GetMethodID(s_ClassClass, "getInterfaces", "()[Ljava/lang/Class;");

	modifierClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/reflect/Modifier;"));
	isStaticID = JPEnv::getJava()->GetStaticMethodID(modifierClass, "isStatic", "(I)Z");
	isPublicID = JPEnv::getJava()->GetStaticMethodID(modifierClass, "isPublic", "(I)Z");
	isAbstractID = JPEnv::getJava()->GetStaticMethodID(modifierClass, "isAbstract", "(I)Z");
	isFinalID = JPEnv::getJava()->GetStaticMethodID(modifierClass, "isFinal", "(I)Z");

	classLoaderClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/ClassLoader;"));
	getSystemClassLoaderID = JPEnv::getJava()->GetStaticMethodID(classLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");  

	s_NoSuchMethodErrorClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/NoSuchMethodError;") );
	s_RuntimeExceptionClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/RuntimeException;") );

	s_ProxyClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/reflect/Proxy;") );
	s_NewProxyInstanceID = JPEnv::getJava()->GetStaticMethodID(s_ProxyClass, "newProxyInstance", "(Ljava/lang/ClassLoader;[Ljava/lang/Class;Ljava/lang/reflect/InvocationHandler;)Ljava/lang/Object;");

	memberClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/reflect/Member;"));
	getModifiersID = JPEnv::getJava()->GetMethodID(memberClass, "getModifiers", "()I");
	getMemberNameID = JPEnv::getJava()->GetMethodID(memberClass, "getName", "()Ljava/lang/String;");

	fieldClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/reflect/Field;"));
	getTypeID = JPEnv::getJava()->GetMethodID(fieldClass, "getType", "()Ljava/lang/Class;");

	methodClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/reflect/Method;"));
	constructorClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/reflect/Constructor;"));
	getReturnTypeID = JPEnv::getJava()->GetMethodID(methodClass, "getReturnType", "()Ljava/lang/Class;");
	getParameterTypesID = JPEnv::getJava()->GetMethodID(methodClass, "getParameterTypes", "()[Ljava/lang/Class;");
	getConstructorParameterTypesID = JPEnv::getJava()->GetMethodID(constructorClass, "getParameterTypes", "()[Ljava/lang/Class;");

	throwableClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Throwable;"));
	getMessageID = JPEnv::getJava()->GetMethodID(throwableClass, "getMessage", "()Ljava/lang/String;");
	printStackTraceID = JPEnv::getJava()->GetMethodID(throwableClass, "printStackTrace", "(Ljava/io/PrintWriter;)V");
	stringWriterClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/io/StringWriter;"));
	printWriterClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/io/PrintWriter;"));
	stringWriterID = JPEnv::getJava()->GetMethodID(stringWriterClass, "<init>", "()V");
	printWriterID = JPEnv::getJava()->GetMethodID(printWriterClass, "<init>", "(Ljava/io/Writer;)V");
	flushID = JPEnv::getJava()->GetMethodID(printWriterClass, "flush", "()V");
	
	numberClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Number;"));
	booleanClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Boolean;"));
	charClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Character;"));
	intValueID = JPEnv::getJava()->GetMethodID(numberClass, "intValue", "()I");
	longValueID = JPEnv::getJava()->GetMethodID(numberClass, "longValue", "()J");
	doubleValueID = JPEnv::getJava()->GetMethodID(numberClass, "doubleValue", "()D");
	booleanValueID = JPEnv::getJava()->GetMethodID(booleanClass, "booleanValue", "()Z");
	charValueID = JPEnv::getJava()->GetMethodID(charClass, "charValue", "()C");

	byteClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Byte;"));
	shortClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Short;"));
	intClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Integer;"));
	floatClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Float;"));

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

string JPJni::asciiFromJava(jstring str) 
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

JCharString JPJni::unicodeFromJava(jstring str) 
{
	const jchar* cstr = NULL;
	jboolean isCopy;
	cstr = JPEnv::getJava()->GetStringChars(str, &isCopy);

	JCharString res = cstr;

	JPEnv::getJava()->ReleaseStringChars(str, cstr);

	return res;	
}

jstring JPJni::javaStringFromJCharString(JCharString& wstr)
{
	jstring result = JPEnv::getJava()->NewString(wstr.c_str(), (jint)wstr.length());
	
	return result;
}

JPTypeName JPJni::getClassName(jobject o) 
{
	if (o == NULL)
	{
		return JPTypeName::fromSimple("java.lang.Object");
	}

	JPCleaner cleaner;
	jclass c = getClass(o);
	cleaner.addLocal(c);
	return JPJni::getName(c);
}

jclass JPJni::getClass(jobject o) 
{
	return (jclass)JPEnv::getJava()->CallObjectMethod(o, getClassID);
}

jstring JPJni::toString(jobject o) 
{
	jstring str = (jstring)JPEnv::getJava()->CallObjectMethod(o, toStringID);
	return str;
}

static string convertToSimpleName(jclass c)  
{
	JPCleaner cleaner;
	jstring jname = (jstring)JPEnv::getJava()->CallObjectMethod(c, getNameID);
	cleaner.addLocal(jname);
	string name = JPJni::asciiFromJava(jname);
	
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

bool JPJni::isInterface(jclass clazz) 
{
	jboolean b = JPEnv::getJava()->CallBooleanMethod(clazz, isInterfaceID);
	return (b ? true : false);
}

bool JPJni::isAbstract(jclass clazz) 
{
	jvalue modif;
	modif.i = JPEnv::getJava()->CallIntMethod(clazz, getClassModifiersID);
	jboolean res = JPEnv::getJava()->CallStaticBooleanMethodA(modifierClass, isAbstractID, &modif);
	
	return (res ? true : false);
}

long JPJni::getClassModifiers(jclass clazz) 
{
	long res = JPEnv::getJava()->CallIntMethod(clazz, getClassModifiersID);
	
	return res;
}


bool JPJni::isFinal(jclass clazz) 
{
	jvalue modif;
	modif.i = JPEnv::getJava()->CallIntMethod(clazz, getClassModifiersID);
	jboolean res = JPEnv::getJava()->CallStaticBooleanMethodA(modifierClass, isFinalID, &modif);
	
	return (res ? true : false);
}

JPTypeName JPJni::getName(jclass clazz) 
{
	string simpleName = convertToSimpleName(clazz);
	
	return JPTypeName::fromSimple(simpleName.c_str());
}

vector<jclass> JPJni::getInterfaces(jclass clazz) 
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

vector<jobject> JPJni::getDeclaredFields(jclass clazz) 
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

vector<jobject> JPJni::getFields(jclass clazz) 
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

vector<jobject> JPJni::getDeclaredMethods(jclass clazz) 
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


vector<jobject> JPJni::getDeclaredConstructors(jclass clazz) 
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

vector<jobject> JPJni::getConstructors(jclass clazz) 
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

vector<jobject> JPJni::getMethods(jclass clazz) 
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

jobject JPJni::getSystemClassLoader()
{
	return JPEnv::getJava()->CallStaticObjectMethod(classLoaderClass, getSystemClassLoaderID) ; 
}

string JPJni::getMemberName(jobject o) 
{
	JPCleaner cleaner;
	jstring name = (jstring)JPEnv::getJava()->CallObjectMethod(o, getMemberNameID);
	cleaner.addLocal(name);

	string simpleName = JPJni::asciiFromJava(name);
	return simpleName;
}
	
bool JPJni::isMemberPublic(jobject o) 
{
	jvalue modif;
	modif.i = JPEnv::getJava()->CallIntMethod(o, getModifiersID);
	jboolean res = JPEnv::getJava()->CallStaticBooleanMethodA(modifierClass, isPublicID, &modif);
	
	return (res ? true : false);
}

bool JPJni::isMemberStatic(jobject o) 
{
	jvalue modif;
	modif.i = JPEnv::getJava()->CallIntMethod(o, getModifiersID);
	jboolean res = JPEnv::getJava()->CallStaticBooleanMethodA(modifierClass, isStaticID, &modif);
	
	return (res ? true : false);
}

bool JPJni::isMemberFinal(jobject o) 
{
	jvalue modif;
	modif.i = JPEnv::getJava()->CallIntMethod(o, getModifiersID);
	jboolean res = JPEnv::getJava()->CallStaticBooleanMethodA(modifierClass, isFinalID, &modif);
	
	return (res ? true : false);
}

bool JPJni::isMemberAbstract(jobject o) 
{
	jvalue modif;
	modif.i = JPEnv::getJava()->CallIntMethod(o, getModifiersID);
	jboolean res = JPEnv::getJava()->CallStaticBooleanMethodA(modifierClass, isAbstractID, &modif);
	
	return (res ? true : false);
}

jint JPJni::hashCode(jobject obj)
{
	jint res = JPEnv::getJava()->CallIntMethod(obj, hashCodeID);
	return res;
}

JPTypeName JPJni::getType(jobject fld) 
{
	TRACE_IN("JPJni::getType");
	
	JPCleaner cleaner;
	jclass c = (jclass)JPEnv::getJava()->CallObjectMethod(fld, getTypeID);
	cleaner.addLocal(c);
	
	return JPJni::getName(c);
	TRACE_OUT;
}

JPTypeName JPJni::getReturnType(jobject o) 
{
	JPCleaner cleaner;
	jclass c = (jclass)JPEnv::getJava()->CallObjectMethod(o, getReturnTypeID);
	cleaner.addLocal(c);
	
	return JPJni::getName(c);
}

vector<JPTypeName> JPJni::getParameterTypes(jobject o, bool isConstructor) 
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

		JPTypeName name = JPJni::getName(c);
		args.push_back(name);
	}
	
	return args;
}

bool JPJni::isConstructor(jobject obj)
{
	jboolean r = JPEnv::getJava()->IsInstanceOf(obj, constructorClass);
	if (r)
	{
		return true;
	}
	return false;
}

string JPJni::getStackTrace(jthrowable th)
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

	jstring res = JPJni::toString(strWriter);
	cleaner.addLocal(res);

	return JPJni::asciiFromJava(res);
}

string JPJni::getMessage(jthrowable th)
{
	JPCleaner cleaner;
	jstring jstr = (jstring)JPEnv::getJava()->CallObjectMethod(th, getMessageID);
	cleaner.addLocal(jstr);

	return JPJni::asciiFromJava(jstr);
}

bool JPJni::isThrowable(jclass c)
{
	jboolean res = JPEnv::getJava()->IsAssignableFrom(c, throwableClass);
	if (res)
	{
		return true;
	}
	return false;
}

long JPJni::intValue(jobject obj)
{
	return JPEnv::getJava()->CallIntMethod(obj, intValueID);	
}

jlong JPJni::longValue(jobject obj)
{
	return JPEnv::getJava()->CallLongMethod(obj, longValueID);	
}

double JPJni::doubleValue(jobject obj)
{
	return JPEnv::getJava()->CallDoubleMethod(obj, doubleValueID);	
}

bool JPJni::booleanValue(jobject obj)
{
	return JPEnv::getJava()->CallBooleanMethod(obj, booleanValueID) ? true : false;	
}

jchar JPJni::charValue(jobject obj)
{
	return JPEnv::getJava()->CallCharMethod(obj, charValueID);	
}

jclass JPJni::getByteClass()
{
	jclass byteClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Byte;"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(byteClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(byteClass, fid);
	JPEnv::getJava()->DeleteLocalRef(byteClass);
	return res;
}

jclass JPJni::getShortClass()
{
	jclass shortClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Short;"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(shortClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(shortClass, fid);
	JPEnv::getJava()->DeleteLocalRef(shortClass);
	return res;
}

jclass JPJni::getIntegerClass()
{
	jclass intClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Integer;"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(intClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(intClass, fid);
	JPEnv::getJava()->DeleteLocalRef(intClass);
	return res;
}

jclass JPJni::getLongClass()
{
	jclass longClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Long;"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(longClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(longClass, fid);
	JPEnv::getJava()->DeleteLocalRef(longClass);
	return res;
}

jclass JPJni::getFloatClass()
{
	jclass floatClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Float;"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(floatClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(floatClass, fid);
	JPEnv::getJava()->DeleteLocalRef(floatClass);
	return res;
}

jclass JPJni::getDoubleClass()
{
	jclass doubleClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Double;"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(doubleClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(doubleClass, fid);
	JPEnv::getJava()->DeleteLocalRef(doubleClass);
	return res;
}

jclass JPJni::getCharacterClass()
{
	jclass charClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Character;"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(charClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(charClass, fid);
	JPEnv::getJava()->DeleteLocalRef(charClass);
	return res;
}

jclass JPJni::getBooleanClass()
{
	jclass booleanClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Boolean;"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(booleanClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(booleanClass, fid);
	JPEnv::getJava()->DeleteLocalRef(booleanClass);
	return res;
}

jclass JPJni::getVoidClass()
{
	jclass voidClass= (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljava/lang/Void;"));
	jfieldID fid = JPEnv::getJava()->GetStaticFieldID(voidClass, "TYPE", "Ljava/lang/Class;");
	jclass res = (jclass)JPEnv::getJava()->GetStaticObjectField(voidClass, fid);
	JPEnv::getJava()->DeleteLocalRef(voidClass);
	return res;
}

void JPJni::startJPypeReferenceQueue(bool useJavaThread)
{
	JPCleaner cleaner;

	JPypeReferenceQueueClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljpype/ref/JPypeReferenceQueue;"));
	JPypeReferenceQueueConstructorMethod = JPEnv::getJava()->GetMethodID(JPypeReferenceQueueClass, "<init>", "()V");
	JPypeReferenceQueueRegisterMethod = JPEnv::getJava()->GetMethodID(JPypeReferenceQueueClass, "registerRef", "(Ljpype/ref/JPypeReference;J)V");
	JPypeReferenceQueueStartMethod = JPEnv::getJava()->GetMethodID(JPypeReferenceQueueClass, "startManaging", "()V");
	JPypeReferenceQueueRunMethod = JPEnv::getJava()->GetMethodID(JPypeReferenceQueueClass, "run", "()V");
	JPypeReferenceQueueStopMethod = JPEnv::getJava()->GetMethodID(JPypeReferenceQueueClass, "stop", "()V");

	JPypeReferenceClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("Ljpype/ref/JPypeReference;"));
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

void JPJni::stopJPypeReferenceQueue()
{
	JPEnv::getJava()->CallVoidMethod(JPEnv::getJava()->getReferenceQueue(), JPypeReferenceQueueStopMethod);
}

void JPJni::registerRef(jobject refQueue, jobject obj, jlong hostRef)
{
	TRACE_IN("JPJni::registerRef");
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

