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
#ifndef _JPYPE_H_
#define _JPYPE_H_

#ifdef __GNUC__
// Python requires char* but C++ string constants are const char*
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#ifdef WIN32

#ifndef __GNUC__ // Then this must mean a variant of GCC on win32 ...
#pragma warning (disable:4786)
#endif

#if defined(__CYGWIN__)
// jni_md.h does not work for cygwin.  Use this instead.
#elif defined(__GNUC__)
// JNICALL causes problem for function prototypes .. since I am not defining any JNI methods there is no need for it
#undef JNICALL
#define JNICALL
#endif

#endif

#include <jni.h>

// Define this and use to allow destructors to throw in C++11 or later
#if defined(_MSC_VER)

// Visual Studio C++ does not seem have changed __cplusplus since 1997
// see: https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-170&viewFallbackFrom=vs-2019
#if (_MSVC_LAND >= 201402)
#define NO_EXCEPT_FALSE noexcept(false)
#else
#define NO_EXCEPT_FALSE throw(JPBaseError)
#endif

#else

// For all the compilers that understand standards
#if (__cplusplus >= 201103L)
#define NO_EXCEPT_FALSE noexcept(false)
#else
#define NO_EXCEPT_FALSE throw(JPBaseError)
#endif

#endif

#include <map>
#include <string>
#include <vector>

using std::map;
using std::string;
using std::vector;

#ifdef JP_INSTRUMENTATION
#include <cstdint>
template <size_t i>
constexpr uint32_t _hash(const char *q, uint32_t v)
{
	return _hash < i - 1 > (q + 1, v * 0x1a481023 + q[0]);
}

template <>
constexpr uint32_t _hash<0>(const char *q, uint32_t v)
{
	return v;
}
#define compile_hash(x) _hash<sizeof(x)-1>(x, 0)

extern void PyJPModuleFault_throw(uint32_t code);
extern int PyJPModuleFault_check(uint32_t code);
#define JP_TRACE_IN(X, ...) try { PyJPModuleFault_throw(compile_hash(X));
#define JP_FAULT_RETURN(X, Y)  if (PyJPModuleFault_check(compile_hash(X))) return Y
#define JP_BLOCK(X)  if (PyJPModuleFault_check(compile_hash(X))==0)
#else
#define JP_FAULT_RETURN(X, Y)  if (false) while (false)
#define JP_BLOCK(X)  if (false) while (false)
#endif

/** Definition of commonly used template types */
using StringVector = vector<string>;

/**
 * Converter are used for bulk byte transfers from Python to Java.
 */
using jconverter = jvalue (*)(void *) ;

/**
 * Create a converter for a bulk byte transfer.
 *
 * Bulk transfers do not check for range and may be lossy.  These are only
 * triggered when a transfer either using memoryview or a slice operator
 * assignment from a buffer object (such as numpy.array).  Converters are
 * created once at the start of the transfer and used to convert each
 * byte by casting the memory and then assigning to the jvalue union with
 * the requested type.
 *
 * Byte order is handled here too: a format string prefixed with '<', '>'
 * or '!' selects a byte-swapping converter (the Reverse<> wrapper in
 * jp_convert.cpp) when it disagrees with the platform's native order.
 *
 * @param from is a Python struct designation
 * @param itemsize is the size of the Python item
 * @param to is the desired Java primitive type
 * @return a converter function to convert each member.
 */
extern jconverter getConverter(const char* from, int itemsize, const char* to);

extern bool _jp_cpp_exceptions;

// Types
class JPClass;
class JPValue;
class JPProxy;
class JPArray;
class JPArrayClass;
class JPArrayView;
class JPBoxedType;
class JPPrimitiveType;
class JPBooleanType;
class JPByteType;
class JPCharType;
class JPShortType;
class JPIntType;
class JPLongType;
class JPFloatType;
class JPDoubleType;
class JPStringType;

