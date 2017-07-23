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
#ifndef _JPPRIMITIVETYPE_H_
#define _JPPRIMITIVETYPE_H_

class JPPrimitiveType : public JPType
{
protected :
	JPPrimitiveType(JPTypeName::ETypes type, const string& boxedType, const string& primitiveType);
	virtual ~JPPrimitiveType();
	
private :
	JPTypeName m_Type;
	string     m_PrimitiveName;
	// These will be freed by the typemanager
	JPClass    *m_BoxedClass;
	JPClass    *m_PrimitiveClass;

public :

	virtual JPTypeName::ETypes getType() const;

	virtual bool isObjectType() const
	{ 
		return false; 
	}
	
	virtual const JPTypeName& getName() const;

	virtual const JPTypeName& getObjectType() const;

	virtual JPClass* getBoxedClass() const 
	{
		return m_BoxedClass;
	}

	virtual const string& getPrimitiveName() const
	{
		return m_PrimitiveName;
	}

	virtual JPClass* getPrimitiveClass() const
	{
		return m_PrimitiveClass;
	}

	virtual jobject	   convertToJavaObject(HostRef* obj);

	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length) = 0;

	virtual void setArrayRange(jarray, int, int, PyObject*) = 0;
};

class JPVoidType : public JPPrimitiveType
{
public :
	JPVoidType();
	virtual ~JPVoidType();
	
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
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length)
	{
		RAISE(JPypeException, "not impled for void*");
	}
	virtual void      setArrayRange(jarray, int start, int length, PyObject* seq)
	{
		RAISE(JPypeException, "not impled for void*");
	}

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubTypeOf(const JPType& other) const
	{
		return other.getName().getType() == JPTypeName::_void;
	}
};

class JPByteType : public JPPrimitiveType
{
public :
	JPByteType();
	virtual ~JPByteType();

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
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	// this returns tuple instead of list, for performance reasons
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);


	virtual HostRef*   convertToDirectBuffer(HostRef* src);

	virtual bool isSubTypeOf(const JPType& other) const
	{
		JPTypeName::ETypes otherType = other.getName().getType();
		return otherType == JPTypeName::_byte
				|| otherType == JPTypeName::_short
				|| otherType == JPTypeName::_int
				|| otherType == JPTypeName::_long
				|| otherType == JPTypeName::_float
				|| otherType == JPTypeName::_double;
	}

};

class JPShortType : public JPPrimitiveType
{
public :
	JPShortType();
	virtual ~JPShortType();

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
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubTypeOf(const JPType& other) const
	{
		JPTypeName::ETypes otherType = other.getName().getType();
		return otherType == JPTypeName::_short
				|| otherType == JPTypeName::_int
				|| otherType == JPTypeName::_long
				|| otherType == JPTypeName::_float
				|| otherType == JPTypeName::_double;
	}

};

class JPIntType : public JPPrimitiveType
{
public :
	JPIntType();
	virtual ~JPIntType();

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
	virtual void      setArrayRange(jarray, int, int, PyObject*);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);

	virtual bool isSubTypeOf(const JPType& other) const
	{
		JPTypeName::ETypes otherType = other.getName().getType();
		return otherType == JPTypeName::_int
				|| otherType == JPTypeName::_long
				|| otherType == JPTypeName::_float
				|| otherType == JPTypeName::_double;
	}
};

class JPLongType : public JPPrimitiveType
{
public :
	JPLongType();
	virtual ~JPLongType();

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
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubTypeOf(const JPType& other) const
	{
		JPTypeName::ETypes otherType = other.getName().getType();
		return otherType == JPTypeName::_long
				|| otherType == JPTypeName::_float
				|| otherType == JPTypeName::_double;
	}
};

class JPFloatType : public JPPrimitiveType
{
public :
	JPFloatType();
	virtual ~JPFloatType();

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
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubTypeOf(const JPType& other) const
	{
		JPTypeName::ETypes otherType = other.getName().getType();
		return otherType == JPTypeName::_float
				|| otherType == JPTypeName::_double;
	}
};

class JPDoubleType : public JPPrimitiveType
{
public :
	JPDoubleType();
	virtual ~JPDoubleType();

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
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);

	virtual bool isSubTypeOf(const JPType& other) const
	{
		JPTypeName::ETypes otherType = other.getName().getType();
		return otherType == JPTypeName::_double;
	}

};

class JPCharType : public JPPrimitiveType
{
public :
	JPCharType();
	virtual ~JPCharType();

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
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef*  getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubTypeOf(const JPType& other) const
	{
		JPTypeName::ETypes otherType = other.getName().getType();
		return otherType == JPTypeName::_char
				|| otherType == JPTypeName::_int
				|| otherType == JPTypeName::_long
				|| otherType == JPTypeName::_float
				|| otherType == JPTypeName::_double;
	}

};

class JPBooleanType : public JPPrimitiveType
{
public :
	JPBooleanType();
	virtual ~JPBooleanType();

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
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubTypeOf(const JPType& other) const
	{
		JPTypeName::ETypes otherType = other.getName().getType();
		return otherType == JPTypeName::_boolean;
	}
};

#endif // _JPPRIMITIVETYPE_H_

