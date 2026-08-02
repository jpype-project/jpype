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

#include "jp_conversioncache.h"
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
	~JPClass() override;

	void setHost(PyObject* host);

	PyTypeObject* getHost()
	{
		return (PyTypeObject*) m_Host.get();
	}

	void setHints(PyObject* host);

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
	 * Non-virtual: consults a per-type cache keyed on Py_TYPE(match.object)
	 * before delegating to the virtual findJavaConversionImpl, and memoizes
	 * the result there if the resolution turned out to be cacheable (see
	 * JPMatch::cacheable). Subclasses override findJavaConversionImpl, not
	 * this.
	 *
	 * @param pyobj is the Python object.
	 * @return the quality of the match
	 */
	JPMatch::Type findJavaConversion(JPMatch& match);

	/**
	 * Cheap, type-only check for whether a single Python object matches
	 * this class at some known quality, without going through the general
	 * findJavaConversion dispatch (JPMatch construction, cache lookup,
	 * matches() chain).
	 *
	 * Used by JPConversionSequence (jp_classhints.cpp) to fast-path the
	 * common case of a homogeneous list of one recognized element type
	 * (e.g. plain `int` for int[]): it tries this first, per element, and
	 * only falls back to the general path (unchanged, fully correct) from
	 * the point where an element doesn't satisfy it -- so a mixed list
	 * only ever pays full price for its non-conforming tail, not the whole
	 * list, and a fully homogeneous list never constructs a JPMatch at
	 * all. Returns false (no opinion, always safe) by default; only worth
	 * overriding where a raw C-API type check can stand in for a known
	 * quality level.
	 */
	virtual bool fastElementCheck(PyObject* obj, JPMatch::Type& quality) const
	{
		return false;
	}

	/** Clear this class's cached findJavaConversion() results.
	 *
	 * Called lazily by findJavaConversion() itself when JPClassHints's
	 * global generation counter has moved on since this class's cache was
	 * last populated (i.e. some class's hints changed somewhere).
	 */
	void clearConversionCache();

protected:
	/**
	 * The actual, per-class conversion search (null/object/proxy/hints,
	 * or a hand-written primitive-type chain, depending on the subclass).
	 * Overridden instead of findJavaConversion so the caching wrapper above
	 * stays in one place. See JPBoxedType/JPFunctional for the one pattern
	 * that needs care: reusing another class's chain (including this base
	 * one) as a building block must call findJavaConversionImpl directly
	 * (explicitly qualified) rather than going back through
	 * findJavaConversion, or it recurses into itself through the vtable.
	 */
	virtual JPMatch::Type findJavaConversionImpl(JPMatch& match);

public:

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
	JPPyObject           m_Hints;

private:
	// Every cacheable matches() sets match.closure to exactly `this`, with
	// one known exception: boxBooleanConversion's dedicated direct-match
	// role (see jp_boxedtype.cpp's JPConversionBoxGeneric comment) always
	// uses the fixed java.lang.Boolean class instead, regardless of `this`.
	// That's a compile-time constant tied to *which conversion* was cached,
	// not extra per-entry state, so JPClass::findJavaConversion special-
	// cases it directly rather than growing the cache entry to carry a
	// third field (see JPConversionCache for why that matters: it's sized
	// to fit in one atomically-accessed 128-bit slot).
	JPConversionCache m_ConversionCache;
	uint64_t m_ConversionCacheGeneration = 0;
} ;

#endif // _JPPOBJECTTYPE_H_