/**
 * Classification of how cheaply a source buffer's bytes can be turned into
 * pcls's array elements, from cheapest to "no bulk shortcut available":
 *
 *  - RAW_NATIVE: byte-for-byte reinterpret, no conversion of any kind --
 *    `converter` is pointer-identical to the converter getConverter()
 *    resolves for pcls's own canonical buffer format/item size.
 *  - RAW_SWAPPED: same numeric kind and width as pcls's own type, but the
 *    source declares a non-native byte order -- a plain byte-swap (no
 *    numeric reinterpretation) turns it into RAW_NATIVE.
 *  - RAW_HALF_NATIVE / RAW_HALF_SWAPPED: source is IEEE 754 half-precision
 *    ('e' format, itemsize 2), native or swapped order respectively --
 *    always a real conversion (there is no native 16-bit float type to
 *    reinterpret into), but decoding it is simple, fixed-cost, per-element
 *    work that a bulk Java-side pass handles far more cheaply than the
 *    general per-row-critical-section fallback.
 *  - RAW_NONE: no bulk shortcut applies (genuine dtype coercion, e.g.
 *    float64 -> int32) -- must fall back to the general element-by-element
 *    converter path.
 *
 * Used to decide whether a multi-dim buffer push
 * (JPConversionMultiArrayBuffer) or a flat JArray.push can take a fast
 * direct-buffer-handoff path instead of the general element-by-element
 * converter path. `code` must be
 * the same target-code string already passed to the getConverter() call
 * that produced `converter`; `format`/`itemsize` are the source buffer's
 * own (`Py_buffer.format`/`Py_buffer.itemsize`).
 */
enum JPRawTransferMode
{
	RAW_NONE = 0,
	RAW_NATIVE = 1,
	RAW_SWAPPED = 2,
	RAW_HALF_NATIVE = 3,
	RAW_HALF_SWAPPED = 4,
};

extern JPRawTransferMode classifyRawTransfer(jconverter converter, JPPrimitiveType* pcls,
		const char* format, int itemsize, const char* code);

/**
 * A buffer-protocol source element's kind/width/byte-order, as classified
 * from a Py_buffer's format string -- the source-side counterpart to
 * JPRawTransferMode, but describing the source itself rather than its
 * relationship to one particular target type. Used by JPConversionBuffer's
 * 1D fast path (jp_classhints.cpp) to hand dtype coercion to
 * Support.fillFlatFromBuffer instead of doing it element-by-element in
 * C++ via a jconverter.
 */
struct JPBufferSource
{
	char kind; // 'i' signed int, 'u' unsigned int, 'f' float (incl. half at size==2)
	int size;  // element width in bytes
	bool swapped; // byte order differs from native
};

/**
 * Classify format/itemsize the same way getConverter (jp_convert.cpp)
 * parses its `from` argument -- same byte-order-prefix stripping, same
 * itemsize==8 'l'/'L' -> 'q'/'Q' aliasing -- but only far enough to
 * describe the source, not to pick a target-specific converter. Returns
 * false for anything getConverter itself wouldn't recognize (complex,
 * structured/record dtypes, ...); callers fall back to the general
 * per-element converter path in that case, same as before this existed.
 */
extern bool classifyBufferSource(const char* format, int itemsize, JPBufferSource& out);

// Members
class JPMethod;
class JPMethodDispatch;
class JPField;

// Services
class JPTypeManager;
class JPClassLoader;
class JPContext;
class JPBuffer;
class JPPyObject;

extern "C" using JCleanupHook = void (*)(void *) ;
extern "C" struct JPConversionInfo;

using JPClassList = vector<JPClass *>;
using JPFieldList = vector<JPField *>;
using JPMethodDispatchList = vector<JPMethodDispatch *>;
using JPMethodList = vector<JPMethod *>;

class JPResource
{
public:
	virtual ~JPResource() = 0;
} ;

// Macros for raising an exception with jpype
//   These must be macros so that we can update the pattern and
//   maintain the appropriate auditing information.  C++ does not
//   have a lot for facilities to make this easy.
#define JP_RAISE_PYTHON()                   { throw JPPythonError::fetch(JP_STACKINFO()); }
#define JP_RAISE_OS_ERROR_UNIX(err, msg)    { throw JPInternalError(msg, err, JP_STACKINFO()); }
#define JP_RAISE_OS_ERROR_WINDOWS(err, msg) { throw JPInternalError(msg, err, JP_STACKINFO()); }
#define JP_RAISE(type, msg)                 { throw JPInternalError(type, msg, JP_STACKINFO()); }

