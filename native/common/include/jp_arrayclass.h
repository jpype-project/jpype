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
#ifndef _JPARRAYCLASS_H_
#define _JPARRAYCLASS_H_

/**
 * Class to wrap Java Class and provide low-level behavior.
 *
 * Forked into concrete subclasses (JPArrayClassBoolean...JPArrayClassDouble)
 * mirroring the JPClass/JPPrimitiveType fork, so that which of the fixed
 * set of array conversions (null/object/char/byte/buffer/multiArrayBuffer/
 * ragged/sequence/hints) can ever match a given array class is decided by
 * which concrete C++ type it is -- resolved once at construction via
 * componentType->createArrayClass() (see JPClass::createArrayClass) -- not
 * re-derived on every findJavaConversionImpl() call the way a single
 * shared class with a generic runtime-filtered list would have to. This
 * base class is still directly instantiated (constructors are public, not
 * protected, unlike the JPArray fork): it covers both a plain
 * class/interface component (String[], Foo[]) and a component that is
 * itself an array class (int[][], ...) -- neither needs a char/byte/buffer
 * specialization, and the nested-array multiArrayBuffer/ragged eligibility
 * already depends only on depth/leaf fields computed here, not on which
 * concrete leaf-primitive subclass this is.
 */
class JPArrayClass : public JPClass
{
public:
	JPArrayClass(JPJavaFrame& frame,
			jclass cls,
			const string& name,
			JPClass* superClass,
			JPClass* componentType,
			jint modifiers);
	~ JPArrayClass() override;

	JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast) override;
	JPMatch::Type findJavaConversionImpl(JPMatch &match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;

	// Reaching this override means the *component type of the array being
	// constructed* is itself an array class (this is componentType's own
	// virtual, called as componentType->createArrayWrapper(value) -- see
	// JPClass::createArrayWrapper) -- so the wrapper needed is JPArrayNested.
	JPArray* createArrayWrapper(const JPValue& value) override;

	// Reaching this override means the *component type of the array being
	// constructed* is itself an array class -- i.e. this builds the
	// JPArrayClass for a nested array (int[][], Object[][], ...). Which
	// further specialization the *new* (one level deeper) array class
	// needs is decided here, once, from fields already fixed on `this`
	// (the inner array): m_MultiArrayLeaf/its type code, never
	// recomputed or re-checked once the concrete class is chosen -- see
	// JPArrayClassNested/JPArrayClassNestedRagged below.
	JPArrayClass* createArrayClass(JPJavaFrame& frame, jclass cls,
			const string& name, JPClass* superClass, jint modifiers) override;

	JPValue newArray(JPJavaFrame& frame, int length);

	/**
	 * Create a new java array containing a set of items take from
	 * a range.
	 *
	 * This is used to support variable arguments.
	 *
	 * @param refs contains a vector of python objects.
	 * @param start is the start of the range inclusive.
	 * @param end is the end of the range exclusive.
	 * @return a jvalue containing a java vector.
	 */
	jvalue convertToJavaVector(JPJavaFrame& frame, JPPyObjectVector& refs, jsize start, jsize end);

	virtual JPClass* getComponentType()
	{
		return m_ComponentType;
	}

	bool isArray() const override
	{
		return true;
	}

	// Nesting depth (1 for e.g. int[], 2 for int[][], ...) and primitive
	// leaf type of this array's component chain, computed once at
	// construction (component classes are always fully built already, so
	// this is O(1) here rather than a dynamic_cast walk on every
	// conversion attempt). getMultiArrayLeaf() is nullptr for arrays that
	// don't bottom out in a single primitive type (e.g. Object[][]) -- see
	// JPConversionMultiArrayBuffer in jp_classhints.cpp, the only user.
	JPPrimitiveType* getMultiArrayLeaf() const
	{
		return m_MultiArrayLeaf;
	}

	int getMultiArrayDepth() const
	{
		return m_MultiArrayDepth;
	}

protected:
	// Accessible to concrete subclasses' constructors, each of which just
	// forwards to this one.
	JPClass* m_ComponentType;
	JPPrimitiveType* m_MultiArrayLeaf;
	int m_MultiArrayDepth;
} ;

