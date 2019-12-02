#ifndef JP_PYTHONTYPES_H_
#define JP_PYTHONTYPES_H_
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

/** Python has a lot of difference cases for how it returns an object.
 * - It can give a new object or set an error.
 * - It can return NULL indicating there is no object.
 * - It can give a borrowed object or NULL if it doesn't exist.
 * - Or we can just have an existing object we want to use.
 *
 * With all these different methods, we need to have a policy that
 * state how we want this object reference to be treated.  Each
 * policy will produce different actions on the creation of
 * a reference wrapper.
 */
namespace JPPyRef
{

enum Type
{
	/**
	 * This policy is used if we need to hold a reference to an existing
	 * object for some duration.  The object may be null.
	 *
	 * Increment reference count if not null, and decrement when done.
	 */
	_use = 0,

	/**
	 * This policy is used when we are given a borrowed reference and we
	 * need to check for errors.
	 *
	 * Check for errors, increment reference count, and decrement when done.
	 * Will throw an exception an error occurs.
	 */
	_borrowed = 1,

	/**
	 * This policy is used when we are given a new reference that we must
	 * destroy.  This will steal a reference.
	 *
	 * claim reference, and decremented when done. Clears errors if NULL.
	 */
	_accept = 2,

	/**
	 * This policy is used when we are given a new reference that we must
	 * destroy.  This will steal a reference.
	 *
	 * Assert not null, claim reference, and decremented when done.
	 * Will throw an exception in the object is null.
	 */
	_claim = 6,

	/**
	 * This policy is used when we are capturing an object returned from a python
	 * call that we are responsible for.  This will steal a reference.
	 *
	 * Check for errors, assert not null, then claim.
	 * Will throw an exception an error occurs.
	 */
	_call = 7
} ;
}

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
 */
class JPPyObject
{
public:

	JPPyObject() : pyobj(NULL)
	{
	}

	/** Create a new reference to a Python object.
	 *
	 * @param usage control how this object is to be handled, see JPPyRef.
	 * @param obj is the python object.
	 */
	JPPyObject(JPPyRef::Type usage, PyObject* obj);

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

	/** Access the object.  This should never appear in
	 * a return statement.
	 */
	PyObject* get()
	{
		return pyobj;
	}

	/** Return string value as UTF8.
	 */
	string str();

	static bool hasAttrString(PyObject*, const char* k);
	JPPyObject getAttrString(const char* k);
	static JPPyObject getAttrString(PyObject*, const char* k);
	int setAttrString(const char* k, PyObject* value);

	const char* getTypeName();
	static const char* getTypeName(PyObject* obj);

	/** Execute a call on an object.
	 *
	 * @param args must a tuple and must not be null.
	 * @param kwargs must be a dict and can be null.
	 */
	JPPyObject call(PyObject* args, PyObject* kwargs);

	/** Determine if this python reference is null.
	 *
	 * @returns true if null.
	 */
	bool isNull() const
	{
		return pyobj == NULL;
	}

	/** Determine if this python reference refers to
	 * None.
	 *
	 * @returns true if reference to None, false otherwise.
	 */
	bool isNone()
	{
		return isNone(pyobj);
	}

	/** Determine if this python reference refers to
	 * None.
	 *
	 * @returns true if reference to None, false otherwise.
	 */
	static bool isNone(PyObject* o);

	/** Returns true if pyobj is an instance of type
	 *  or a subclass thereof
	 *
	 * @param is a python object.
	 * @param is a python class or a tuple of types.
	 */
	static bool isInstance(PyObject* pyobj, PyObject* type);

	/** Returns true if pycls is equal to or derived from type.
	 *
	 * @param is a python class.
	 * @param is a python class or a tuple of types.
	 */
	static bool isSubclass(PyObject* pycls, PyObject* type);

	static bool isSequenceOfItems(PyObject* obj);

	/** Get a reference to Python None.
	 */
	static JPPyObject getNone();

protected:
	void incref();
	void decref();

	PyObject* pyobj;
} ;

class JPPyType : public JPPyObject
{
public:
	static bool check(PyObject* obj);
} ;

/****************************************************************************
 * Number types
 ***************************************************************************/

/** Wrapper for a Python Bool object. */
namespace JPPyBool
{
bool check(PyObject* obj);
JPPyObject fromLong(jlong value);
}

/** Wrapper for a Python Int object.
 *
 * Only Python 2 has int objects. For purposes of deciding
 * the wrapper type long and int should be treated identically
 * or we will have operational differences between Python 2 and 3.
 */
namespace JPPyInt
{
bool check(PyObject* obj);
jint asInt(PyObject* obj);

JPPyObject fromInt(jint l);
JPPyObject fromLong(jlong l);
}

/** Wrapper for a Python long object.
 */
namespace JPPyLong
{
bool check(PyObject* obj);
bool checkConvertable(PyObject* obj);

/** Check if this is really an integer type or just can be converted to.
 *
 * PEP-357 says __index__ is defined if we can use it as an array slice.
 * Only integer types can do that.
 */
bool checkIndexable(PyObject* obj);
jlong asLong(PyObject* obj);
JPPyObject fromLong(jlong l);
}

/** Wrapper for a Python float object. */
namespace JPPyFloat
{
bool check(PyObject* obj);
bool checkConvertable(PyObject* obj);
jdouble asDouble(PyObject* obj);
jfloat asFloat(PyObject* obj);

// The scope of the created object is that of the JPPyFloat
JPPyObject fromDouble(jdouble l);
JPPyObject fromFloat(jfloat l);
}

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

	JPPyString(JPPyRef::Type usage, PyObject* obj) : JPPyObject(usage, obj)
	{
	}

	JPPyString(const JPPyObject &self) : JPPyObject(self)
	{
	}

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
	 * @param unicode is true if unicode is allowed.
	 */
	static JPPyObject fromStringUTF8(const string& str, bool unicode = false);

	/** Get a UTF-8 encoded string from Python
	 */
	static string asStringUTF8(PyObject* obj);

	static JPPyObject fromCharUTF16(const jchar c);
	static bool checkCharUTF16(PyObject* obj);
	static jchar asCharUTF16(PyObject* obj);

} ;

