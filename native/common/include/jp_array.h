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
#ifndef _JPARRAY_H_
#define _JPARRAY_H_

#include "jp_javaframe.h"

class JPArray;
class JPPrimitiveType;

class JPArrayView
{
public:
	JPArrayView(JPJavaFrame& frame, JPArray* array);
	JPArrayView(JPJavaFrame& frame, JPArray* array, jobject collection);
	~JPArrayView();
	// m_Memory is a raw owning allocation; copying would double-free it.
	// No code path currently copies a JPArrayView, so deleting rather than
	// implementing is the safe choice - implement properly if a real need
	// for copying ever comes up.
	JPArrayView(const JPArrayView&) = delete;
	JPArrayView& operator=(const JPArrayView&) = delete;
	void reference();
	bool unreference(JPJavaFrame& frame);
public:
	JPArray *m_Array;
	void *m_Memory{};
	Py_buffer m_Buffer{};
	int m_RefCount;
	Py_ssize_t m_Shape[5]{};
	Py_ssize_t m_Strides[5]{};
	jboolean m_IsCopy{};
	jboolean m_Owned{};
} ;

/**
 * Class to wrap Java Class and provide low-level behavior
 *
 * Forked into concrete subclasses (JPArrayBoolean...JPArrayDouble,
 * JPArrayObject, JPArrayNested : JPArrayObject) mirroring the existing
 * JPClass/JPPrimitiveType fork. This base class is never instantiated
 * directly (constructors are protected); the concrete subclass a given
 * array gets is resolved once, at construction, via
 * JPArray::create()/JPClass::createArrayWrapper -- not re-derived on every
 * element access the way a single shared class would have to.
 *
 * Only getItem()/slice() -- the hot per-element read path -- are forked so
 * far. setItem/pullTo/pushFrom/toList/clone/JPArrayView stay here on the
 * shared base, unchanged, still resolving component-type facts via
 * m_Class->getComponentType()/dynamic_cast the same way they always have;
 * migrating them onto the same per-subclass static knowledge is a
 * deliberately separate follow-on, not attempted here.
 */
class JPArray
{
	friend class JPArrayView;
protected:
	explicit JPArray(const JPValue& array);
	JPArray(JPArray* cls, jsize start, jsize stop, jsize step);

	/** Wraparound + bounds-check a Python-style index against m_Length,
	 * shared by every concrete subclass's getItem() instead of duplicated
	 * once per subclass. Raises IndexError and does not return on an
	 * out-of-bounds index.
	 */
	jsize checkIndex(jsize ndx) const;

public:
	virtual ~JPArray();

	/** Construct the concrete JPArray subclass matching value's component
	 * type -- resolves via value's array class's component type's own
	 * JPClass::createArrayWrapper virtual, reusing the same per-type fork
	 * that already exists on JPClass rather than a new dispatch table.
	 */
	static JPArray* create(const JPValue& value);

	JPArrayClass* getClass()
	{
		return m_Class;
	}

	jsize     getLength() const;
	void       setRange(JPJavaFrame& frame, jsize start, jsize length, jsize step, PyObject* val);

	/** Get a single element. Overridden per concrete subclass -- see the
	 * class comment above. Signature deliberately carries no frame
	 * parameter, so this stays legal C++ (override signatures must match
	 * exactly) no matter how many concrete subclasses exist or what kind
	 * of frame (if any) each one's implementation actually needs.
	 */
	virtual JPPyObject getItem(jsize ndx) = 0;

	/** Get a single element given an already-resolved reference to the
	 * underlying Java array (real local or global ref -- JNI element
	 * reads don't care which), instead of resolving m_Object fresh.
	 *
	 * Used by PyJPArrayIter's hot loop: rather than pay a
	 * retrieveGlobal()+release JNI round trip on every single element
	 * (see bugs/ArrayIterLocalRefLeak.md), it resolves the array once, as
	 * a genuine NewGlobalRef held for the iterator's own lifetime, and
	 * calls this directly for every element. Not pure virtual -- the
	 * default just falls back to getItem(ndx), so a subclass that has no
	 * per-element resolve cost to amortize (JPArrayObject/JPArrayNested,
	 * which already push a real frame per call for other reasons) simply
	 * doesn't need to override it. The 8 leaf primitive types do.
	 *
	 * ndx is assumed already range-checked by the caller (the iterator
	 * only ever calls this with 0 <= ndx < getLength()) -- unlike
	 * getItem(jsize), this does not call checkIndex() itself.
	 */
	virtual JPPyObject getItem(jsize ndx, jobject resolved)
	{
		return getItem(ndx);
	}

