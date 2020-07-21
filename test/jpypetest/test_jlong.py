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

VALUES = [random.randint(-2**63, 2**63 - 1) for i in range(10)]


class JLongTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.cls = JClass("jpype.common.Fixture")
        self.fixture = self.cls()

    @common.requireInstrumentation
    def testJPNumberLong_int(self):
        jd = JLong(1)
        _jpype.fault("PyJPNumberLong_int")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(jd)
        int(jd)

    @common.requireInstrumentation
    def testJPNumberLong_float(self):
        jd = JLong(1)
        _jpype.fault("PyJPNumberLong_float")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(jd)
        float(jd)

    @common.requireInstrumentation
    def testJPNumberLong_str(self):
        jd = JLong(1)
        _jpype.fault("PyJPNumberLong_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jd)
        str(jd)

    @common.requireInstrumentation
    def testJPNumberLong_repr(self):
        jd = JLong(1)
        _jpype.fault("PyJPNumberLong_repr")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(jd)
        repr(jd)

    @common.requireInstrumentation
    def testJPNumberLong_compare(self):
        jd = JLong(1)
        _jpype.fault("PyJPNumberLong_compare")
        with self.assertRaisesRegex(SystemError, "fault"):
            jd == 1
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            jd == 1
        jd == 1

    @common.requireInstrumentation
    def testJPNumberLong_hash(self):
        jd = JLong(1)
        _jpype.fault("PyJPNumberLong_hash")
        with self.assertRaises(SystemError):
            hash(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            hash(jd)
        hash(jd)

    @common.requireInstrumentation
    def testFault(self):
        _jpype.fault("JPLongType::findJavaConversion")
        with self.assertRaises(SystemError):
            JLong(1.0)

    @common.requireInstrumentation
    def testConversionFault(self):
        _jpype.fault("JPLongType::findJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JLong._canConvertToJava(object())

    @common.requireInstrumentation
    def testArrayFault(self):
        ja = JArray(JLong)(5)
        _jpype.fault("JPJavaFrame::NewLongArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JLong)(1)
        _jpype.fault("JPJavaFrame::SetLongArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 0
        _jpype.fault("JPJavaFrame::GetLongArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])
        _jpype.fault("JPJavaFrame::GetLongArrayElements")
        # Special case, only BufferError is allowed from getBuffer
        with self.assertRaises(BufferError):
            memoryview(ja[0:3])
        _jpype.fault("JPJavaFrame::ReleaseLongArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:3] = bytes([1, 2, 3])
        _jpype.fault("JPJavaFrame::ReleaseLongArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            jpype.JObject(ja[::2], jpype.JObject)
        _jpype.fault("JPJavaFrame::ReleaseLongArrayElements")

        def f():
            # Special case no fault is allowed
            memoryview(ja[0:3])
        f()
        _jpype.fault("JPLongType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0, 0]

    def testFromJLongWiden(self):
        self.assertEqual(JLong(JByte(123)), 123)
        self.assertEqual(JLong(JShort(12345)), 12345)
        self.assertEqual(JLong(JInt(12345678)), 12345678)
        self.assertEqual(JLong(JLong(12345678)), 12345678)

    def testFromJLongWiden(self):
        self.assertEqual(JLong(JDouble(12345678)), 12345678)

    def testFromNone(self):
        with self.assertRaises(TypeError):
            JLong(None)
        self.assertEqual(JLong._canConvertToJava(None), "none")

    def testUnBox(self):
        self.assertEqual(JLong(java.lang.Double(1.2345)), 1)

    def testFromFloat(self):
        self.assertEqual(JLong._canConvertToJava(1.2345), "explicit")
        @jpype.JImplements("java.util.function.LongSupplier")
        class q(object):
            @jpype.JOverride
            def getAsLong(self):
                return 4.5  # this will hit explicit conversion
        self.assertEqual(JObject(q()).getAsLong(), 4)

    def testFromLong(self):
        self.assertEqual(JLong(12345), 12345)
        self.assertEqual(JLong._canConvertToJava(12345), "implicit")

    def testFromObject(self):
        with self.assertRaises(TypeError):
            JLong(object())
        with self.assertRaises(TypeError):
            JLong(JObject())
        with self.assertRaises(TypeError):
            JLong(JString("A"))
        self.assertEqual(JLong._canConvertToJava(object()), "none")
        ja = JArray(JLong)(5)
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.common.Fixture")
        with self.assertRaises(TypeError):
            jf.static_long_field = object()
        with self.assertRaises(TypeError):
            jf().long_field = object()

    def testCallFloatFromNone(self):
        with self.assertRaises(TypeError):
            self.fixture.callFloat(None)
        with self.assertRaises(TypeError):
            self.fixture.static_long_field = None
        with self.assertRaises(TypeError):
            self.fixture.long_field = None

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
        self.fixture.long_field = q
        self.assertEqual(self.fixture.long_field, q)
        self.assertEqual(self.fixture.getLong(), q)
        #  Check static field
        self.cls.static_long_field = q
        self.assertEqual(self.fixture.static_long_field, q)
        self.assertEqual(self.fixture.getStaticLong(), q)
        self.assertEqual(self.cls.getStaticLong(), q)
        #  Check call
        self.assertEqual(self.fixture.callLong(q), q)
        self.assertEqual(self.cls.callStaticLong(q), q)

    def checkTypeFail(self, q, exc=TypeError):
        with self.assertRaises(exc):
            self.fixture.long_field = q
        with self.assertRaises(exc):
            self.fixture.callLong(q)
        with self.assertRaises(exc):
            self.fixture.callStaticLong(q)

    def testCastFloat(self):
        self.fixture.long_field = JLong(6.0)
        self.assertEqual(self.fixture.long_field, 6)

    def testCheckInt(self):
        self.checkType(1)

    def testCheckFloat(self):
        self.checkTypeFail(2.0)

    def testCheckRange(self):
        self.checkType(2**63 - 1)
        self.checkType(-2**63)
        self.checkTypeFail(2**63, exc=OverflowError)
        self.checkTypeFail(-2**63 - 1, exc=OverflowError)

    def testExplicitRange(self):
        # These will not overflow as they are explicit casts
        self.assertEqual(JLong(2**64), 0)
        self.assertEqual(JLong(-2**64), 0)

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

    def testCheckJLong(self):
        self.checkType(JLong(-2**31 + 1))
        self.checkType(JLong(2**31 - 1))

    def testCheckJLong(self):
        self.checkType(JLong(-2**63 + 1))
        self.checkType(JLong(2**63 - 1))

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
        self.checkType(np.uint32(2**32 - 1))
        self.checkType(np.int32(-2**31))
        self.checkType(np.int32(2**31 - 1))

    @common.requireNumpy
    def testCheckNumpyInt64(self):
        #self.checkTypeFail(np.random.randint(-2**63,2**63-1, dtype=np.int64))
        # FIXME OverflowError
        #self.checkType(np.uint64(np.random.randint(0,2**64-1, dtype=np.uint64)))
        # FIXME OverflowError
        # self.checkType(np.uint64(2**64-1))
        self.checkType(np.int64(-2**63))
        self.checkType(np.int64(2**63 - 1))

    @common.requireNumpy
    def testCheckNumpyFloat32(self):
        self.checkTypeFail(np.float32(np.random.rand()))

    @common.requireNumpy
    def testCheckNumpyFloat64(self):
        self.checkTypeFail(np.float64(np.random.rand()))

    def checkArrayType(self, a, expected):
        # Check init
        ja = JArray(JLong)(a)
        self.assertElementsEqual(ja, expected)
        ja = JArray(JLong)(len(a))
        ja[:] = a
        self.assertElementsEqual(ja, expected)
        return ja

    def checkArrayTypeFail(self, a):
        # Check init
        ja = JArray(JLong)(a)
        ja = JArray(JLong)(len(a))
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
        a = np.random.randint(-2**31, 2**31 - 1, size=100, dtype=np.int)
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
        self.checkArrayType(a, a.astype(np.int64))

    @common.requireNumpy
    def testArrayInitFromNPFloat32(self):
        a = np.random.random(100).astype(np.float32)
        self.checkArrayType(a, a.astype(np.int64))

    @common.requireNumpy
    def testArrayInitFromNPFloat64(self):
        a = np.random.random(100).astype(np.float64)
        self.checkArrayType(a, a.astype(np.int64))

    def testArraySetRange(self):
        ja = JArray(JLong)(3)
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

    def testArrayConversionFail(self):
        jarr = JArray(JLong)(VALUES)
        with self.assertRaises(TypeError):
            jarr[1] = 'a'

    def testArraySliceLength(self):
        jarr = JArray(JLong)(VALUES)
        jarr[1:2] = [1]
        with self.assertRaises(ValueError):
            jarr[1:2] = [1, 2, 3]

    def testArrayConversionInt(self):
        jarr = JArray(JLong)(VALUES)
        result = jarr[0: len(jarr)]
        self.assertElementsEqual(VALUES, result)
        result = jarr[2:10]
        self.assertElementsEqual(VALUES[2:10], result)

    def testArrayConversionError(self):
        jarr = JArray(JLong, 1)(10)
        with self.assertRaises(TypeError):
            jarr[1:2] = [dict()]
        # -1 is returned by python, if conversion fails also, ensure this works
        jarr[1:2] = [-1]

    def testArrayClone(self):
        array = JArray(JLong, 2)([[1, 2], [3, 4]])
        carray = array.clone()
        # Verify the first dimension is cloned
        self.assertFalse(array.equals(carray))
        # Copy is shallow
        self.assertTrue(array[0].equals(carray[0]))

    def testArrayGetSlice(self):
        contents = VALUES
        array = JArray(JLong)(contents)
        self.assertEqual(list(array[1:]), contents[1:])
        self.assertEqual(list(array[:-1]), contents[:-1])
        self.assertEqual(list(array[1:-1]), contents[1:-1])

    def testArraySetSlice(self):
        contents = [1, 2, 3, 4]
        array = JArray(JLong)(contents)
        array[1:] = [5, 6, 7]
        contents[1:] = [5, 6, 7]
        self.assertEqual(list(array[:]), contents[:])
        array[:-1] = [8, 9, 10]
        contents[:-1] = [8, 9, 10]
        self.assertEqual(list(array[:]), contents[:])

    def testArrayGetSliceStep(self):
        contents = VALUES
        array = JArray(JLong)(contents)
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
        array = JArray(JLong)(contents)
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
        array = JArray(JLong)(contents)
        array[::2] = [5, 6, 7]
        contents[::2] = [5, 6, 7]
        self.assertEqual(list(array[:]), contents[:])

    def testArrayEquals(self):
        contents = VALUES
        array = JArray(JLong)(contents)
        array2 = JArray(JLong)(contents)
        self.assertEqual(array, array)
        self.assertNotEqual(array, array2)

    def testArrayIter(self):
        contents = VALUES
        array = JArray(JLong)(contents)
        contents2 = [i for i in array]
        self.assertEqual(contents, contents2)

    def testArrayGetOutOfBounds(self):
        contents = [1, 2, 3, 4]
        array = JArray(JLong)(contents)
        with self.assertRaises(IndexError):
            array[5]
        self.assertEqual(array[-1], contents[-1])
        self.assertEqual(array[-4], contents[-4])
        with self.assertRaises(IndexError):
            array[-5]

    def testArraySetOutOfBounds(self):
        contents = [1, 2, 3, 4]
        array = JArray(JLong)(contents)
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
        JA = JArray(JLong)
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
        ja = JArray(JLong)(n)
        a = [i for i in reversed(ja)]
        n = [i for i in reversed(n)]
        self.assertEqual(a, n)

    def testArrayHash(self):
        ja = JArray(JLong)([1, 2, 3])
        self.assertIsInstance(hash(ja), int)

    @common.requireNumpy
    def testArrayBufferDims(self):
        ja = JArray(JLong)(5)
        a = np.zeros((5, 2))
        with self.assertRaisesRegex(TypeError, "incorrect"):
            ja[:] = a

    def testArrayBadItem(self):
        class q(object):
            def __int__(self):
                raise SystemError("nope")

            def __index__(self):
                raise SystemError("nope")
        ja = JArray(JLong)(5)
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
