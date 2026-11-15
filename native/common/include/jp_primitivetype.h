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
#ifndef _JPPRIMITIVETYPE_H_
#define _JPPRIMITIVETYPE_H_
#include "jp_boxedtype.h"

class JPPrimitiveType : public JPClass
{
protected:
	explicit JPPrimitiveType(JPJavaFrame& frame, jclass cls, const string& name);
	~JPPrimitiveType() override;

public:
	bool isPrimitive() const override;

	virtual JPClass* getBoxedClass(JPJavaFrame& frame) const = 0;

	virtual char getTypeCode() = 0;
	virtual jlong getAsLong(jvalue v) = 0;
	virtual jdouble getAsDouble(jvalue v) = 0;

	void setClass(JPJavaFrame& frame, jclass o)
	{
		m_Class = frame.storeGlobal(o);
	}

	// The virtual JPClass::getArrayItem is only ever called polymorphically
	// through JPArrayObject (jp_array.cpp), which is only constructed for
	// reference-typed array components -- JPArray::create always builds a
	// dedicated JPArrayByte/JPArrayInt/etc. wrapper for a primitive
	// component, and that wrapper's getItem() calls getFastArrayItem
	// instead. So this override can never actually be reached today, but
	// still needs to exist: JPClass's own default implementation treats
	// `a` as a jobjectArray (GetObjectArrayElement), which would
	// misbehave on a primitive array if anything -- now or in the future
	// -- ever did dispatch getArrayItem polymorphically for one. One
	// shared override here (replacing seven duplicate per-type ones)
	// keeps that guarantee without seven copies of unreachable code.
	JPPyObject getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx) override;

	virtual void getView(JPJavaFrame& frame, JPArrayView& view) = 0;
	virtual void releaseView(JPJavaFrame& frame, JPArrayView& view) = 0;
	virtual const char* getBufferFormat() = 0;
	virtual Py_ssize_t getItemSize() = 0;
	virtual void copyElements(JPJavaFrame &frame,
			jarray a, jsize start, jsize len,
			void* memory, int offset) = 0;

	// Mirror of copyElements (Get<Type>ArrayRegion) in the opposite
	// direction (Set<Type>ArrayRegion) -- used by JPArray::pushFrom's raw
	// fast path to write a matching-dtype, contiguous source buffer
	// straight into an existing Java array in a single JNI call.
	virtual void setElements(JPJavaFrame &frame,
			jarray a, jsize start, jsize len,
			const void* memory, int offset) = 0;

	virtual PyObject *newMultiArray(JPJavaFrame &frame,
			JPPyBuffer& view, int subs, int base, jobject dims) = 0;

	// Same traversal as newMultiArray, but returns the raw Java array
	// (a local ref) instead of wrapping it as a Python object -- for use
	// from a JPConversion::convert(), which needs a jvalue, not a PyObject.
	// converter must already be resolved (see getConverter in jpype.h).
	virtual jobject newMultiArrayObject(JPJavaFrame &frame,
			JPPyBuffer& view, jconverter converter, int subs, int base, jobject dims) = 0;

	// Helper for Long types -- builds the wrapper instance directly from
	// the native value (no throwaway PyLongObject on the way in; see
	// jp_primitivetype.cpp).
	PyObject *convertLong(PyTypeObject* wrapper, long long value);

	/**
	 * Bulk-read a (possibly strided) range of a primitive array's elements
	 * into a new Python list in one JNI critical section, instead of one
	 * JNI call (getArrayItem) per element -- closes the `array->list` pull
	 * gap. Boxing itself (one PyObject per
	 * element) still happens per element, same as getArrayItem -- that
	 * part is unavoidable for a real Python list -- but it reuses
	 * convertToPythonObject so behavior (including any registered host
	 * customization) matches getArrayItem exactly, just without the
	 * redundant JNI round trips. Works uniformly for every primitive type
	 * via getTypeCode()/getItemSize(), so it lives here rather than as a
	 * per-type override.
	 *
	 * @param dtype Target primitive type for a forced cast (see
	 * getConverter/jconverter), or nullptr to use this type itself (no
	 * cast). @param wrap If true, box each element as a tagged wrapper
	 * instance of dtype (as convertToPythonObject would); if false,
	 * return a plain Python int/float/bool/str. Ignored when dtype is
	 * nullptr, which is always plain.
	 */
	JPPyObject getArrayRange(JPJavaFrame& frame, jarray a, jsize start, jsize step, jsize len,
			JPPrimitiveType* dtype = nullptr, bool wrap = false);
} ;

#endif
