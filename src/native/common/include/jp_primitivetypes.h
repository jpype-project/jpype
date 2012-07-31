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
#ifndef _JPPRIMITIVETYPE_H_
#define _JPPRIMITIVETYPE_H_

class JPPrimitiveType : public JPType
{
protected :
	JPPrimitiveType(JPTypeName::ETypes type, bool isObject, JPTypeName objectType)
	{
		m_Type = JPTypeName::fromType(type);
		m_IsObject = isObject;
		m_ObjectTypeName = objectType;
	}
	
	virtual ~JPPrimitiveType()
	{
	}
	
private :
	JPTypeName m_Type;
	bool       m_IsObject;
	JPTypeName m_ObjectTypeName;

public :
	virtual bool       isObjectType() 
	{ 
		return m_IsObject; 
	}
	
	virtual JPTypeName getName()
	{
		return m_Type;
	}
	
	virtual JPTypeName getObjectType()
	{
		return m_ObjectTypeName;
	}

	virtual jobject	   convertToJavaObject(HostRef* obj);
};

class JPVoidType : public JPPrimitiveType
{
public :
	JPVoidType() : JPPrimitiveType(JPTypeName::_void, false, JPTypeName::fromSimple("java.lang.Void"))
	{
	}
	
	virtual ~JPVoidType()
	{
	}
	
public : // JPType implementation
	virtual HostRef*  getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setStaticValue(jclass c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);	
	virtual HostRef*  invokeStatic(jclass, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, jclass, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual void       setArrayValues(jarray, HostRef*);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
};

class JPByteType : public JPPrimitiveType
{
public :
	JPByteType() : JPPrimitiveType(JPTypeName::_byte, false, JPTypeName::fromSimple("java.lang.Byte"))
	{
	}
	
	virtual ~JPByteType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setStaticValue(jclass c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(jclass, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, jclass, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual void       setArrayValues(jarray, HostRef*);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
};

class JPShortType : public JPPrimitiveType
{
public :
	JPShortType() : JPPrimitiveType(JPTypeName::_short, false, JPTypeName::fromSimple("java.lang.Short"))
	{
	}
	
	virtual ~JPShortType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setStaticValue(jclass c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(jclass, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, jclass, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual void       setArrayValues(jarray, HostRef*);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
};

class JPIntType : public JPPrimitiveType
{
public :
	JPIntType(): JPPrimitiveType(JPTypeName::_int, false, JPTypeName::fromSimple("java.lang.Integer"))
	{
	}
	
	virtual ~JPIntType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setStaticValue(jclass c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(jclass, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, jclass, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual void       setArrayValues(jarray, HostRef*);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	
};

class JPLongType : public JPPrimitiveType
{
public :
	JPLongType() : JPPrimitiveType(JPTypeName::_long, false, JPTypeName::fromSimple("java.lang.Long"))
	{
	}
	
	virtual ~JPLongType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setStaticValue(jclass c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(jclass, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, jclass, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual void       setArrayValues(jarray, HostRef*);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
};

class JPFloatType : public JPPrimitiveType
{
public :
	JPFloatType() : JPPrimitiveType(JPTypeName::_float, false, JPTypeName::fromSimple("java.lang.Float"))
	{
	}
	
	virtual ~JPFloatType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setStaticValue(jclass c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(jclass, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, jclass, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual void       setArrayValues(jarray, HostRef*);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);
};

class JPDoubleType : public JPPrimitiveType
{
public :
	JPDoubleType() : JPPrimitiveType(JPTypeName::_double, false, JPTypeName::fromSimple("java.lang.Double"))
	{
	}
	
	virtual ~JPDoubleType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setStaticValue(jclass c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(jclass, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, jclass, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual void       setArrayValues(jarray, HostRef*);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);
};

class JPCharType : public JPPrimitiveType
{
public :
	JPCharType() : JPPrimitiveType(JPTypeName::_char, false, JPTypeName::fromSimple("java.lang.Character"))
	{
	}
	
	virtual ~JPCharType()
	{
	}

public : // JPType implementation
	virtual HostRef*   getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setStaticValue(jclass c, jfieldID fid, HostRef* val);
	virtual HostRef*   getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*   asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*   invokeStatic(jclass, jmethodID, jvalue*);
	virtual HostRef*   invoke(jobject, jclass, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*>  getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef*  getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual void       setArrayValues(jarray, HostRef*);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);
};

class JPBooleanType : public JPPrimitiveType
{
public :
	JPBooleanType() : JPPrimitiveType(JPTypeName::_boolean, false, JPTypeName::fromSimple("java.lang.Boolean"))
	{
	}
	
	virtual ~JPBooleanType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(jclass c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setStaticValue(jclass c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid, JPTypeName& tgtType);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(jclass, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, jclass, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual void       setArrayValues(jarray, HostRef*);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);
};

#endif // _JPPRIMITIVETYPE_H_

