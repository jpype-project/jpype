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

class JPJni
{
public :
	static jclass    s_ClassClass;
	static jclass    s_StringClass;
	static jclass    s_NoSuchMethodErrorClass;
	static jclass    s_RuntimeExceptionClass;
	static jclass    s_ProxyClass;
	static jmethodID s_NewProxyInstanceID;

	static jlong     s_minByte;
	static jlong     s_maxByte;
	static jlong     s_minShort;
	static jlong     s_maxShort;
	static jlong     s_minInt;
	static jlong     s_maxInt;
	static jfloat   s_minFloat;
	static jfloat   s_maxFloat;

public :
	static void init();

	static void startJPypeReferenceQueue(bool);
	static void stopJPypeReferenceQueue();
	static void registerRef(jobject refQueue, jobject obj, jlong hostRef);

	static string asciiFromJava(jstring str);
	static JCharString unicodeFromJava(jstring str);
	
	static jstring javaStringFromJCharString(JCharString& str);
	
	static JPTypeName getClassName(jobject obj);
	static jclass getClass(jobject obj);
	static jstring toString(jobject obj);

	/**
	 * java.lang.Class.isInterface()
	 */	
	static bool            isInterface(jclass);

	/**
	 * java.lang.reflect.Modifier.isAbstract(java.lang.Class.getModifiers()
	 */	
	static bool            isAbstract(jclass);

	/**
	 * java.lang.reflect.Modifier.isFinal(java.lang.Class.getModifiers()
	 */	
	static bool            isFinal(jclass);

	/**
	 * java.lang.Class.getName()
	 */
	static JPTypeName      getName(jclass);

	/**
	 * java.lang.Class.getInterfaces()
	 */
	static vector<jclass>  getInterfaces(jclass);

	/**
	 * java.lang.Class.getDeclaredFields()
	 */
	static vector<jobject> getDeclaredFields(jclass);


	/**
	 * java.lang.Class.getConstructors()
	 */
	static vector<jobject> getConstructors(jclass);

	/**
	 * java.lang.Class.getFields()
	 */
	static vector<jobject> getFields(jclass);

	/**
	 * java.lang.Class.getDeclaredMethods()
	 */
	static vector<jobject> getDeclaredMethods(jclass);

	/**
	 * java.lang.Class.getDeclaredMethods()
	 */
	static vector<jobject> getMethods(jclass);

	/**
	 * java.lang.Class.getDeclaredMethods()
	 */
	static vector<jobject> getDeclaredConstructors(jclass);
	
	/**
	 * java.lang.Class.getModifiers()
	 */
	static long getClassModifiers(jclass);

	static jobject getSystemClassLoader();

	/**
	 * java.lang.reflect.Member.getName()
	 */
	static string getMemberName(jobject);
	
	/**
	 * java.lang.reflect.Modifier.isPublic(java.lang.reflect.member.getModifiers())
	 */
	static bool isMemberPublic(jobject);

	/**
	 * java.lang.reflect.Modifier.isStatic(java.lang.reflect.member.getModifiers())
	 */
	static bool isMemberStatic(jobject);

	/**
	 * java.lang.reflect.Modifier.isFinal(java.lang.reflect.member.getModifiers())
	 */
	static bool isMemberFinal(jobject);

	/**
	 * java.lang.reflect.Modifier.isStatic(java.lang.reflect.member.getModifiers())
	 */
	static bool isMemberAbstract(jobject);
		
	/**
	 * java.lang.reflect.Field.getType
	 */
	static JPTypeName getType(jobject fld);

	/**
	 * java.lang.reflect.Method.getReturnType
	 */
	static JPTypeName getReturnType(jobject);
	
	static jint hashCode(jobject);

	/**
	 * java.lang.reflect.Method.getParameterTypes
	 */
	static vector<JPTypeName> getParameterTypes(jobject, bool);

	static bool isConstructor(jobject);
	static string getStackTrace(jthrowable th);
	static string getMessage(jthrowable th);
	static bool isThrowable(jclass c);

	static long intValue(jobject);
	static jlong longValue(jobject);
	static double doubleValue(jobject);
	static bool booleanValue(jobject);
	static jchar charValue(jobject);

	static jclass getByteClass();
	static jclass getShortClass();
	static jclass getIntegerClass();
	static jclass getLongClass();
	static jclass getFloatClass();
	static jclass getDoubleClass();
	static jclass getCharacterClass();
	static jclass getBooleanClass();
	static jclass getVoidClass();
};

#endif // _JPJNIUTIL_H_


