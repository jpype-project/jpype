/*****************************************************************************
   Copyright 2004 Steve M�nard

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
#ifndef _PYHOSTENV_H_
#define _PYHOSTENV_H_


class PythonHostEnvironment : public HostEnvironment
{
public :
	PythonHostEnvironment()
	{
	}
	
	virtual ~PythonHostEnvironment()
	{
	}
	
public :
	void setJavaLangObjectClass(PyObject* obj)
	{
		m_JavaLangObject = obj;
	}

	PyObject* getJavaLangObjectClass()
	{
		return m_JavaLangObject;
	}

	void setJavaArrayClass(PyObject* obj)
	{
		m_JavaArrayClass = obj;
	}

	void setWrapperClass(PyObject* obj)
	{
		m_WrapperClass = obj;
	}

	void setStringWrapperClass(PyObject* obj)
	{
		m_StringWrapperClass = obj;
	}

	void setProxyClass(PyObject* obj)
	{
		m_ProxyClass = obj;
	}

	void setGetJavaClassMethod(PyObject* obj)
	{
		m_GetClassMethod = obj;
		Py_INCREF(obj);
	}


	void setGetJavaArrayClassMethod(PyObject* obj)
	{
		m_GetArrayClassMethod = obj;
		Py_INCREF(obj);
	}

	void setSpecialConstructorKey(PyObject* obj)
	{
		m_SpecialConstructorKey = obj;
		Py_INCREF(obj);
	}

	PyObject* getSpecialConstructorKey()
	{
		return m_SpecialConstructorKey;
	}

	void setJavaExceptionClass(PyObject* obj)
	{
		m_JavaExceptionClass = obj;
	}  

	static void deleteJPObjectDestructor(void* data, void* desc);

	static void deleteJPArrayDestructor(void* data, void* desc)
	{
		delete (JPArray*)data;
	}

	static void deleteObjectJValueDestructor(void* data, void* desc)
	{
		jvalue* pv = (jvalue*)data;
		JPEnv::getJava()->DeleteGlobalRef(pv->l);
		delete pv;
	}

	static void deleteJValueDestructor(void* data, void* desc)
	{
		jvalue* pv = (jvalue*)data;
		delete pv;
	}

	static void deleteJPProxyDestructor(void* data, void* desc)
	{
		JPProxy* pv = (JPProxy*)data;
		delete pv;
	}

	

	PyObject* getJavaShadowClass(JPClass* jc);

private :
	PyObject* m_JavaLangObject;
	PyObject* m_JavaArrayClass;
	PyObject* m_WrapperClass;
	PyObject* m_StringWrapperClass;
	PyObject* m_ProxyClass;
	map<string, PyObject*> m_ClassMap;
	PyObject* m_GetClassMethod;
	PyObject* m_GetArrayClassMethod;


public :
	PyObject* m_SpecialConstructorKey;
	PyObject* m_JavaExceptionClass;

public :
	virtual void* acquireRef(void*);
	virtual void releaseRef(void*);
	virtual bool isRefNull(void*);
	virtual string describeRef(HostRef*);

	virtual void* gotoExternal();
	virtual void returnExternal(void* state);

	virtual void setRuntimeException(const char* msg);
	virtual void setAttributeError(const char* msg);
	virtual void setTypeError(const char* msg);
	virtual void raise(const char* msg);
	
	virtual HostRef* getNone();
	virtual bool isNone(HostRef*);

	virtual bool     isBoolean(HostRef*);
	virtual jboolean booleanAsBoolean(HostRef*);
	virtual HostRef* getTrue();
	virtual HostRef* getFalse();

	virtual bool isSequence(HostRef*);
	virtual HostRef* newMutableSequence(jsize);
	virtual HostRef* newImmutableSequence(jsize);
	virtual jsize getSequenceLength(HostRef*);
	virtual HostRef* getSequenceItem(HostRef*, jsize);
	virtual void setSequenceItem(HostRef*, jsize, HostRef*);
	
	virtual bool isInt(HostRef*);
	virtual HostRef* newInt(jint);
	virtual jint intAsInt(HostRef*);

	virtual bool isLong(HostRef*);
	virtual HostRef* newLong(jlong);
	virtual jlong longAsLong(HostRef*);

	virtual bool isFloat(HostRef*);
	virtual HostRef* newFloat(jdouble);
	virtual jdouble floatAsDouble(HostRef*);

	virtual bool isMethod(HostRef*);
	virtual HostRef* newMethod(JPMethod*);
	virtual JPMethod* asMethod(HostRef*);

	virtual bool isObject(HostRef*);
	virtual HostRef* newObject(JPObject*);
	virtual JPObject* asObject(HostRef*);

	virtual bool isClass(HostRef*);
	virtual HostRef* newClass(JPClass*);
	virtual JPClass* asClass(HostRef*);

	virtual bool isArrayClass(HostRef*);
	virtual HostRef* newArrayClass(JPArrayClass*);
	virtual JPArrayClass* asArrayClass(HostRef*);

	virtual bool isArray(HostRef*);
	virtual HostRef* newArray(JPArray*);
	virtual JPArray* asArray(HostRef*);

	virtual bool                   isProxy(HostRef*);
	virtual JPProxy* asProxy(HostRef*);
	virtual HostRef* getCallableFrom(HostRef*, string&);

	virtual bool isWrapper(PyObject*) ;
	virtual JPTypeName getWrapperTypeName(PyObject*);
	virtual jvalue getWrapperValue(PyObject*);

	virtual bool isWrapper(HostRef*);
	virtual JPTypeName getWrapperTypeName(HostRef*);
	virtual jvalue getWrapperValue(HostRef*);
	virtual HostRef* newStringWrapper(jstring);

	virtual bool isString(HostRef*);
	virtual jsize getStringLength(HostRef*);
	virtual string   stringAsString(HostRef*);
	virtual JCharString stringAsJCharString(HostRef*);
	virtual HostRef* newStringFromUnicode(const jchar*, unsigned int);
	virtual HostRef* newStringFromASCII(const char*, unsigned int);
	virtual bool     isByteString(HostRef*);
	virtual bool     isUnicodeString(HostRef* ref);
	virtual void     getRawByteString(HostRef*, char**, long&);
	virtual void     getRawUnicodeString(HostRef*, jchar**, long&);
	virtual size_t   getUnicodeSize();

	virtual void* prepareCallbackBegin();
	virtual void  prepareCallbackFinish(void* state);

	virtual HostRef* callObject(HostRef* callable, vector<HostRef*>& args);

	virtual void printError();

	virtual bool mapContains(HostRef* map, HostRef* key);
	virtual HostRef* getMapItem(HostRef* map, HostRef* key);	

	virtual bool objectHasAttribute(HostRef* obj, HostRef* key);
	virtual HostRef* getObjectAttribute(HostRef* obj, HostRef* key);

	virtual bool isJavaException(HostException*);
	virtual HostRef* getJavaException(HostException*);
	virtual void clearError();
	virtual void printReferenceInfo(HostRef* obj);
};

#endif // _PYHOSTENV_H_