namespace JPPyMemoryView
{
bool check(PyObject* obj);
void getByteBufferSize(PyObject* pyobj, char **outBuffer, long& outSize);
}

/****************************************************************************
 * Container types
 ***************************************************************************/

/** Wrapper for a Python tuple object. */
class JPPyTuple : public JPPyObject
{
public:

	JPPyTuple(JPPyRef::Type usage, PyObject* obj) : JPPyObject(usage, obj)
	{
	}

	JPPyTuple(const JPPyTuple &self) : JPPyObject(self)
	{
	}

	/** Create a new tuple holding a fixed number of items.
	 *
	 * Every item must be set before the tuple is used or we are heading
	 * for a segfault.  Tuples are not mutable so items can only be set
	 * during creation.
	 */
	static JPPyTuple newTuple(jlong sz);

	static bool check(PyObject* obj);

	/** Set an item in the tuple.
	 *
	 * This does not steal a reference to the object.
	 */
	void setItem(jlong ndx, PyObject* val);

	/** Fetch an item from a tuple.
	 *
	 * Scope remains with the JPTuple, so no smart reference is required.
	 * @throws if the index is out of the range of the tuple.
	 */
	PyObject* getItem(jlong ndx);

	/** Get the size of the tuple.
	 */
	jlong size();

} ;

/** Wrapper for a Python list object. */
class JPPyList : public JPPyObject
{
public:

	JPPyList(JPPyRef::Type usage, PyObject* obj) : JPPyObject(usage, obj)
	{
	}

	JPPyList(const JPPyObject &self) : JPPyObject(self)
	{
	}

	static JPPyList newList(jlong sz);
	static bool check(PyObject* obj);

	/** Set an item in the list.
	 *
	 * This does not steal a reference to the object.
	 */
	void setItem(jlong ndx, PyObject* val);

	/** Fetch an item from a tuple.
	 *
	 * Scope remains with the JPTuple, so no smart reference is required.
	 * @throws if the index is out of the range of the tuple.
	 */
	PyObject* getItem(jlong ndx);
	jlong size();

} ;

/** Wrapper for a Python sequence.
 *
 * In most cases, we will not use this directly, but rather convert to
 * a JPPyObjectVector for easy access.
 */
class JPPySequence : public JPPyObject
{
public:

	JPPySequence(JPPyRef::Type usage, PyObject* obj) : JPPyObject(usage, obj)
	{
	}

	JPPySequence(const JPPyObject &self) : JPPyObject(self)
	{
	}

	// Note this use to work the same for list, sequence and tuple, but that breaks pypy
	bool check();
	static bool check(PyObject* obj);

	JPPyObject getItem(jlong ndx);

	JPPyObject operator[](jlong i)
	{
		return getItem(i);
	}

	jlong size();
} ;

/** For purposes of efficiency, we should only convert a sequence once per
 * method call.  This class is to support that operation.
 *
 * THis object is read only.
 */
class JPPyObjectVector
{
public:

	JPPyObjectVector(int i);

	/** Use an existing sequence members as a vector.
	 */
	JPPyObjectVector(PyObject* sequence);

	/** Use an existing sequence members as a vector plus the
	 * object instance.
	 */
	JPPyObjectVector(PyObject* inst, PyObject* sequence);

	size_t size() const
	{
		return contents.size();
	}

	PyObject* operator[](size_t i)
	{
		return contents[i].get();
	}

	JPPyObject& getInstance()
	{
		return instance;
	}

private:
	JPPyObjectVector& operator=(const JPPyObjectVector&) ;
	JPPyObjectVector(const JPPyObjectVector&);

private:
	JPPyObject instance;
	JPPySequence seq;
	vector<JPPyObject> contents;
} ;

/** Wrapper for a Python dict.
 *
 * Currently this is not used in this project.  It is being retained
 * so that we can support kwargs at some point in the future.
 */
class JPPyDict : public JPPyObject
{
public:

	JPPyDict(const JPPyObject &self) : JPPyObject(self)
	{
	}

	// Create a new Dict
	static JPPyDict newDict();

	static bool check(PyObject* obj);

	bool contains(PyObject* k);

	JPPyObject getKeys();
	JPPyObject copy(PyObject* obj);

	// item remains with the scope of JPPyDict
	PyObject* getItem(PyObject* k);
	void setItemString(PyObject* o, const char* n);
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
void clear();
/** Check if there is a pending Python exception.
 *
 * @return true if pending, false otherwise.
 */
bool occurred();

bool fetch(JPPyObject& exceptionClass, JPPyObject& exceptionValue, JPPyObject& exceptionTrace);
void restore(JPPyObject& exceptionClass, JPPyObject& exceptionValue, JPPyObject& exceptionTrace);
}

/** Memory management for handling a python exception currently in progress. */
class JPPyErrFrame
{
public:
	JPPyObject exceptionClass;
	JPPyObject exceptionValue;
	JPPyObject exceptionTrace;
	bool good;

	JPPyErrFrame()
	{
		good = JPPyErr::fetch(exceptionClass, exceptionValue, exceptionTrace);
	}

	~JPPyErrFrame()
	{
		if (good)
			JPPyErr::restore(exceptionClass, exceptionValue, exceptionTrace);
	}
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
	void* state1;
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
	void* state1;
} ;


#endif
