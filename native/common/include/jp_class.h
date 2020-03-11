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

#include "jp_modifier.h"

class JPClass : public JPResource
{
public:
	// Special entry point for JVM independent entities
	JPClass(const string& name, jint modifiers);
	JPClass(JPJavaFrame& context,
			jclass clss,
			const string& name,
			JPClass* super,
			const JPClassList& interfaces,
			jint modifiers);
	virtual ~JPClass();

	void setHost(PyObject* host)
	{
		m_Host = JPPyObject(JPPyRef::_use, host);
	}

	PyObject* getHost()
	{
		return m_Host.get();
	}

	void setHints(PyObject* host)
	{
		m_Hints = JPPyObject(JPPyRef::_use, host);
	}

	PyObject* getHints()
	{
		return m_Hints.get();
	}

public:
	void ensureMembers(JPJavaFrame& frame);

	jclass getJavaClass() const
	{
		jclass cls = m_Class.get();
		if (cls == 0)
			JP_RAISE(PyExc_RuntimeError, "Class is null");
		return cls;
	}

	void assignMembers(JPMethodDispatch* ctor,
			JPMethodDispatchList& methods,
			JPFieldList& fields);

	string toString() const;

	string getCanonicalName() const
	{
		return m_CanonicalName;
	}

	string getName() const;

	bool isAbstract() const
	{
		return JPModifier::isAbstract(m_Modifiers);
	}

	bool isFinal() const
	{
		return JPModifier::isFinal(m_Modifiers);
	}

	bool isThrowable() const
	{
		return JPModifier::isThrowable(m_Modifiers);
	}

	bool isInterface() const
	{
		return JPModifier::isInterface(m_Modifiers);
	}

	virtual bool isPrimitive() const
	{
		return false;
	}

	JPMethodDispatch* getCtor()
	{
		return m_Constructors;
	}

	const JPMethodDispatchList& getMethods()
	{
		return m_Methods;
	}

	const JPFieldList&  getFields()
	{
		return m_Fields;
	}

	/**
	 * Determine if a Python object will convert to this java type.
	 *
	 * This is used to determine which overload is the best match.
	 *
	 * @param pyobj is the Python object.
	 * @return the quality of the match
	 */
	virtual JPMatch::Type getJavaConversion(JPJavaFrame* frame, JPMatch& match, PyObject* pyobj);

	/** Create a new Python object to wrap a Java value.
	 *
	 * @return a new Python object.
	 */
	virtual JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue val);

	/**
	 * Get the Java value representing as an object.
	 *
	 * This will unbox if the type is a primitive.
	 *
	 * @return a java value with class.
	 */
	virtual JPValue getValueFromObject(const JPValue& obj);

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
	virtual void        setArrayRange(JPJavaFrame& frame, jarray,
			jsize start, jsize length, jsize step, PyObject* vals);
	virtual JPPyObject  getArrayItem(JPJavaFrame& frame, jarray, jsize ndx);
	virtual void        setArrayItem(JPJavaFrame& frame, jarray, jsize ndx, PyObject* val);

	/**
	 * Expose IsAssignableFrom to python.
	 */
	virtual bool isAssignableFrom(JPJavaFrame& frame, JPClass* o);

	// Object properties

	JPClass* getSuperClass()
	{
		return m_SuperClass;
	}

	virtual JPValue newInstance(JPJavaFrame& frame, JPPyObjectVector& args);

	const JPClassList& getInterfaces()
	{
		return m_Interfaces;
	}

	JPContext* getContext() const
	{
		if (m_Context == 0)
			JP_RAISE(PyExc_RuntimeError, "Null context");
		return
		m_Context;
	}

protected:
	JPContext*           m_Context;
	JPClassRef           m_Class;
	JPClass*             m_SuperClass;
	JPClassList          m_Interfaces;
	JPMethodDispatch*    m_Constructors;
	JPMethodDispatchList m_Methods;
	JPFieldList          m_Fields;
	string               m_CanonicalName;
	jint                 m_Modifiers;
	JPPyObject           m_Host;
	JPPyObject           m_Hints;
} ;

#endif // _JPPOBJECTTYPE_H_
