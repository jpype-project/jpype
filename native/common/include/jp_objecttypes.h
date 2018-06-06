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
	JPObjectType(JPTypeName::ETypes type, const JPTypeName& objectType) :
		m_Type(JPTypeName::fromType(type)),
		m_ObjectTypeName(objectType)
	{
	}
	
	virtual ~JPObjectType()
	{
	}
	
public :
	virtual const JPTypeName& getName() const
	{
		return m_Type;
	}
	
	virtual const JPTypeName& getObjectType() const
	{
		return m_ObjectTypeName;
	}

	virtual bool      isObjectType() const
	{ 
		return true; 
	}

	virtual HostRef*  getStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, JPTypeName& tgtType);
	virtual void      setStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, JPTypeName& tgtType);
	virtual void      setInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObjectFromObject(jvalue val);

	virtual jobject convertToJavaObject(HostRef* obj);

	virtual HostRef*  invokeStatic(JPJavaFrame& frame, jclass, jmethodID, jvalue*);
	virtual HostRef*  invoke(JPJavaFrame& frame, jobject, jclass clazz, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(JPJavaFrame& frame, int size);
	virtual vector<HostRef*> getArrayRange(JPJavaFrame& frame, jarray, int start, int length);
	virtual void      setArrayRange(JPJavaFrame& frame, jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef*  getArrayItem(JPJavaFrame& frame, jarray, int ndx);
	virtual void      setArrayItem(JPJavaFrame& frame, jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(JPJavaFrame& frame, jarray, int start, int length)
	{
		RAISE(JPypeException, "not impled for void*");
	}
	
	virtual void setArrayRange(JPJavaFrame& frame, jarray, int start, int len, PyObject*) {
		RAISE(JPypeException, "not impled for void*");
	}

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubTypeOf(const JPType& other) const;

protected :
	virtual jclass    getClass() const = 0;
	
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
	virtual jclass    getClass() const;

public : // JPType implementation	
	virtual HostRef*  asHostObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
};

#endif // _JPPOBJECTTYPE_H_