#ifndef PyObject_HEAD
struct _object;
using PyObject = _object;
#endif

#include "jp_pythontypes.h"

template <typename... T>
static inline JPPyObject JPPyTuple_Pack(T... args) {
	return JPPyObject::call(PyTuple_Pack(sizeof...(T), args...));
}

// Base utility headers
#include "jp_javaframe.h"
#include "jp_context.h"
#include "jp_exception.h"
#include "jp_error.h"
#include "jp_tracer.h"
#include "jp_typemanager.h"
#include "jp_encoding.h"
#include "jp_modifier.h"
#include "jp_match.h"

// Other header files
#include "jp_classhints.h"
#include "jp_method.h"
#include "jp_value.h"
#include "jp_class.h"

// Primitives classes
#include "jp_primitivetype.h"

/**
 * Shared fast path for JPClass::setArrayRange's 8 primitive overrides
 * (JPIntType::setArrayRange etc., jp_<type>type.cpp) -- covers both
 * JPArray::setRange (Python slice assignment, `javaArr[:] = numpy_array`)
 * and JPArray::clone (jp_array.cpp), the two call sites that write into an
 * *existing* array at an arbitrary destination start/step and therefore
 * can't go through JPConversionBuffer's argument-conversion dispatch
 * (jp_classhints.cpp), which always allocates a fresh array. Tries the
 * same single-JNI-call buffer-handoff (Support.fillFlatIntoArray) used by
 * JPConversionBuffer's own fast path; returns false (nothing done, caller
 * falls back to its existing per-element loop) whenever the source isn't
 * ndim==1, isn't a recognized numeric format, or has a non-positive
 * stride (a reversed numpy view) -- same scope limits as
 * JPConversionBuffer's fast path, for the same reason (not worth the
 * extra base-address arithmetic). Does not validate length itself --
 * false on a mismatch (view.shape[0] != length) is fine, since the
 * caller's own fallback path already raises the right error for that.
 */
extern bool tryFastBufferPush(JPJavaFrame &frame, JPPrimitiveType *pcls, jarray dest,
		jsize start, jsize step, jsize length, PyObject *sequence);

/**
 * Shared fast path for constructing a brand-new N-D primitive array
 * (int[][], double[][][], ...) directly from a buffer-protocol source
 * whose ndim already matches the target nesting depth -- the multi-dim
 * counterpart to tryFastBufferPush above. Used by both
 * JPConversionMultiArrayBuffer::convert (jp_classhints.cpp, the
 * method-argument push path) and JArray.of()'s N-D case
 * (PyJPModule_convertBuffer, pyjp_module.cpp), which were previously two
 * separate call sites doing the same classifyRawTransfer-gated
 * DirectByteBuffer handoff to Support.fillMultiArrayFromBuffer.
 *
 * `buffer` must already be validated (PyBUF_STRIDES | PyBUF_FORMAT,
 * view.ndim == the array's nesting depth). `jdims` is the caller's
 * already-built int[] of view.shape. On success returns true and sets
 * `out` to the newly constructed array; returns false (out untouched,
 * caller falls back to its existing per-element newMultiArray/
 * newMultiArrayObject path) whenever the source isn't C-contiguous or
 * requires genuine dtype coercion rather than a fixed bulk-friendly
 * reinterpret/byte-swap/half-decode.
 */
extern bool tryFastMultiArrayBuffer(JPJavaFrame &frame, JPPrimitiveType *pcls,
		JPPyBuffer &buffer, jintArray jdims, jarray &out);

/** Build a Java int[] of view.shape[0..view.ndim), for use as the `jdims`
 * argument to tryFastMultiArrayBuffer (and the N-D newMultiArray fallback
 * paths that still need it after tryFastMultiArrayBuffer declines).
 *
 * Requires view.shape != nullptr (guaranteed whenever the buffer was
 * obtained with PyBUF_ND or PyBUF_STRIDES, which imply PyBUF_ND).
 */
extern jintArray buildDimsArray(JPJavaFrame &frame, Py_buffer &view);

#endif // _JPYPE_H_
