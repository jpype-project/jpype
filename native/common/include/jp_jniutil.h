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
	string toString(jobject obj);

	// String
	string toStringUTF8(jstring str);
	jstring fromStringUTF8(const string& str);
	
	// Class	
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

	jobject getSystemClassLoader();

	string getStackTrace(jthrowable th);
	string getMessage(jthrowable th);
};

#endif // _JPJNIUTIL_H_