	void       setItem(JPJavaFrame& frame, jsize ndx, PyObject*);

	/** Construct a slice of this array, preserving the concrete subclass
	 * (a slice of a JPArrayInt must still be a JPArrayInt, not degrade to
	 * some less-specific type). Overridden per concrete subclass.
	 */
	virtual JPArray* slice(jsize start, jsize stop, jsize step) = 0;

	/**
	 *  Create a shallow copy of an array.
	 *
	 * This is used to extract a slice before calling or casting operations.
	 *
	 * @param frame
	 * @param obj
	 * @return
	 */
	jarray     clone(JPJavaFrame& frame, PyObject* obj);

	/**
	 * Bulk-copy this array's elements into a caller-supplied writable
	 * Python buffer (JArray.pullTo). Primitive arrays only -- requires a
	 * matching element count and item size, but not a matching shape
	 * (dest may be any number of dimensions, so long as the total element
	 * count lines up).
	 *
	 * @param dest a writable buffer-protocol object.
	 */
	void       pullTo(PyObject* dest);

	/**
	 * Bulk-copy a caller-supplied readable Python buffer into this array's
	 * elements in place (JArray.pushFrom) -- the mirror of pullTo.
	 * Primitive arrays only -- requires a matching element count, but not
	 * a matching shape (src may be any number of dimensions, so long as
	 * the total element count lines up); the source dtype need not match
	 * the array's component type (a real converting fallback handles that
	 * case).
	 *
	 * @param src a readable buffer-protocol object.
	 */
	void       pushFrom(PyObject* src);

	/**
	 * Bulk-convert this array into a genuine Python list (JArray.toList()).
	 *
	 * For a primitive array, reads the whole range in a single JNI
	 * critical section (JPPrimitiveType::getArrayRange) instead of one JNI
	 * call per element. For an Object[]/nested-array component type, boxes
	 * each element individually (each element can be a distinct runtime
	 * type, so there is no bulk read to do) but recurses into any nested
	 * Java array so a multi-dim primitive array produces genuinely nested
	 * Python lists rather than a list of JArray wrapper objects.
	 *
	 * @param dtype Target primitive type for a forced cast, or nullptr to
	 * use this array's own component type (no cast). Only meaningful for
	 * primitive arrays; ignored once recursion reaches an Object[] level.
	 * @param wrap If true, box each element as a tagged wrapper instance
	 * of dtype (e.g. JInt); if false, return a plain Python int/float/
	 * bool/str. Ignored when dtype is nullptr (always plain in that case).
	 * @return a new Python list.
	 */
	JPPyObject toList(JPPrimitiveType* dtype = nullptr, bool wrap = false);

	bool       isSlice() const
	{
		return m_Slice;
	}

	jarray     getJava(JPJavaFrame& frame)
	{
		return (jarray) frame.retrieveGlobal(m_Object);
	}

protected:
	// Accessible to concrete subclasses: JPArrayInt::getItem() etc. need
	// m_Object/m_Start/m_Step directly; JPArrayObject::getItem() also
	// needs m_Class.
	//
	// m_Context is captured once at construction (m_Class->getContext(),
	// while m_Class is still guaranteed live) rather than re-derived from
	// m_Class on every access -- JPContext::shutdownJVM() deletes every
	// registered JPClass (including m_Class) via m_Resources, but the
	// JPContext object itself outlives that (only freed later, at Python
	// module teardown), so a captured m_Context stays valid for a getItem()
	// call after shutdown (see JVMNotRunning-on-shutdown tests) while
	// m_Class->getContext() at that point would be a use-after-free.
	JPContext*    m_Context;
	JPArrayClass* m_Class;
	jref          m_Object;
	jsize         m_Start;
	jsize         m_Step;
	jsize         m_Length;
	bool          m_Slice;
} ;

/** Component type is a plain class/interface (not primitive, not itself an
 * array). The default JPArray subclass -- see JPClass::createArrayWrapper.
 * getItem() constructs its own outer() frame and calls
 * JPClass::getArrayItem(JPJavaFrame&, ...).
 */
class JPArrayObject : public JPArray
{
public:
	explicit JPArrayObject(const JPValue& array);
	JPArrayObject(JPArrayObject* src, jsize start, jsize stop, jsize step);

	JPPyObject getItem(jsize ndx) override;
	JPArray* slice(jsize start, jsize stop, jsize step) override;
} ;

