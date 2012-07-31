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
#ifndef _JPTYPE_H_
#define _JPTYPE_H_

enum EMatchType
{
	_none,
	_explicit,
	_implicit,
	_exact
};

/**
 * Base class for all JPype Types, be it primitive, class or array
 */
class JPType
{
protected :
	JPType()
	{
	}
	
	virtual ~JPType()
	{
	}
	
public :
	virtual HostRef*   getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType) = 0 ;
	virtual void       setStaticValue(jclass c, jfieldID fid, HostRef* val) = 0 ;

	virtual HostRef*   getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType) = 0 ;
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val) = 0 ;

	virtual HostRef*   asHostObject(jvalue val) = 0 ;
	virtual HostRef*   asHostObjectFromObject(jvalue val) = 0;
	
	virtual JPTypeName getName() = 0;
	
	virtual EMatchType canConvertToJava(HostRef* obj) = 0;

	virtual jvalue     convertToJava(HostRef* obj) = 0;
	virtual jobject    convertToJavaObject(HostRef* obj) = 0;

	virtual bool       isObjectType() = 0;
	virtual JPTypeName getObjectType() = 0;
	
	virtual HostRef*   invokeStatic(jclass, jmethodID, jvalue*) = 0;
	virtual HostRef*   invoke(jobject, jclass, jmethodID, jvalue*) = 0;
	
	virtual jarray     newArrayInstance(int size) = 0;
	virtual vector<HostRef*>   getArrayRange(jarray, int start, int length) = 0;
	virtual void       setArrayRange(jarray, int start, int length, vector<HostRef*>& vals) = 0;
	virtual HostRef*   getArrayItem(jarray, int ndx) = 0;
	virtual void       setArrayItem(jarray, int ndx, HostRef* val) = 0;
	virtual void       setArrayValues(jarray, HostRef*) = 0;

	virtual HostRef*   convertToDirectBuffer(HostRef* src) = 0;
};

#endif // _JPTYPE_H_
