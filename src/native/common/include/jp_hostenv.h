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
#ifndef _JPHOSTENV_H_
#define _JPHOSTENV_H_

class HostRef
{
public :
	HostRef(void* data, bool acquire);
	HostRef(void* data);
	virtual ~HostRef();
	
public :
	HostRef* copy();
	void release();
	bool isNull();		
	void* data();

private :
	void* m_HostData;

};


// Pre-delcare those required types
class JPArray;
class JPArrayClass;
class JPClass;
class JPMethod;
class JPObject;
class JPProxy;

class HostEnvironment
{
public :
	virtual ~HostEnvironment() {}

	virtual void* acquireRef(void*) = 0;
	virtual void releaseRef(void*) = 0;
	virtual bool isRefNull(void*) = 0;
	virtual string describeRef(HostRef*) = 0;

	virtual void* gotoExternal() = 0;
	virtual void  returnExternal(void*) = 0;
	
	virtual void setRuntimeException(const char* msg) = 0;
	virtual void setAttributeError(const char* msg) = 0;
	virtual void setTypeError(const char* msg) = 0;
	virtual void raise(const char* msg) = 0;
	
	virtual HostRef* getNone() = 0;
	virtual bool     isNone(HostRef*) = 0;

	virtual bool     isBoolean(HostRef*) = 0;
	virtual jboolean booleanAsBoolean(HostRef*) = 0;
	virtual HostRef* getTrue() = 0;
	virtual HostRef* getFalse() = 0;

	virtual bool     isSequence(HostRef*) = 0;
	virtual HostRef* newMutableSequence(jsize) = 0;
	virtual HostRef* newImmutableSequence(jsize) = 0;
	virtual jsize    getSequenceLength(HostRef*) = 0;
	virtual HostRef* getSequenceItem(HostRef*, jsize) = 0;
	virtual void     setSequenceItem(HostRef*, jsize, HostRef*) = 0;
	
	virtual bool     isInt(HostRef*) = 0;
	virtual HostRef* newInt(jint) = 0;
	virtual jint     intAsInt(HostRef*) = 0;

	virtual bool     isLong(HostRef*) = 0;
	virtual HostRef* newLong(jlong) = 0;
	virtual jlong    longAsLong(HostRef*) = 0;

	virtual bool     isFloat(HostRef*) = 0;
	virtual HostRef* newFloat(jdouble) = 0;
	virtual jdouble  floatAsDouble(HostRef*) = 0;

	virtual bool                    isMethod(HostRef*) = 0;
	virtual HostRef* newMethod(JPMethod*) = 0;
	virtual JPMethod* asMethod(HostRef*) = 0;

	virtual bool                    isObject(HostRef*) = 0;
	virtual JPObject* asObject(HostRef*) = 0;
	virtual HostRef* newObject(JPObject*) = 0;

	virtual bool                   isClass(HostRef*) = 0;
	virtual HostRef* newClass(JPClass*) = 0;
	virtual JPClass* asClass(HostRef*) = 0;

	virtual bool                        isArrayClass(HostRef*) = 0;
	virtual HostRef* newArrayClass(JPArrayClass*) = 0;
	virtual JPArrayClass* asArrayClass(HostRef*) = 0;

	virtual bool                   isArray(HostRef*) = 0;
	virtual HostRef* newArray(JPArray*) = 0;
	virtual JPArray* asArray(HostRef*) = 0;

	virtual bool                   isProxy(HostRef*) = 0;
	virtual JPProxy* asProxy(HostRef*) = 0;
	virtual HostRef* getCallableFrom(HostRef*, string&) = 0;

	virtual bool isWrapper(HostRef*) = 0;
	virtual JPTypeName getWrapperTypeName(HostRef*) = 0;
	virtual jvalue getWrapperValue(HostRef*) = 0;
	virtual HostRef* newStringWrapper(jstring) = 0;

	virtual bool     isString(HostRef*) = 0;
	virtual jsize    getStringLength(HostRef*) = 0;
	virtual string   stringAsString(HostRef*) = 0;
	virtual JCharString  stringAsJCharString(HostRef*) = 0;
	virtual HostRef* newStringFromUnicode(const jchar*, unsigned int) = 0;
	virtual HostRef* newStringFromASCII(const char*, unsigned int) = 0;
	virtual bool     isByteString(HostRef*) = 0;
	virtual bool     isUnicodeString(HostRef*) = 0;
	virtual void     getRawByteString(HostRef*, char**, long&) = 0;
	virtual void     getRawUnicodeString(HostRef*, jchar**, long&) = 0;
	virtual size_t   getUnicodeSize() = 0;

	
	virtual void* prepareCallbackBegin() = 0;
	virtual void  prepareCallbackFinish(void* state) = 0;

	virtual HostRef* callObject(HostRef* callable, vector<HostRef*>& args) = 0;

	virtual bool mapContains(HostRef* map, HostRef* key) = 0;
	virtual HostRef* getMapItem(HostRef* map, HostRef* key) = 0;	

	virtual bool objectHasAttribute(HostRef* obj, HostRef* key) = 0;
	virtual HostRef* getObjectAttribute(HostRef* obj, HostRef* key) = 0;
	
	virtual bool isJavaException(HostException*) = 0;
	virtual HostRef* getJavaException(HostException*) = 0;
	virtual void printError() = 0;
	virtual void clearError() = 0;

	virtual void printReferenceInfo(HostRef* obj) = 0;
};

#endif // _JPHOSTENV_H_
