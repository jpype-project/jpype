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
#ifndef JP_PYTHONTYPES_H_
#define JP_PYTHONTYPES_H_
#include <Python.h>
#include <vector>

/**
 * This set of source are mostly sugar with some light weight
 * reference management.  Each type holds a reference for the duration of its
 * scope.  If the PyObject must survive the reference container lifespan
 * then a keep must be called.  keep() should only appear when returning from
 * the python module.  Any earlier than the return provides an opportunity for
 * an exception to be throw which may leak.
 *
 * For this module to function properly, each call needs to be check against the
 * python object model to verify.
 * 1) Does a call return an object as a new reference or a borrowed one.
 * 2) Does a call set an error that must be checked.
 * 3) Does the call steal a reference to an object passed into it.
 *
 * Calls should always accept a raw PyObject* and return JPPyObject if
 * the return can be an new object or PyObject* if the return is a borrowed
 * reference.  Obviously holding a borrowed reference can create issues as
 * nothing prevents a borrowed object from falling out of the list and
 * being deleted.
 *
 */

//  Note: Python uses a sized size type.  Thus we will map it to jlong.
//  Note: for conversions we will use jint and jlong types so that we map directly to java.
//  Note: Where possible we will use std::string in place of const char*

#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

/** Reference to a Python object.
 *
 * This creates a reference on creation and deletes it on destruction.
 *
 * Because there is a cost associated with creating wrappers, most
 * methods should be static if they don't actually require management.
 *
 * Methods will return either a bare PyObject* if they hold the
 * object like a container, or a wrapper which will control the lifespan
 * of the object.
 *
 * Python has a lot of difference cases for how it returns an object.
 * - It can give a new object or set an error.
 * - It can return NULL indicating there is no object.
 * - It can give a borrowed object or NULL if it doesn't exist.
 * - Or we can just have an existing object we want to use.
 *
 * With all these different methods, we need to have a policy that
 * state how we want this object reference to be treated.  Each
 * policy will produce different actions on the creation of
 * a reference wrapper.
 *
 */
class JPPyObject
{
	/** Create a new reference to a Python object.
	 *
	 * @param obj is the python object.
	 */
	explicit JPPyObject(PyObject* obj);

public:

	/**
	 * This policy is used if we need to hold a reference to an existing
	 * object for some duration.  The object may be null.
	 *
	 * Increment reference count if not null, and decrement when done.
	 */
	static JPPyObject use(PyObject* obj);

	/**
	 * This policy is used when we are given a new reference that we must
	 * destroy.  This will steal a reference.
	 *
	 * claim reference, and decremented when done. Clears errors if NULL.
	 */
	static JPPyObject accept(PyObject* obj);

	/**
	 * This policy is used when we are given a new reference that we must
	 * destroy.  This will steal a reference.
	 *
	 * Assert not null, claim reference, and decremented when done.
	 * Will throw an exception in the object is null.
	 */
	static JPPyObject claim(PyObject* obj);

	/**
	 * This policy is used when we are capturing an object returned from a python
	 * call that we are responsible for.  This will steal a reference.
	 *
	 * Check for errors, assert not null, then claim.
	 * Will throw an exception an error occurs.
	 */
	static JPPyObject call(PyObject* obj);

	JPPyObject() : m_PyObject(nullptr)
	{
	}

	JPPyObject(const JPPyObject &self);

	~JPPyObject();

	JPPyObject& operator=(const JPPyObject& o);

	/**
	 * Keep an object by creating a reference.
	 *
	 * This should only appear in the return statement in the cpython module.
	 * The reference must not be null.  Keep invalidates this handle from any
	 * further use as you were supposed to have called return.
	 *
	 * @return the pointer to the Python object.
	 */
	PyObject* keep();

	/** Used in special case of exception handling. */
	PyObject* keepNull()
	{
		PyObject *out = m_PyObject;
		m_PyObject = nullptr;
		return out;
	}

	/** Access the object.  This should never appear in
	 * a return statement.
	 */
	PyObject* get()
	{
		return m_PyObject;
	}

	/** Determine if this python reference is null.
	 *
	 * @returns true if null.
	 */
	bool isNull() const
	{
		return m_PyObject == nullptr;
	}

	/**
	 * Get a reference to Python None.
	 */
	static JPPyObject getNone();

	void incref();
	void decref();

protected:
	PyObject* m_PyObject{nullptr};
} ;

/****************************************************************************
 * String
 ***************************************************************************/

/** Wrapper for the concept of a Python string.
 *
 * For the purposes of this interface we will hide the differences
 * between unicode and string.
 */
class JPPyString : public JPPyObject
{
public:


	/** Check if the object is a bytes or unicode.
	 *
	 * @returns true if the object is bytes or unicode.
	 */
	static bool check(PyObject* obj);

	/** Create a new string from utf8 encoded string.
	 * Note: java utf8 is not utf8.
	 *
	 * Python2 produced str unless unicode is set to
	 * true.  Python3 will always produce a unicode string.
	 *
	 * @param str is the string to convert
	 */
	static JPPyObject fromStringUTF8(const string& str);

	/** Get a UTF-8 encoded string from Python
	 */
	static string asStringUTF8(PyObject* obj);

	static JPPyObject fromCharUTF16(jchar c);
	static bool checkCharUTF16(PyObject* obj);
	static jchar asCharUTF16(PyObject* obj);

} ;

/****************************************************************************
 * Container types
 ***************************************************************************/

/** Wrapper for a Python sequence.
 *
 * In most cases, we will not use this directly, but rather convert to
 * a JPPyObjectVector for easy access.
 */
class JPPySequence
{
	JPPyObject m_Sequence;

	explicit JPPySequence(PyObject* obj)
	{
		m_Sequence = JPPyObject::use(obj);
	}

public:

	/** Needed for named constructor.
	 */
	JPPySequence(const JPPySequence& seq) = default;

	/** Use an existing Python sequence in C++.
	 */
	static JPPySequence use(PyObject* obj)
	{
		return JPPySequence(obj);
	}

	JPPyObject operator[](jlong i);

	jlong size();

	JPPySequence& operator= (const JPPySequence&) = delete;

} ;

/** For purposes of efficiency, we should only convert a sequence once per
 * method call.  This class is to support that operation.
 *
 * THis object is read only.
 */
class JPPyObjectVector
{
public:

	/** Use an existing sequence members as a vector.
	 */
	explicit JPPyObjectVector(PyObject* sequence);

	/** Use an existing sequence members as a vector plus the
	 * object instance.
	 */
	JPPyObjectVector(PyObject* inst, PyObject* sequence);

	size_t size() const
	{
		return m_Contents.size();
	}

	PyObject* operator[](Py_ssize_t i)
	{
		return m_Contents[i].get();
	}

	JPPyObject& getInstance()
	{
		return m_Instance;
	}

    // disallow copying
	JPPyObjectVector& operator= (const JPPyObjectVector& ) = delete;
	JPPyObjectVector(const JPPyObjectVector& ) = delete;

private:
	JPPyObject m_Instance;
	JPPyObject m_Sequence;
	vector<JPPyObject> m_Contents;
} ;

/****************************************************************************
 * Error handling
 ***************************************************************************/

/** Front end for all Python exception handling.
 *
 * To issue an error from within the C++ layer use the appropriate
 * JP_RAISE_* macro.  If within the Python extension module use
 * the Python interface.
 *
 */
namespace JPPyErr
{
bool fetch(JPPyObject& exceptionClass, JPPyObject& exceptionValue, JPPyObject& exceptionTrace);
void restore(JPPyObject& exceptionClass, JPPyObject& exceptionValue, JPPyObject& exceptionTrace);
}

/** Memory management for handling a python exception currently in progress. */
class JPPyErrFrame
{
public:
	JPPyObject m_ExceptionClass;
	JPPyObject m_ExceptionValue;
	JPPyObject m_ExceptionTrace;
	bool good;

	JPPyErrFrame();
	~JPPyErrFrame();
	void clear();
	void normalize();
} ;

/** Used to establish a python lock when called from a
 * thread external to python such a java.
 */
class JPPyCallAcquire
{
public:
	/** Acquire the lock. */
	JPPyCallAcquire();
	/* Release the lock. */
	~JPPyCallAcquire();
private:
	long m_State;
} ;

/** Used when leaving python to an external potentially
 * blocking call
 */
class JPPyCallRelease
{
public:
	/** Release the lock. */
	JPPyCallRelease();
	/** Reacquire the lock. */
	~JPPyCallRelease();
private:
    PyThreadState* m_State1;
} ;

class JPPyBuffer
{
public:
	/**
	 * Attempt to create a buffer view.
	 *
	 * If this fails then valid will return false and
	 * PyExc_BufferError will be set.  Clear the exception if
	 * the alternative methods are used.
	 *
	 * @param obj
	 * @param flags
	 */
	JPPyBuffer(PyObject* obj, int flags);
	~JPPyBuffer();

	char *getBufferPtr(std::vector<Py_ssize_t>& indices) const;

	Py_buffer& getView()
	{
		return m_View;
	}

	bool valid() const
	{
		return m_Valid;
	}

private:
	Py_buffer m_View{};
	bool m_Valid;
} ;

#endif
