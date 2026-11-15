// --- file: common/jp_primitivetype.cpp ---
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
#include <cctype>
#include <vector>
#include "jpype.h"
#include "pyjp.h"

JPPrimitiveType::JPPrimitiveType(JPJavaFrame& frame, jclass cls, const string& name)
: JPClass(frame, cls, name, 0x411)
{
}

JPPrimitiveType::~JPPrimitiveType()
= default;

bool JPPrimitiveType::isPrimitive() const
{
	return true;
}

JPPyObject JPPrimitiveType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)  // GCOVR_EXCL_LINE
{
	// See the declaration's comment (jp_primitivetype.h) -- unreachable
	// today (JPArrayByte/JPArrayInt/etc. all use getFastArrayItem
	// instead), kept only so a future polymorphic caller can't silently
	// fall through to JPClass's jobjectArray-based default.
	JP_RAISE(PyExc_SystemError, "getArrayItem not implemented for primitive types; use getFastArrayItem");  // GCOVR_EXCL_LINE
}

PyObject *JPPrimitiveType::convertLong(PyTypeObject* wrapper, long long value)
{
	if (wrapper == nullptr)
		JP_RAISE(PyExc_SystemError, "bad wrapper");

	// Builds directly into the real wrapper instance. Callers used to
	// build a throwaway plain PyLongObject via PyLong_FromLong(Long) just
	// so this could immediately unpack it back out via PyLong_AsLongLong
	// -- a redundant allocation on every array-pull element, since the
	// caller already has the native value in hand (it came straight out
	// of a jvalue). Skip the round trip.
	return PyJPNumber_longFromLongLong(wrapper, value);
}

namespace
{

// Box a jvalue (already converted into dstCode's native representation)
// as a plain Python object -- no Java-value tagging, no wrapper class.
// This is the "dtype=None"/"dtype=int"/"dtype=float" case.
PyObject *boxPlain(char dstCode, jvalue v)
{
	switch (dstCode)
	{
		case 'Z': return PyBool_FromLong(v.z);
		case 'B': return PyLong_FromLong(v.b);
		case 'C': return PyUnicode_FromOrdinal(v.c);
		case 'S': return PyLong_FromLong(v.s);
		case 'I': return PyLong_FromLong(v.i);
		case 'J': return PyLong_FromLongLong(v.j);
		case 'F': return PyFloat_FromDouble(v.f);
		default:  return PyFloat_FromDouble(v.d); // 'D'
	}
}

// One adaptor, resolved once per getArrayRange() call from (srcCode,
// dtype, wrap) and then invoked unconditionally for every element --
// no per-element branching on dtype or typeCode. `caster` performs the
// numeric cast (identity when dtype is this type itself); `adapt` only
// decides plain-vs-wrapped boxing of the cast result.
using JPListAdaptor = PyObject* (*)(JPJavaFrame&, JPPrimitiveType*, jconverter, const void*);

PyObject *adaptPlain(JPJavaFrame& frame, JPPrimitiveType* dstType, jconverter caster, const void* src)
{
	return boxPlain(dstType->getTypeCode(), caster(const_cast<void*>(src)));
}

PyObject *adaptWrap(JPJavaFrame& frame, JPPrimitiveType* dstType, jconverter caster, const void* src)
{
	return dstType->convertToPythonObject(frame, caster(const_cast<void*>(src)), false).keep();
}

} // namespace

JPPyObject JPPrimitiveType::getArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize step, jsize len, JPPrimitiveType* dtype, bool wrap)
{
	JPPyObject list = JPPyObject::call(PyList_New(len));
	if (len == 0)
		return list;

	Py_ssize_t itemsize = getItemSize();
	char typeCode = getTypeCode();

	if (dtype != nullptr && typeCode == 'Z')
		JP_RAISE(PyExc_TypeError, "dtype is not supported for boolean arrays");

	JPPrimitiveType* dstType = (dtype != nullptr) ? dtype : this;
	bool doWrap = (dtype != nullptr) && wrap;

	char toCode[2] = { (char) tolower(dstType->getTypeCode()), '\0' };
	jconverter caster = getConverter(getBufferFormat(), (int) itemsize, toCode);
	if (caster == nullptr)
		JP_RAISE(PyExc_TypeError, "no conversion available for the requested dtype");

	JPListAdaptor adapt = doWrap ? &adaptWrap : &adaptPlain;

	// A GetPrimitiveArrayCritical pin held across this whole per-element
	// PyObject-allocation loop is the wrong tool even though it measures
	// no faster than the alternative below (verified: same numbers at
	// every size) -- the loop's duration scales with len and with
	// whatever CPython's own allocator does per element, which is
	// exactly the kind of unbounded hold time the JNI critical-section
	// contract warns against. Prefer a plain Get<Type>ArrayRegion copy
	// into a local buffer, released immediately, then convert from that
	// local memory with no JNI/GC interaction at all during boxing.
	// Region calls have no stride support, so this only applies to
	// step == 1 (an unsliced array, the overwhelmingly common case);
	// step != 1 falls back to the critical-section path below.
	if (step == 1)
	{
		std::vector<char> buf((size_t) itemsize * len);
		copyElements(frame, a, start, len, buf.data(), 0);
		const char *base = buf.data();
		for (jsize i = 0; i < len; ++i)
		{
			const char *src = base + (jlong) i * itemsize;
			PyList_SET_ITEM(list.get(), i, adapt(frame, dstType, caster, src));
		}
		return list;
	}

	jboolean isCopy;
	void *mem = frame.getEnv()->GetPrimitiveArrayCritical(a, &isCopy);
	JP_TRACE_JAVA("GetPrimitiveArrayCritical", mem);
	const char *base = (const char*) mem;

	for (jsize i = 0; i < len; ++i)
	{
		const char *src = base + (start + (jlong) i * step) * itemsize;
		PyList_SET_ITEM(list.get(), i, adapt(frame, dstType, caster, src));
	}

	JP_TRACE_JAVA("ReleasePrimitiveArrayCritical", mem);
	frame.getEnv()->ReleasePrimitiveArrayCritical(a, mem, JNI_ABORT);
	return list;
}

