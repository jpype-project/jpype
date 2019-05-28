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
	extern jclass s_ObjectClass;
	extern jclass s_ClassClass;
	extern jclass s_StringClass;
	extern jclass s_NoSuchMethodErrorClass;
	extern jclass s_RuntimeExceptionClass;
	extern jclass s_ProxyClass;
	extern jmethodID s_NewProxyInstanceID;

	extern jlong s_Byte_Min;
	extern jlong s_Byte_Max;
	extern jlong s_Short_Min;
	extern jlong s_Short_Max;
	extern jlong s_Int_Min;
	extern jlong s_Int_Max;
	extern jfloat s_Float_Min;
	extern jfloat s_Float_Max;

	void init();

	// Object
	jclass getClass(jobject obj);
	string toString(jobject obj);

	// String
	string toStringUTF8(jstring str);
	jstring fromStringUTF8(const string& str);
	jobject stringToCharArray(jstring str);

	// Class	
	jclass getComponentType(jclass cls);
	string convertToSimpleName(jclass cls);
	string getCanonicalName(jclass cls);
	bool isArray(jclass cls);
	bool isThrowable(jclass c);

	/**
	 * java.lang.Class.isInterface()
	 */
	bool isInterface(jclass cls);

	/**
	 * java.lang.reflect.Modifier.isAbstract(java.lang.Class.getModifiers()
	 */
	bool isAbstract(jclass cls);

	/**
	 * java.lang.reflect.Modifier.isFinal(java.lang.Class.getModifiers()
	 */
	bool            isFinal(jclass cls);

	/**
	 * java.lang.Class.getInterfaces()
	 */
	vector<jclass>  getInterfaces(JPJavaFrame& frame, jclass cls);

	/**
	 * java.lang.Class.getDeclaredFields()
	 */
	vector<jobject> getDeclaredFields(JPJavaFrame& frame, jclass cls);


	/**
	 * java.lang.Class.getConstructors()
	 */
	vector<jobject> getConstructors(JPJavaFrame& frame, jclass cls);

	/**
	 * java.lang.Class.getFields()
	 */
	vector<jobject> getFields(JPJavaFrame& frame, jclass cls);

	/**
	 * java.lang.Class.getDeclaredMethods()
	 */
	vector<jobject> getDeclaredMethods(JPJavaFrame& frame, jclass cls);

	/**
	 * java.lang.Class.getDeclaredMethods()
	 */
	vector<jobject> getMethods(JPJavaFrame& frame, jclass cls);

	/**
	 * java.lang.Class.getDeclaredMethods()
	 */
	vector<jobject> getDeclaredConstructors(JPJavaFrame& frame, jclass cls);

	/**
	 * java.lang.Class.getModifiers()
	 */
	long getClassModifiers(jclass cls);

	jobject getSystemClassLoader();

	/**
	 * java.lang.reflect.Member.getName()
	 */
	string getMemberName(jobject member);

	/**
	 * java.lang.reflect.Modifier.isPublic(java.lang.reflect.member.getModifiers())
	 */
	bool isMemberPublic(jobject member);

	/**
	 * java.lang.reflect.Modifier.is(java.lang.reflect.member.getModifiers())
	 */
	bool isMemberStatic(jobject member);

	/**
	 * java.lang.reflect.Modifier.isFinal(java.lang.reflect.member.getModifiers())
	 */
	bool isMemberFinal(jobject member);

	/**
	 * java.lang.reflect.Modifier.isAbstract(java.lang.reflect.member.getModifiers())
	 */
	bool isMemberAbstract(jobject member);

	// Field
	/**
	 * java.lang.reflect.Modifier.isPublic(java.lang.reflect.Field.getModifiers()
	 */
	bool isFieldPublic(jobject field);

	/**
	 * java.lang.reflect.Field.getType
	 */
	jclass getFieldType(jobject field);

	// Method
	/**
	 * java.lang.reflect.Method.getReturnType
	 */
	jclass getMethodReturnType(jobject method);

	/**
	 * java.lang.reflect.Method.isSynthetic()
	 */
	bool isMethodSynthetic(jobject method);

	/**
	 * java.lang.reflect.Method.isSynthetic()
	 */
	bool isMethodVarArgs(jobject);

	/**
	 * java.lang.reflect.Method.getParameterTypes
	 */
	vector<JPClassRef> getMethodParameterTypes(jobject, bool);

	bool isConstructor(jobject);

	string getStackTrace(jthrowable th);
	string getMessage(jthrowable th);

	// Boxed class methods
	long intValue(jobject);
	jlong longValue(jobject);
	double doubleValue(jobject);
	bool booleanValue(jobject);
	jchar charValue(jobject);

	/** Look up the class for a java primitive.
	 * @param name is the name of the Boxed type in native format.
	 */
	jclass getPrimitiveClass(jclass cls);

};

#endif // _JPJNIUTIL_H_
