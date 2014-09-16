/*****************************************************************************
   Copyright 2004 Steve Ménard

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
#ifndef _JPJNIUTIL_H_
#define _JPJNIUTIL_H_

namespace JPJni
{
	extern jclass s_ClassClass;
	extern jclass s_StringClass;
	extern jclass s_NoSuchMethodErrorClass;
	extern jclass s_RuntimeExceptionClass;
	extern jclass s_ProxyClass;
	extern jmethodID s_NewProxyInstanceID;

	extern jlong s_minByte;
	extern jlong s_maxByte;
	extern jlong s_minShort;
	extern jlong s_maxShort;
	extern jlong s_minInt;
	extern jlong s_maxInt;
	extern jfloat s_minFloat;
	extern jfloat s_maxFloat;

	void init();

	void startJPypeReferenceQueue(bool);
	void stopJPypeReferenceQueue();
	void registerRef(jobject refQueue, jobject obj, jlong hostRef);

	string asciiFromJava(jstring str);
	JCharString unicodeFromJava(jstring str);

	jstring javaStringFromJCharString(JCharString& str);
	
	JPTypeName getClassName(jobject obj);
	jclass getClass(jobject obj);
	jstring toString(jobject obj);

	/**
	* java.lang.Class.isInterface()
	*/
	bool            isInterface(jclass);

	/**
	* java.lang.reflect.Modifier.isAbstract(java.lang.Class.getModifiers()
	*/
	bool            isAbstract(jclass);

	/**
	* java.lang.reflect.Modifier.isFinal(java.lang.Class.getModifiers()
	*/
	bool            isFinal(jclass);

	/**
	* java.lang.Class.getName()
	*/
	JPTypeName      getName(jclass);

	/**
	* java.lang.Class.getInterfaces()
	*/
	vector<jclass>  getInterfaces(jclass);

	/**
	* java.lang.Class.getDeclaredFields()
	*/
	vector<jobject> getDeclaredFields(jclass);


	/**
	* java.lang.Class.getConstructors()
	*/
	vector<jobject> getConstructors(jclass);

	/**
	* java.lang.Class.getFields()
	*/
	vector<jobject> getFields(jclass);

	/**
	* java.lang.Class.getDeclaredMethods()
	*/
	vector<jobject> getDeclaredMethods(jclass);

	/**
	* java.lang.Class.getDeclaredMethods()
	*/
	vector<jobject> getMethods(jclass);

	/**
	* java.lang.Class.getDeclaredMethods()
	*/
	vector<jobject> getDeclaredConstructors(jclass);
	
	/**
	* java.lang.Class.getModifiers()
	*/
	long getClassModifiers(jclass);

	jobject getSystemClassLoader();

	/**
	* java.lang.reflect.Member.getName()
	*/
	string getMemberName(jobject);
	
	/**
	* java.lang.reflect.Modifier.isPublic(java.lang.reflect.member.getModifiers())
	*/
	bool isMemberPublic(jobject);

	/**
	* java.lang.reflect.Modifier.is(java.lang.reflect.member.getModifiers())
	*/
	bool isMemberStatic(jobject);

	/**
	* java.lang.reflect.Modifier.isFinal(java.lang.reflect.member.getModifiers())
	*/
	bool isMemberFinal(jobject);

	/**
	* java.lang.reflect.Modifier.is(java.lang.reflect.member.getModifiers())
	*/
	bool isMemberAbstract(jobject);

	/**
	* java.lang.reflect.Field.getType
	*/
	JPTypeName getType(jobject fld);

	/**
	* java.lang.reflect.Method.getReturnType
	*/
	JPTypeName getReturnType(jobject);
	
	jint hashCode(jobject);

	/**
	* java.lang.reflect.Method.getParameterTypes
	*/
	vector<JPTypeName> getParameterTypes(jobject, bool);

	bool isConstructor(jobject);
	string getStackTrace(jthrowable th);
	string getMessage(jthrowable th);
	bool isThrowable(jclass c);

	long intValue(jobject);
	jlong longValue(jobject);
	double doubleValue(jobject);
	bool booleanValue(jobject);
	jchar charValue(jobject);

	jclass getByteClass();
	jclass getShortClass();
	jclass getIntegerClass();
	jclass getLongClass();
	jclass getFloatClass();
	jclass getDoubleClass();
	jclass getCharacterClass();
	jclass getBooleanClass();
	jclass getVoidClass();
};

#endif // _JPJNIUTIL_H_