// One subclass per primitive component type. Each overrides
// findJavaConversionImpl with the exact, literal chain of conversions that
// can apply to arrays of that element type -- e.g. JPArrayClassChar is the
// only one that ever tries charArrayConversion -- instead of a shared,
// data-driven list. Definitions live colocated in each type's own .cpp
// (jp_booleantype.cpp, jp_bytetype.cpp, ...) alongside that type's
// createArrayClass()/createArrayWrapper() overrides, per this codebase's
// existing per-type-file convention (see e.g. JPIntType::findJavaConversionImpl
// in jp_inttype.cpp for the same duplication-over-templates pattern already
// used for non-array conversions).
class JPArrayClassBoolean : public JPArrayClass
{
public:
	using JPArrayClass::JPArrayClass;
	JPMatch::Type findJavaConversionImpl(JPMatch &match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;
} ;

class JPArrayClassByte : public JPArrayClass
{
public:
	using JPArrayClass::JPArrayClass;
	JPMatch::Type findJavaConversionImpl(JPMatch &match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;
} ;

class JPArrayClassChar : public JPArrayClass
{
public:
	using JPArrayClass::JPArrayClass;
	JPMatch::Type findJavaConversionImpl(JPMatch &match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;
} ;

class JPArrayClassShort : public JPArrayClass
{
public:
	using JPArrayClass::JPArrayClass;
	JPMatch::Type findJavaConversionImpl(JPMatch &match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;
} ;

class JPArrayClassInt : public JPArrayClass
{
public:
	using JPArrayClass::JPArrayClass;
	JPMatch::Type findJavaConversionImpl(JPMatch &match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;
} ;

class JPArrayClassLong : public JPArrayClass
{
public:
	using JPArrayClass::JPArrayClass;
	JPMatch::Type findJavaConversionImpl(JPMatch &match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;
} ;

class JPArrayClassFloat : public JPArrayClass
{
public:
	using JPArrayClass::JPArrayClass;
	JPMatch::Type findJavaConversionImpl(JPMatch &match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;
} ;

class JPArrayClassDouble : public JPArrayClass
{
public:
	using JPArrayClass::JPArrayClass;
	JPMatch::Type findJavaConversionImpl(JPMatch &match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;
} ;

/** A nested array (component type is itself an array class) whose leaf is
 * not eligible for the ragged-native conversion -- currently unreachable
 * in practice, since isRaggedEligible (jp_classhints.cpp) now covers
 * every primitive type code, but kept as the selectable alternative to
 * JPArrayClassNestedRagged below (selected via
 * JPArrayClass::createArrayClass, not a runtime check in the chain) for
 * any future leaf type that isn't ragged-eligible. Adds
 * multiArrayBufferConversion; never tries raggedSequenceConversion.
 */
class JPArrayClassNested : public JPArrayClass
{
public:
	using JPArrayClass::JPArrayClass;
	JPMatch::Type findJavaConversionImpl(JPMatch &match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;
} ;

/** A nested array that bottoms out in a ragged-eligible primitive leaf
 * (every primitive type code -- Z/B/C/S/I/J/F/D, see isRaggedEligible in
 * jp_classhints.cpp), e.g. int[][], char[][][]. Adds both
 * multiArrayBufferConversion and raggedSequenceConversion,
 * unconditionally -- again decided once at JPArrayClass::createArrayClass,
 * never re-checked per call.
 */
class JPArrayClassNestedRagged : public JPArrayClass
{
public:
	using JPArrayClass::JPArrayClass;
	JPMatch::Type findJavaConversionImpl(JPMatch &match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;
} ;

#endif // _JPARRAYCLASS_H_
