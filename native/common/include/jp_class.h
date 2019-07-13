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
#ifndef _JP_CLASS_H_
#define _JP_CLASS_H_

class JPClass
{
public:
	typedef map<string, JPMethod*> MethodMap;
	typedef vector<JPMethod*> MethodList;
	typedef vector<JPField*> FieldList;
	typedef vector<JPClass*> ClassList;

	JPClass(jclass clss);
	virtual ~JPClass();

public:

	jclass getJavaClass() const
	{
		return m_Class.get();
	}
        
	string toString() const;
	string getCanonicalName() const;
	bool isAbstract();
	bool isFinal();
	bool isThrowable();
	bool isInterface();
	virtual bool isPrimitive() const;
	const MethodList& getMethods();
	const FieldList&  getFields();

	/**
	 * Determine if a Python object will convert to this java type. 
	 * 
	 * This is used to determine which overload is the best match.
	 * 
	 * @param obj is the Python object.
	 * @return the quality of the match
	 */
	virtual JPMatch::Type canConvertToJava(PyObject* obj);

	/**
	 * Execute a conversion from Python to java.
	 * 
	 * This should only be called if canConvertToJava returned
	 * a valid conversion.
	 * 
	 * @param obj is the Python object.
	 * @return a jvalue holding the converted python object.
	 */
	virtual jvalue convertToJava(PyObject* obj);

	/** Create a new Python object to wrap a Java value. 
	 * 
	 * @return a new Python object.
	 */
	virtual JPPyObject convertToPythonObject(jvalue val);

	/**
	 * Get the Java value representing as an object.
	 * 
	 * This will unbox if the type is a primitive.
	 *  
	 * @return a java value with class.
	 */
	virtual JPValue getValueFromObject(jobject obj);

	/** 
	 * Call a static method that returns this type of object. 
	 */
	virtual JPPyObject invokeStatic(JPJavaFrame& frame, jclass, jmethodID, jvalue*);

	/** 
	 * Call a method that returns this type of object. 
	 */
	virtual JPPyObject invoke(JPJavaFrame& frame, jobject, jclass clazz, jmethodID, jvalue*);

	/**
	 * Get a static field that returns this type.
	 * 
	 * @param frame is the frame to hold the local reference.
	 * @param cls is the class holding the static field.
	 * @param fid is the field id.
	 * @return 
	 */
	virtual JPPyObject  getStaticField(JPJavaFrame& frame, jclass cls, jfieldID fid);
	virtual void        setStaticField(JPJavaFrame& frame, jclass cls, jfieldID fid, PyObject* val);

	virtual JPPyObject  getField(JPJavaFrame& frame, jobject obj, jfieldID fid);
	virtual void        setField(JPJavaFrame& frame, jobject obj, jfieldID fid, PyObject* val);

	virtual jarray      newArrayInstance(JPJavaFrame& frame, jsize size);
	virtual JPPyObject  getArrayRange(JPJavaFrame& frame, jarray, jsize start, jsize length);
	virtual void        setArrayRange(JPJavaFrame& frame, jarray, jsize start, jsize length, PyObject* vals);
	virtual JPPyObject  getArrayItem(JPJavaFrame& frame, jarray, jsize ndx);
	virtual void        setArrayItem(JPJavaFrame& frame, jarray, jsize ndx, PyObject* val);

	/** Determine if this class is a super or implements another class.
	 * 
	 * This is used specifically in the method overload to determine 
	 * if a method will cover another.  For objects this is the same as
	 * IsAssignableFrom.  For primitive type, then this will be true
	 * if this primitive can be converted to other without a cast.
	 * 
	 * In the sense of
	 *  http://docs.oracle.com/javase/specs/jls/se7/html/jls-4.html#jls-4.10
	 * 
	 * @param other is the class to to assign to.
	 * @return true if this class is the same, a super class, or implements
	 * the other class.
	 */
	virtual bool isSubTypeOf(JPClass* other) const;

	/**
	 * Expose IsAssignableFrom to python. 
	 * 
	 * FIXME this may be able to be replaced with isSubTypeOf.
	 * They are doing the same thing. 
	 */
	bool isAssignableFrom(JPClass* o);

	// Object properties
	JPClass* getSuperClass();
	virtual JPValue newInstance(JPPyObjectVector& args);
	const ClassList& getInterfaces();

	string describe();

	// Check if a value is an instance of this class
	bool isInstance(JPValue& val);
	
	virtual void postLoad();
private:
	void loadFields();
	void loadMethods();
	void loadConstructors();

protected:
	JPClassRef   m_Class;
	JPClass*     m_SuperClass;
	ClassList    m_SuperInterfaces;
	FieldList    m_Fields;
	MethodList   m_Methods;
	JPMethod*    m_Constructors;
	string       m_CanonicalName;
	bool         m_InterfacesLoaded;
	bool         m_IsInterface;
	bool         m_IsThrowable;
	bool         m_IsAbstract;
	bool         m_IsFinal;

} ;

#endif // _JPPOBJECTTYPE_H_
