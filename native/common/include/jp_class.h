/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#ifndef _JP_CLASS_H_
#define _JP_CLASS_H_

#include <atomic>
#include "jp_modifier.h"

/** Life cycle of the Python wrapper of a Java class.
 *
 * The wrapper is published before the post load step (customizers, inner
 * classes) has run, so another thread may observe it in between.  Only a
 * completed wrapper may be handed to Python code.  A failed post load is
 * permanent: the half built wrapper must never escape.
 */
enum class JPClassHostState : unsigned char
{
	uninitialized,
	inProgress,     // published by the class factory, post load still running
	awaitingHints,  // bound from Python (class_ setter), hints not assigned yet
	complete,
	failed
};

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
	~JPClass() override;

	/** Publish the host from the class factory.
	 *
	 * The post load step (customizers, inner classes) follows and completes
	 * the host.  Only the first publication wins: returns false if a host
	 * was already published (the host is never replaced).
	 */
	bool setHost(PyObject* host);

	/** Publish a host that is bound from Python (the class_ setter).
	 *
	 * Such a wrapper has no post load step of its own; assigning its hints
	 * finalizes it.  Returns false if a host was already published.
	 */
	bool setBoundHost(PyObject* host);

	PyTypeObject* getHost()
	{
		return (PyTypeObject*) m_HostReady.load(std::memory_order_acquire);
	}

	JPClassHostState getHostState() const
	{
		return m_HostState.load(std::memory_order_acquire);
	}

	// Returns false if hints were already published; they are never replaced.
	bool setHints(PyObject* hints);
	bool hasHints() const;
	void completeHints();

	/** Mark the wrapper as permanently failed.
	 *
	 * Called when the post load step raises.  The host remains published so
	 * the creating thread can still recurse while unwinding, but every
	 * later lookup raises instead of returning the half built class.
	 */
	void failHints();

	/** Check if the Python wrapper has been fully constructed.
	 *
	 * The host is published as soon as the type object exists, but the
	 * post load step (customizers, inner classes) runs afterwards.  On
	 * free-threaded Python another thread may observe the host in between,
	 * so anything handing the host to Python code must wait until this is
	 * true.  Hints are completed as the very last step of PyJPClass_hook
	 * (or by the finalizing class_/_hints setters).
	 */
	bool isHostComplete() const
	{
		return getHostState() == JPClassHostState::complete;
	}

	bool isHostFailed() const
	{
		return getHostState() == JPClassHostState::failed;
	}

	PyObject* getHints();

public:
	void ensureMembers(JPJavaFrame& frame);

	jclass getJavaClass() const;

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

	virtual bool isArray() const
	{
		return false;
	}

	jlong getModifiers()
	{
		return m_Modifiers;
	}

	virtual bool isPrimitive() const
	{
		return false;
	}

	virtual bool isPrimitiveArray() const
	{
		return JPModifier::isPrimitiveArray(m_Modifiers);
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
	virtual JPMatch::Type findJavaConversion(JPMatch& match);

	virtual void getConversionInfo(JPConversionInfo &info);

	/** Create a new Python object to wrap a Java value.
	 *
	 * Some conversion convert to a Python type such as Java boolean and char.
	 * Null pointers match to Python None.  Objects convert automatically to
	 * the most derived type. To disable this behavior the cast option can be
	 * specified.
	 *
	 * @param cast force the wrapper to be the defined type.
	 * @return a new Python object.
	 */
	virtual JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast);

	/**
	 * Get the Java value representing as an object.
	 *
	 * This will unbox if the type is a primitive.
	 *
	 * @return a java value with class.
	 */
	virtual JPValue getValueFromObject(JPJavaFrame& frame, const JPValue& obj);

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

	JPClass*            newArrayType(JPJavaFrame &frame, long d);
	virtual jarray      newArrayOf(JPJavaFrame& frame, jsize size);
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

protected:
	JPClassRef           m_Class;
	JPClass*             m_SuperClass;
	JPClassList          m_Interfaces;
	JPMethodDispatch*    m_Constructors;
	JPMethodDispatchList m_Methods;
	JPFieldList          m_Fields;
	string               m_CanonicalName;
	jint                 m_Modifiers;
	JPPyObject           m_Host;
	std::atomic<PyObject*> m_HostReady{nullptr};
	JPPyObject           m_Hints;
	std::atomic<PyObject*> m_HintsReady{nullptr};
	std::atomic<JPClassHostState> m_HostState{JPClassHostState::uninitialized};
} ;

#endif // _JPPOBJECTTYPE_H_