/** Component type is itself an array class (the outer level of int[][],
 * etc.). Structurally distinct from JPArrayObject, but currently
 * behaviorally identical (inherits getItem() unchanged) -- exists as a
 * real, named place in the type hierarchy for multi-dim-specific bulk
 * logic that's deliberately not attempted here. slice() must still be
 * overridden: JPArrayObject::slice() names its own concrete type
 * explicitly (`new JPArrayObject(this, ...)`), so leaving it
 * un-overridden here would silently degrade a sliced nested array from
 * JPArrayNested to JPArrayObject.
 */
class JPArrayNested : public JPArrayObject
{
public:
	explicit JPArrayNested(const JPValue& array);
	JPArrayNested(JPArrayNested* src, jsize start, jsize stop, jsize step);

	JPArray* slice(jsize start, jsize stop, jsize step) override;
} ;

class JPArrayBoolean : public JPArray
{
	JPBooleanType* m_CompType;
public:
	explicit JPArrayBoolean(const JPValue& array);
	JPArrayBoolean(JPArrayBoolean* src, jsize start, jsize stop, jsize step);

	JPPyObject getItem(jsize ndx) override;
	JPPyObject getItem(jsize ndx, jobject resolved) override;
	JPArray* slice(jsize start, jsize stop, jsize step) override;
} ;

class JPArrayByte : public JPArray
{
	JPByteType* m_CompType;
public:
	explicit JPArrayByte(const JPValue& array);
	JPArrayByte(JPArrayByte* src, jsize start, jsize stop, jsize step);

	JPPyObject getItem(jsize ndx) override;
	JPPyObject getItem(jsize ndx, jobject resolved) override;
	JPArray* slice(jsize start, jsize stop, jsize step) override;
} ;

class JPArrayChar : public JPArray
{
	JPCharType* m_CompType;
public:
	explicit JPArrayChar(const JPValue& array);
	JPArrayChar(JPArrayChar* src, jsize start, jsize stop, jsize step);

	JPPyObject getItem(jsize ndx) override;
	JPPyObject getItem(jsize ndx, jobject resolved) override;
	JPArray* slice(jsize start, jsize stop, jsize step) override;
} ;

class JPArrayShort : public JPArray
{
	JPShortType* m_CompType;
public:
	explicit JPArrayShort(const JPValue& array);
	JPArrayShort(JPArrayShort* src, jsize start, jsize stop, jsize step);

	JPPyObject getItem(jsize ndx) override;
	JPPyObject getItem(jsize ndx, jobject resolved) override;
	JPArray* slice(jsize start, jsize stop, jsize step) override;
} ;

class JPArrayInt : public JPArray
{
	JPIntType* m_CompType;
public:
	explicit JPArrayInt(const JPValue& array);
	JPArrayInt(JPArrayInt* src, jsize start, jsize stop, jsize step);

	JPPyObject getItem(jsize ndx) override;
	JPPyObject getItem(jsize ndx, jobject resolved) override;
	JPArray* slice(jsize start, jsize stop, jsize step) override;
} ;

class JPArrayLong : public JPArray
{
	JPLongType* m_CompType;
public:
	explicit JPArrayLong(const JPValue& array);
	JPArrayLong(JPArrayLong* src, jsize start, jsize stop, jsize step);

	JPPyObject getItem(jsize ndx) override;
	JPPyObject getItem(jsize ndx, jobject resolved) override;
	JPArray* slice(jsize start, jsize stop, jsize step) override;
} ;

class JPArrayFloat : public JPArray
{
	JPFloatType* m_CompType;
public:
	explicit JPArrayFloat(const JPValue& array);
	JPArrayFloat(JPArrayFloat* src, jsize start, jsize stop, jsize step);

	JPPyObject getItem(jsize ndx) override;
	JPPyObject getItem(jsize ndx, jobject resolved) override;
	JPArray* slice(jsize start, jsize stop, jsize step) override;
} ;

class JPArrayDouble : public JPArray
{
	JPDoubleType* m_CompType;
public:
	explicit JPArrayDouble(const JPValue& array);
	JPArrayDouble(JPArrayDouble* src, jsize start, jsize stop, jsize step);

	JPPyObject getItem(jsize ndx) override;
	JPPyObject getItem(jsize ndx, jobject resolved) override;
	JPArray* slice(jsize start, jsize stop, jsize step) override;
} ;

#endif // _JPARRAY_H_
