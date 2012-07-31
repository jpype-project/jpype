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
#ifndef _JPPOBJECTTYPE_H_
#define _JPPOBJECTTYPE_H_

class JPObjectType : public JPType
{
protected :
	JPObjectType(JPTypeName::ETypes type, JPTypeName objectType) :
		m_Type(JPTypeName::fromType(type)),
		m_ObjectTypeName(objectType)
	{
	}
	
	virtual ~JPObjectType() 
	{
	}
	
public :
	virtual JPTypeName getName()
	{
		return m_Type;
	}
	
	virtual JPTypeName getObjectType()
	{
		return m_ObjectTypeName;
	}

	virtual bool      isObjectType() 
	{ 
		return true; 
	}

	virtual HostRef* getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType);
	virtual void      setStaticValue(jclass c, jfieldID fid, HostRef* val);
	virtual HostRef* getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType);
	virtual void      setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*   asHostObjectFromObject(jvalue val);

	virtual jobject convertToJavaObject(HostRef* obj);

	virtual HostRef* invokeStatic(jclass, jmethodID, jvalue*);
	virtual HostRef* invoke(jobject, jclass clazz, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual void       setArrayValues(jarray, HostRef*);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);

protected :
	virtual jclass    getClass() = 0;	
	
private :
	JPTypeName m_Type;
	JPTypeName m_ObjectTypeName;
};

class JPStringType : public JPObjectType
{
public :
	JPStringType() : JPObjectType(JPTypeName::_string, JPTypeName::fromSimple("java.lang.String"))
	{
	}
	
	virtual ~JPStringType()
	{
	}

protected :
	virtual jclass    getClass();	

public : // JPType implementation	
	virtual HostRef*  asHostObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
};

class JPClassType : public JPObjectType
{
public :
	JPClassType() : JPObjectType(JPTypeName::_class, JPTypeName::fromSimple("java.lang.Class"))
	{
	}
	
	virtual ~JPClassType()
	{
	}

protected :
	virtual jclass    getClass();	

public : // JPType implementation
	virtual HostRef*  asHostObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
};

#endif // _JPPOBJECTTYPE_H_
