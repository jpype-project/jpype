# *****************************************************************************
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   See NOTICE file for details.
#
# *****************************************************************************
import sys
import unittest
import jpype
import common
import random
import _jpype
import jpype
from jpype import java
from jpype.types import *
try:
    import numpy as np
except ImportError:
    pass

VALUES = [random.randint(-2**31, 2**31 - 1) for i in range(10)]


class JIntTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.cls = JClass("jpype.common.Fixture")
        self.fixture = self.cls()

    @common.requireInstrumentation
    def testJPNumberLong_int(self):
        jd = JInt(1)
        _jpype.fault("PyJPNumberLong_int")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(jd)
        int(jd)

    @common.requireInstrumentation
    def testJPNumberLong_float(self):
        jd = JInt(1)
        _jpype.fault("PyJPNumberLong_float")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(jd)
        float(jd)

    @common.requireInstrumentation
    def testJPNumberLong_str(self):
        jd = JInt(1)
        _jpype.fault("PyJPNumberLong_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jd)
        str(jd)

    @common.requireInstrumentation
    def testJPNumberLong_repr(self):
        jd = JInt(1)
        _jpype.fault("PyJPNumberLong_repr")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(jd)
        repr(jd)

    @common.requireInstrumentation
    def testJPNumberLong_compare(self):
        jd = JInt(1)
        _jpype.fault("PyJPNumberLong_compare")
        with self.assertRaisesRegex(SystemError, "fault"):
            jd == 1
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            jd == 1
        jd == 1

    @common.requireInstrumentation
    def testJPNumberLong_hash(self):
        jd = JInt(1)
        _jpype.fault("PyJPNumberLong_hash")
        with self.assertRaises(SystemError):
            hash(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            hash(jd)
        hash(jd)

    @common.requireInstrumentation
    def testFault(self):
        _jpype.fault("JPIntType::findJavaConversion")
        with self.assertRaises(SystemError):
            JInt(1.0)

    @common.requireInstrumentation
    def testConversionFault(self):
        _jpype.fault("JPIntType::findJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JInt._canConvertToJava(object())

    @common.requireInstrumentation
    def testArrayFault(self):
        ja = JArray(JInt)(5)
        _jpype.fault("JPJavaFrame::NewIntArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JInt)(1)
        _jpype.fault("JPJavaFrame::SetIntArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 0
        _jpype.fault("JPJavaFrame::GetIntArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])
        _jpype.fault("JPJavaFrame::GetIntArrayElements")
        # Special case, only BufferError is allowed from getBuffer
        with self.assertRaises(BufferError):
            memoryview(ja[0:3])
        # ja[0:3] = bytes(...) and cloning a slice both go through
        # tryFastBufferPush's DirectByteBuffer handoff now (setArrayRange
        # tries it before falling back to the Get/ReleaseIntArrayElements
        # critical section), so the fault point to arm is
        # fillFlatIntoArray, not ReleaseIntArrayElements -- that release
        # call is never reached for a buffer-protocol source.
        _jpype.fault("JPJavaFrame::fillFlatIntoArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:3] = bytes([1, 2, 3])
        _jpype.fault("JPJavaFrame::fillFlatIntoArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            jpype.JObject(ja[::2], jpype.JObject)
        _jpype.fault("JPJavaFrame::ReleaseIntArrayElements")

        def f():
            # Special case no fault is allowed
            memoryview(ja[0:3])
        f()
        _jpype.fault("JPIntType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0, 0]

    def testFromJIntWiden(self):
        self.assertEqual(JInt(JByte(123)), 123)
        self.assertEqual(JInt(JShort(12345)), 12345)
        self.assertEqual(JInt(JInt(12345678)), 12345678)
        self.assertEqual(JInt(JLong(12345678)), 12345678)

    def testFromJIntWiden_2(self):
        self.assertEqual(JInt(JDouble(12345678)), 12345678)

    def testFromNone(self):
        with self.assertRaises(TypeError):
            JInt(None)
        self.assertEqual(JInt._canConvertToJava(None), "none")

    def testUnBox(self):
        self.assertEqual(JInt(java.lang.Double(1.2345)), 1)

    def testFromFloat(self):
        self.assertEqual(JInt._canConvertToJava(1.2345), "explicit")

        @jpype.JImplements("java.util.function.IntSupplier")
        class q(object):
            @jpype.JOverride
            def getAsInt(self):
                return 4.5  # this will hit explicit conversion
        self.assertEqual(JObject(q()).getAsInt(), 4)

    def testFromLong(self):
        self.assertEqual(JInt(12345), 12345)
        self.assertEqual(JInt._canConvertToJava(12345), "implicit")

    def testFromObject(self):
        with self.assertRaises(TypeError):
            JInt(object())
        with self.assertRaises(TypeError):
            JInt(JObject())
        with self.assertRaises(TypeError):
            JInt(JString("A"))
        self.assertEqual(JInt._canConvertToJava(object()), "none")
        ja = JArray(JInt)(5)
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.common.Fixture")
        with self.assertRaises(TypeError):
            jf.static_int_field = object()
        with self.assertRaises(TypeError):
            jf().int_field = object()

    def testCallFloatFromNone(self):
        with self.assertRaises(TypeError):
            self.fixture.callFloat(None)
        with self.assertRaises(TypeError):
            self.fixture.static_int_field = None
        with self.assertRaises(TypeError):
            self.fixture.int_field = None

    def testThrow(self):
        #  Check throw
        with self.assertRaises(JException):
            self.fixture.throwInt()
        with self.assertRaises(JException):
            self.cls.throwStaticInt()
        with self.assertRaises(JException):
            self.fixture.throwStaticInt()

    def checkType(self, q):
        #  Check field
        self.fixture.int_field = q
        self.assertEqual(self.fixture.int_field, q)
        self.assertEqual(self.fixture.getInt(), q)
        #  Check static field
        self.cls.static_int_field = q
        self.assertEqual(self.fixture.static_int_field, q)
        self.assertEqual(self.fixture.getStaticInt(), q)
        self.assertEqual(self.cls.getStaticInt(), q)
        #  Check call
        self.assertEqual(self.fixture.callInt(q), q)
        self.assertEqual(self.cls.callStaticInt(q), q)

    def checkTypeFail(self, q, exc=TypeError):
        with self.assertRaises(exc):
            self.fixture.int_field = q
        with self.assertRaises(exc):
            self.fixture.callInt(q)
        with self.assertRaises(exc):
            self.fixture.callStaticInt(q)

    def testCastFloat(self):
        self.fixture.int_field = JInt(6.0)
        self.assertEqual(self.fixture.int_field, 6)

    def testCheckInt(self):
        self.checkType(1)

    def testCheckFloat(self):
        self.checkTypeFail(2.0)

    def testCheckRange(self):
        self.checkType(2**31 - 1)
        self.checkType(-2**31)
        self.checkTypeFail(2**31, exc=OverflowError)
        self.checkTypeFail(-2**31 - 1, exc=OverflowError)

    def testExplicitRange(self):
        # These will not overflow as they are explicit casts
        self.assertEqual(JInt(2**32), 0)
        self.assertEqual(JInt(-2**32), 0)

    def testCheckBool(self):
        self.checkType(True)
        self.checkType(False)

    def testCheckJBoolean(self):
        self.checkTypeFail(JBoolean(True))
        self.checkTypeFail(JBoolean(False))

    def testCheckJChar(self):
        self.checkType(JChar("A"))

    def testCheckJByte(self):
        self.checkType(JByte(-128))
        self.checkType(JByte(127))

    def testCheckJShort(self):
        self.checkType(JShort(-2**15))
        self.checkType(JShort(2**15 - 1))

    def testCheckJInt(self):
        self.checkType(JInt(-2**31 + 1))
        self.checkType(JInt(2**31 - 1))

    def testCheckJLong(self):
        self.checkTypeFail(JLong(-2**63 + 1))
        self.checkTypeFail(JLong(2**63 - 1))

    @common.requireNumpy
    def testCheckNumpyInt8(self):
        self.checkType(np.random.randint(-127, 128, dtype=np.int8))
        self.checkType(np.random.randint(0, 255, dtype=np.uint8))
        self.checkType(np.uint8(0))
        self.checkType(np.uint8(255))
        self.checkType(np.int8(-128))
        self.checkType(np.int8(127))

    @common.requireNumpy
    def testCheckNumpyInt16(self):
        self.checkType(np.random.randint(-2**15, 2**15 - 1, dtype=np.int16))
        self.checkType(np.random.randint(0, 2**16 - 1, dtype=np.uint16))
        self.checkType(np.uint16(0))
        self.checkType(np.uint16(2**16 - 1))
        self.checkType(np.int16(-2**15))
        self.checkType(np.int16(2**15 - 1))

    @common.requireNumpy
    def testCheckNumpyInt32(self):
        self.checkType(np.uint32(0))
        self.checkTypeFail(np.uint32(2**32 - 1), exc=OverflowError)
        self.checkType(np.int32(-2**31))
        self.checkType(np.int32(2**31 - 1))

    @common.requireNumpy
    def testCheckNumpyInt64(self):
        #self.checkTypeFail(np.random.randint(-2**63,2**63-1, dtype=np.int64))
        # FIXME OverflowError
        #self.checkType(np.uint64(np.random.randint(0,2**64-1, dtype=np.uint64)))
        # FIXME OverflowError
        # self.checkType(np.uint64(2**64-1))
        self.checkTypeFail(np.int64(-2**63), OverflowError)
        self.checkTypeFail(np.int64(2**63 - 1), OverflowError)

    @common.requireNumpy
    def testCheckNumpyFloat32(self):
        self.checkTypeFail(np.float32(np.random.rand()))

    @common.requireNumpy
    def testCheckNumpyFloat64(self):
        self.checkTypeFail(np.float64(np.random.rand()))

    def checkArrayType(self, a, expected):
        # Check init
        ja = JArray(JInt)(a)
        self.assertElementsEqual(ja, expected)
        ja = JArray(JInt)(len(a))
        ja[:] = a
        self.assertElementsEqual(ja, expected)
        return ja

    def checkArrayTypeFail(self, a):
        # Check init
        ja = JArray(JInt)(a)
        ja = JArray(JInt)(len(a))
        ja[:] = a

    def testArrayConversion(self):
        a = [random.randint(-2**31, 2**31) for i in range(100)]
        jarr = self.checkArrayType(a, a)
        result = jarr[2:10]
        self.assertEqual(len(a[2:10]), len(result))
        self.assertElementsAlmostEqual(a[2:10], result)

        # empty slice
        result = jarr[-1:3]
        expected = a[-1:3]
        self.assertElementsAlmostEqual(expected, result)

        result = jarr[3:-2]
        expected = a[3:-2]
        self.assertElementsAlmostEqual(expected, result)

    @common.requireNumpy
    def testArrayInitFromNPInt(self):
        a = np.random.randint(-2**31, 2**31 - 1, size=100, dtype=np.int_)
        self.checkArrayType(a, a)

    @common.requireNumpy
    def testArrayInitFromNPInt8(self):
        a = np.random.randint(-2**7, 2**7 - 1, size=100, dtype=np.int8)
        self.checkArrayType(a, a)

    @common.requireNumpy
    def testArrayInitFromNPInt16(self):
        a = np.random.randint(-2**15, 2**15 - 1, size=100, dtype=np.int16)
        self.checkArrayType(a, a)

    @common.requireNumpy
    def testArrayInitFromNPInt32(self):
        a = np.random.randint(-2**31, 2**31 - 1, size=100, dtype=np.int32)
        self.checkArrayType(a, a)

    @common.requireNumpy
    def testArrayInitFromNPInt64(self):
        a = np.random.randint(-2**63, 2**63 - 1, size=100, dtype=np.int64)
        self.checkArrayType(a, a.astype(np.int32))

    @common.requireNumpy
    def testArrayInitFromNPFloat32(self):
        a = np.random.random(100).astype(np.float32)
        self.checkArrayType(a, a.astype(np.int32))

    @common.requireNumpy
    def testArrayInitFromNPFloat64(self):
        a = np.random.random(100).astype(np.float64)
        self.checkArrayType(a, a.astype(np.int32))

    def testArraySetRange(self):
        ja = JArray(JInt)(3)
        ja[0:1] = [123]
        self.assertEqual(ja[0], 123)
        ja[0:1] = [-1]
        self.assertEqual(ja[0], -1)
        with self.assertRaises(TypeError):
            ja[0:1] = [1.000]
        with self.assertRaises(TypeError):
            ja[0:1] = [java.lang.Double(321)]
        with self.assertRaises(TypeError):
            ja[0:1] = [object()]

    def testArraySetRangeTuple(self):
        ja = JArray(JInt)(3)
        ja[0:2] = (123, -1)
        self.assertEqual(list(ja[0:2]), [123, -1])
        with self.assertRaises(TypeError):
            ja[0:1] = (1.000,)
        with self.assertRaises(TypeError):
            ja[0:1] = (object(),)

    def testArraySetRangeSequence(self):
        ja = JArray(JInt)(3)
        ja[0:2] = common.GenericSequence([123, -1])
        self.assertEqual(list(ja[0:2]), [123, -1])
        with self.assertRaises(TypeError):
            ja[0:1] = common.GenericSequence([1.000])
        with self.assertRaises(TypeError):
            ja[0:1] = common.GenericSequence([object()])

    def testArraySetRangeTupleNonExactIndex(self):
        # A bool is a valid __index__ object but not PyLong_CheckExact --
        # exercises the TUPLE loop's PyIndex_Check fallback conversion.
        ja = JArray(JInt)(2)
        ja[0:2] = (1, True)
        self.assertEqual(list(ja[0:2]), [1, 1])

    def testArraySetRangeListNonExactIndex(self):
        # Same as testArraySetRangeTupleNonExactIndex, but the LIST loop's
        # own PyIndex_Check fallback sub-branch.
        ja = JArray(JInt)(2)
        ja[0:2] = [1, True]
        self.assertEqual(list(ja[0:2]), [1, 1])

    @common.requireNumpy
    def testArraySetRangeBufferEmptySliceDeclines(self):
        # tryFastBufferPush's length <= 0 decline branch.
        ja = JArray(JInt)(3)
        ja[0:0] = np.array([], dtype=np.int32)
        self.assertEqual(list(ja), [0, 0, 0])

    @common.requireNumpy
    def testArraySetRangeBufferNdimMismatchDeclines(self):
        # tryFastBufferPush's view.ndim != 1 decline branch (jp_convert.cpp)
        # -- a 2D buffer source into a flat slice assignment. Not dead: the
        # general fallback path (JPIntType::setArrayRange) has its own
        # redundant ndim check that raises the actual TypeError, but
        # tryFastBufferPush's own decline is what routes it there.
        ja = JArray(JInt)(3)
        with self.assertRaisesRegex(TypeError, "incorrect"):
            ja[0:3] = np.zeros((3, 1), dtype=np.int32)

    @common.requireNumpy
    def testArraySetRangeBufferUnrecognizedFormatDeclines(self):
        # tryFastBufferPush's classifyBufferSource() decline branch -- a
        # complex128 source's buffer format ('Zd') isn't recognized by
        # classifyBufferSource, so this declines the bulk path and falls
        # to the general getConverter() fallback, which doesn't recognize
        # it either and raises.
        ja = JArray(JInt)(3)
        with self.assertRaises(ValueError):
            ja[0:3] = np.array([1, 2, 3], dtype=np.complex128)

    @common.requireNumpy
    def testArraySetRangeBufferFallback(self):
        # A negative-stride (reversed) buffer source can't be handed to the
        # bulk tryFastBufferPush path (classifyBufferSource requires a
        # positive stride), so this exercises the older per-element
        # getConverter()/Convert<T> fallback in setArrayRange instead.
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype=np.int64)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackInt16Source(self):
        # getConverter's int16_t source case (from[0] == 'h', non-swapped)
        # -> 'i' target.
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype=np.int16)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackInt16SourceSwapped(self):
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype='>i2')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint16Source(self):
        # getConverter's uint16_t source case (from[0] == 'H', non-swapped)
        # -> 'i' target.
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype=np.uint16)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint16SourceSwapped(self):
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype='>u2')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint32Source(self):
        # getConverter's uint32_t source case (from[0] in 'I','L',
        # non-swapped) -> 'i' target.
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype=np.uint32)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint32SourceSwapped(self):
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype='>u4')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint64Source(self):
        # getConverter's uint64_t source case (from[0] == 'Q',
        # non-swapped) -> 'i' target.
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype=np.uint64)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint64SourceSwapped(self):
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype='>u8')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat32Source(self):
        # getConverter's float source case (from[0] == 'f', non-swapped)
        # -> 'i' target.
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype=np.float32)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat32SourceSwapped(self):
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype='>f4')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat64SourceSwapped(self):
        # getConverter's double source case (from[0] == 'd', swapped) ->
        # 'i' target. Non-swapped 'i' is already covered elsewhere.
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype='>f8')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat16SourceSwapped(self):
        # getConverter's float16 source case (from[0] == 'e', swapped) ->
        # 'i' target.
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype='>f2')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    def testArraySetRangeBufferFallbackIntpSource(self):
        # getConverter's Py_ssize_t source case (from[0] == 'n') -> 'i'
        # target.
        ja = JArray(JInt)(3)
        mv = memoryview(bytearray(24)).cast('n')
        mv[0], mv[1], mv[2] = 1, 2, 3
        ja[0:3] = mv[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    def testArraySetRangeBufferFallbackUintpSource(self):
        # getConverter's size_t source case (from[0] == 'N') -> 'i'
        # target.
        ja = JArray(JInt)(3)
        mv = memoryview(bytearray(24)).cast('N')
        mv[0], mv[1], mv[2] = 1, 2, 3
        ja[0:3] = mv[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackInt8Source(self):
        # getConverter's int8_t source case (from[0] in '?','c','b') ->
        # 'i' target.
        ja = JArray(JInt)(3)
        a = np.array([1, 2, 3], dtype=np.int8)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat16Subnormal(self):
        # jp_convert.cpp's Half<Convert<float>::toI>::convert -- a
        # subnormal half-float (exp==0, frac!=0) truncated to int is 0
        # regardless of which nonzero subnormal magnitude.
        bits = np.array([1, 0x0200, 0x03ff], dtype=np.uint16)
        a = bits.view(np.float16)
        ja = JArray(JInt)(3)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [0, 0, 0])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat16InfNan(self):
        # jp_convert.cpp's Half<Convert<float>::toI>::convert -- the "to
        # infinity and beyond" branch (exp==31): +inf/-inf/nan all cast to
        # jint the same way float infinity/nan already do (INT_MIN).
        bits = np.array([0x7C00, 0xFC00, 0x7E00], dtype=np.uint16)
        a = bits.view(np.float16)
        ja = JArray(JInt)(3)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [-2147483648, -2147483648, -2147483648])

    @unittest.skipUnless(sys.version_info >= (3, 12),
            "PEP 688 __buffer__ needed to force a buffer export that "
            "declines PyBUF_STRIDES|PyBUF_FORMAT -- see "
            "test_arrayMultiDimBuffer.py's own copy of this technique "
            "for the full rationale.")
    def testArraySetRangeBufferDeclinesStrides(self):
        # tryFastBufferPush (jp_convert.cpp): PyObject_CheckBuffer passes
        # but opening the buffer with PyBUF_STRIDES|PyBUF_FORMAT fails --
        # must decline (fall through to the general per-element path)
        # rather than propagate the BufferError.
        class NoStrides:
            def __buffer__(self, flags):
                raise BufferError("declines strides on purpose")

            def __release_buffer__(self, view):
                pass

        ja = JArray(JInt)(3)
        with self.assertRaises(TypeError):
            ja[0:3] = NoStrides()

    def testArrayConversionFail(self):
        jarr = JArray(JInt)(VALUES)
        with self.assertRaises(TypeError):
            jarr[1] = 'a'

    def testArraySliceLength(self):
        jarr = JArray(JInt)(VALUES)
        jarr[1:2] = [1]
        with self.assertRaises(ValueError):
            jarr[1:2] = [1, 2, 3]

    def testArrayConversionInt(self):
        jarr = JArray(JInt)(VALUES)
        result = jarr[0: len(jarr)]
        self.assertElementsEqual(VALUES, result)
        result = jarr[2:10]
        self.assertElementsEqual(VALUES[2:10], result)

    def testArrayConversionError(self):
        jarr = JArray(JInt, 1)(10)
        with self.assertRaises(TypeError):
            jarr[1:2] = [dict()]
        # -1 is returned by python, if conversion fails also, ensure this works
        jarr[1:2] = [-1]

    def testArrayClone(self):
        array = JArray(JInt, 2)([[1, 2], [3, 4]])
        carray = array.clone()
        # Verify the first dimension is cloned
        self.assertFalse(array.equals(carray))
        # Copy is shallow
        self.assertTrue(array[0].equals(carray[0]))

    def testArrayGetSlice(self):
        contents = VALUES
        array = JArray(JInt)(contents)
        self.assertEqual(list(array[1:]), contents[1:])
        self.assertEqual(list(array[:-1]), contents[:-1])
        self.assertEqual(list(array[1:-1]), contents[1:-1])

    def testArraySetSlice(self):
        contents = [1, 2, 3, 4]
        array = JArray(JInt)(contents)
        array[1:] = [5, 6, 7]
        contents[1:] = [5, 6, 7]
        self.assertEqual(list(array[:]), contents[:])
        array[:-1] = [8, 9, 10]
        contents[:-1] = [8, 9, 10]
        self.assertEqual(list(array[:]), contents[:])

    def testArrayGetSliceStep(self):
        contents = VALUES
        array = JArray(JInt)(contents)
        self.assertEqual(list(array[::2]), contents[::2])
        self.assertEqual(list(array[::3]), contents[::3])
        self.assertEqual(list(array[::4]), contents[::4])
        self.assertEqual(list(array[::5]), contents[::5])
        self.assertEqual(list(array[::6]), contents[::6])
        self.assertEqual(list(array[::7]), contents[::7])
        self.assertEqual(list(array[::8]), contents[::8])
        self.assertEqual(list(array[1::3]), contents[1::3])
        self.assertEqual(list(array[1:-2:3]), contents[1:-2:3])

    def testArraySliceStepNeg(self):
        contents = VALUES
        array = JArray(JInt)(contents)
        self.assertEqual(list(array[::-1]), contents[::-1])
        self.assertEqual(list(array[::-2]), contents[::-2])
        self.assertEqual(list(array[::-3]), contents[::-3])
        self.assertEqual(list(array[::-4]), contents[::-4])
        self.assertEqual(list(array[::-5]), contents[::-5])
        self.assertEqual(list(array[::-6]), contents[::-6])
        self.assertEqual(list(array[2::-3]), contents[2::-3])
        self.assertEqual(list(array[-2::-3]), contents[-2::-3])

    def testArraySetArraySliceStep(self):
        contents = [1, 2, 3, 4, 5, 6]
        array = JArray(JInt)(contents)
        array[::2] = [5, 6, 7]
        contents[::2] = [5, 6, 7]
        self.assertEqual(list(array[:]), contents[:])

    def testArrayEquals(self):
        contents = VALUES
        array = JArray(JInt)(contents)
        array2 = JArray(JInt)(contents)
        self.assertEqual(array, array)
        self.assertNotEqual(array, array2)

    def testArrayIter(self):
        contents = VALUES
        array = JArray(JInt)(contents)
        contents2 = [i for i in array]
        self.assertEqual(contents, contents2)

    def testArrayGetOutOfBounds(self):
        contents = [1, 2, 3, 4]
        array = JArray(JInt)(contents)
        with self.assertRaises(IndexError):
            array[5]
        self.assertEqual(array[-1], contents[-1])
        self.assertEqual(array[-4], contents[-4])
        with self.assertRaises(IndexError):
            array[-5]

    def testArraySetOutOfBounds(self):
        contents = [1, 2, 3, 4]
        array = JArray(JInt)(contents)
        with self.assertRaises(IndexError):
            array[5] = 1
        array[-1] = 5
        contents[-1] = 5
        array[-4] = 6
        contents[-4] = 6
        self.assertEqual(list(array[:]), contents)
        with self.assertRaises(IndexError):
            array[-5] = 1

    def testArraySliceCast(self):
        JA = JArray(JInt)
        ja = JA(VALUES)
        ja2 = ja[::2]
        jo = jpype.JObject(ja2, jpype.JObject)
        ja3 = jpype.JObject(jo, JA)
        self.assertEqual(type(jo), jpype.JClass("java.lang.Object"))
        self.assertEqual(type(ja2), JA)
        self.assertEqual(type(ja3), JA)
        self.assertEqual(list(ja2), list(ja3))

    def testArrayReverse(self):
        n = list(VALUES)
        ja = JArray(JInt)(n)
        a = [i for i in reversed(ja)]
        n = [i for i in reversed(n)]
        self.assertEqual(a, n)

    def testArrayHash(self):
        ja = JArray(JInt)([1, 2, 3])
        self.assertIsInstance(hash(ja), int)

    @common.requireNumpy
    def testArrayBufferDims(self):
        ja = JArray(JInt)(5)
        a = np.zeros((5, 2))
        with self.assertRaisesRegex(TypeError, "incorrect"):
            ja[:] = a

    def testArrayBadItem(self):
        class q(object):
            def __int__(self):
                raise SystemError("nope")

            def __index__(self):
                raise SystemError("nope")
        ja = JArray(JInt)(5)
        a = [1, -1, q(), 3, 4]
        with self.assertRaisesRegex(SystemError, "nope"):
            ja[:] = a

    def testArrayBadDims(self):
        class q(bytes):
            # Lie about our length
            def __len__(self):
                return 5
        a = q([1, 2, 3])
        ja = JArray(JInt)(5)
        with self.assertRaisesRegex(ValueError, "Slice"):
            ja[:] = [1, 2, 3]
        with self.assertRaisesRegex(ValueError, "mismatch"):
            ja[:] = a
