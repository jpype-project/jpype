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
#include "jpype.h"
#include "pyjp.h"
#include "jp_array.h"
#include "jp_arrayclass.h"
#include "jp_classhints.h"
#include "jp_context.h"
#include "jp_stringtype.h"

JPArrayClass::JPArrayClass(JPJavaFrame& frame,
		jclass cls,
		const string& name,
		JPClass* superClass,
		JPClass* componentType,
		jint modifiers)
: JPClass(frame, cls, name, superClass, JPClassList(), modifiers)
{
	m_ComponentType = componentType;

	// componentType is always fully constructed already (array classes are
	// built bottom-up), so this can be resolved once here rather than with
	// a dynamic_cast walk on every JPConversionMultiArrayBuffer::matches().
	auto *actl = dynamic_cast<JPArrayClass*>(componentType);
	if (actl != nullptr)
	{
		m_MultiArrayDepth = actl->m_MultiArrayDepth + 1;
		m_MultiArrayLeaf = actl->m_MultiArrayLeaf;
	} else
	{
		m_MultiArrayDepth = 1;
		// componentType is never a caller-local stack object here: array
		// classes are built bottom-up (see comment above) from a
		// componentType that is always an already-fully-constructed,
		// heap-allocated JPClass registered with the context, so this
		// dynamic_cast only ever repositions that same long-lived pointer
		// -- it doesn't manufacture a new local whose address could
		// outlive the caller. Matches the plain (unflagged) m_ComponentType
		// = componentType store just above.
		m_MultiArrayLeaf = dynamic_cast<JPPrimitiveType*>(componentType); // lgtm [cpp/local-variable-address-stored-in-non-local-memory]
	}
}

JPArrayClass::~JPArrayClass()
= default;

JPMatch::Type JPArrayClass::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPArrayClass::findJavaConversion");
	// This base is used directly for a plain class/interface component
	// (String[], Foo[]) and for a nested array that doesn't bottom out in
	// a primitive leaf (Object[][], String[][], ...) -- neither case can
	// ever match char/byte/buffer/multiArrayBuffer/raggedSequence (see
	// JPArrayClassChar/Byte/Xxx and JPArrayClassNested/NestedRagged in
	// jp_arrayclass.h), so this chain never tries any of them at all,
	// unconditionally, rather than gating them with a runtime check that
	// could be skipped if an earlier branch in an || chain already
	// matched.
	if (nullConversion->matches(this, match)
			|| objectConversion->matches(this, match)
			|| listConversion->matches(this, match)
			|| tupleConversion->matches(this, match)
			|| sequenceConversion->matches(this, match)
			|| hintsConversion->matches(this, match)
			)
		return match.type;
	JP_TRACE("None");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPArrayClass::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	objectConversion->getInfo(this, info);
	sequenceConversion->getInfo(this, info);
	hintsConversion->getInfo(this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}

JPArrayClass* JPArrayClass::createArrayClass(JPJavaFrame& frame, jclass cls,
		const string& name, JPClass* superClass, jint modifiers)
{
	// `this` is the component type of the array being built, i.e. the
	// *inner* array (int[] when building int[][]) -- its own leaf/depth
	// fields (set once at its own construction, above) fully determine
	// which specialization the new, one-level-deeper array class needs.
	// Decided once, here, rather than carried as a runtime condition
	// into the new class's findJavaConversionImpl.
	if (m_MultiArrayLeaf == nullptr)
		return new JPArrayClass(frame, cls, name, superClass, this, modifiers);
	if (isRaggedEligible(m_MultiArrayLeaf->getTypeCode()))
		return new JPArrayClassNestedRagged(frame, cls, name, superClass, this, modifiers);
	return new JPArrayClassNested(frame, cls, name, superClass, this, modifiers);
}

JPMatch::Type JPArrayClassNested::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPArrayClassNested::findJavaConversion");
	if (nullConversion->matches(this, match)
			|| objectConversion->matches(this, match)
			|| multiArrayBufferConversion->matches(this, match)
			|| sequenceConversion->matches(this, match)
			|| hintsConversion->matches(this, match)
			)
		return match.type;
	JP_TRACE("None");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPArrayClassNested::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	objectConversion->getInfo(this, info);
	multiArrayBufferConversion->getInfo(this, info);
	sequenceConversion->getInfo(this, info);
	hintsConversion->getInfo(this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}

JPMatch::Type JPArrayClassNestedRagged::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPArrayClassNestedRagged::findJavaConversion");
	// raggedSequenceConversion must never be reached for a flat, depth-1
	// array (see RESULTS.md for the measured ~50-70% int[] push
	// slowdown from doing so): at depth 1, sequenceConversion::convert()
	// calls the primitive-type-specific setArrayRange override (e.g.
	// JPIntType::setArrayRange, a single JNI critical-pin + tight write
	// loop, no verify/copy split) rather than JPClass::setArrayRange's
	// generic non-primitive-component fallback, so there's no
	// redundant-pass problem for raggedSequenceConversion to fix at
	// depth 1 the way there is for a genuinely nested array (this
	// class, only ever used at depth >= 2, where the redundant
	// re-matching happens because the component type is itself an
	// array class).
	if (nullConversion->matches(this, match)
			|| objectConversion->matches(this, match)
			|| multiArrayBufferConversion->matches(this, match)
			|| raggedSequenceConversion->matches(this, match)
			|| sequenceConversion->matches(this, match)
			|| hintsConversion->matches(this, match)
			)
		return match.type;
	JP_TRACE("None");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPArrayClassNestedRagged::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	objectConversion->getInfo(this, info);
	multiArrayBufferConversion->getInfo(this, info);
	raggedSequenceConversion->getInfo(this, info);
	sequenceConversion->getInfo(this, info);
	hintsConversion->getInfo(this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}

JPPyObject JPArrayClass::convertToPythonObject(JPJavaFrame& frame, jvalue value, bool cast)
{
	JP_TRACE_IN("JPArrayClass::convertToPythonObject");
	if (!cast)
	{
		if (value.l == nullptr)
			return JPPyObject::getNone();
	}
	JPPyObject wrapper = PyJPClass_create(frame, this);
	JPPyObject obj = PyJPArray_create(frame, (PyTypeObject*) wrapper.get(), JPValue(this, value));
	return obj;
	JP_TRACE_OUT;
}

jvalue JPArrayClass::convertToJavaVector(JPJavaFrame& frame, JPPyObjectVector& refs, jsize start, jsize end)
{
	JP_TRACE_IN("JPArrayClass::convertToJavaVector");
	auto length = (jsize) (end - start);

	jarray array = m_ComponentType->newArrayOf(frame, length);
	jvalue res;
	for (jsize i = start; i < end; i++)
	{
		m_ComponentType->setArrayItem(frame, array, i - start, refs[i]);
	}
	res.l = array;
	return res;
	JP_TRACE_OUT;
}

JPValue JPArrayClass::newArray(JPJavaFrame& frame, int length)
{
	jvalue v;
	v.l = m_ComponentType->newArrayOf(frame, length);
	return JPValue(this, v);
}

JPArray* JPArrayClass::createArrayWrapper(const JPValue& value)
{
	return new JPArrayNested(value);
}

JPArrayNested::JPArrayNested(const JPValue& array)
: JPArrayObject(array)
{
}

JPArrayNested::JPArrayNested(JPArrayNested* src, jsize start, jsize stop, jsize step)
: JPArrayObject(src, start, stop, step)
{
}

JPArray* JPArrayNested::slice(jsize start, jsize stop, jsize step)
{
	return new JPArrayNested(this, start, stop, step);
}
