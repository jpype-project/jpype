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


class JDoubleTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.value = 1.0 + 1.0 / 65536
        self.cls = JClass("jpype.common.Fixture")
        self.fixture = self.cls()

    def compareDoubleEqual(self, x, y, msg=None):
        if x == y:
            return
        if x < 0:
            x = -x
        if y < 0:
            y = -y
        a = (x + y) / 2
        b = (x - y)
        if b < 0:
            b = -b
        if b < a * 1e-14:
            return
        msg = self._formatMessage(msg, '%s == %s' % (safe_repr(first),
                                                     safe_repr(second)))
        raise self.failureException(msg)

    @common.requireInstrumentation
    def testJPNumberFloat_int(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_int")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(jd)
        int(jd)

    @common.requireInstrumentation
    def testJPNumberFloat_float(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_float")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(jd)
        float(jd)

    @common.requireInstrumentation
    def testJPNumberFloat_str(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jd)
        str(jd)

    @common.requireInstrumentation
    def testJPNumberFloat_repr(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_repr")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(jd)
        repr(jd)

    @common.requireInstrumentation
    def testJPNumberFloat_compare(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_compare")
        with self.assertRaisesRegex(SystemError, "fault"):
            jd == 1
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            jd == 1
        jd == 1

    @common.requireInstrumentation
    def testJPNumberFloat_hash(self):
        jd = JDouble(1)
        _jpype.fault("PyJPNumberFloat_hash")
        with self.assertRaises(SystemError):
            hash(jd)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            hash(jd)
        hash(jd)

    @common.requireInstrumentation
    def testFault(self):
        _jpype.fault("JPDoubleType::findJavaConversion")
        with self.assertRaises(SystemError):
            JDouble(1.0)

    @common.requireInstrumentation
    def testConversionFault(self):
        _jpype.fault("JPDoubleType::findJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JDouble._canConvertToJava(object())

    @common.requireInstrumentation
    def testArrayFault(self):
        ja = JArray(JDouble)(5)
        _jpype.fault("JPJavaFrame::NewDoubleArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JDouble)(1)
        _jpype.fault("JPJavaFrame::SetDoubleArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 0
        _jpype.fault("JPJavaFrame::GetDoubleArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])
        _jpype.fault("JPJavaFrame::GetDoubleArrayElements")
        # Special case, only BufferError is allowed from getBuffer
        with self.assertRaises(BufferError):
            memoryview(ja[0:3])
        _jpype.fault("JPJavaFrame::ReleaseDoubleArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:3] = bytes([1, 2, 3])
        _jpype.fault("JPJavaFrame::ReleaseDoubleArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            jpype.JObject(ja[::2], jpype.JObject)
        _jpype.fault("JPJavaFrame::ReleaseDoubleArrayElements")

        def f():
            # Special case no fault is allowed
            memoryview(ja[0:3])
        f()
        _jpype.fault("JPDoubleType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0, 0]

    def testFromJIntWiden(self):
        self.assertEqual(JDouble(JByte(123)), 123)
        self.assertEqual(JDouble(JShort(12345)), 12345)
        self.assertEqual(JDouble(JInt(123456789)), 123456789)
        self.assertEqual(JDouble(JLong(123456789)), 123456789)

    def testFromJFloatWiden(self):
        self.assertEqual(JDouble(JFloat(12345678)), 12345678)

    def testFromNone(self):
        with self.assertRaises(TypeError):
            JDouble(None)
        self.assertEqual(JDouble._canConvertToJava(None), "none")

    def testFromJDouble(self):
        self.assertEqual(JDouble(JDouble(1.2345)), 1.2345)

    def testUnBox(self):
        self.assertEqual(JDouble(java.lang.Double(1.2345)), 1.2345)

    def testFromFloat(self):
        self.assertEqual(JDouble(1.2345), 1.2345)
        self.assertEqual(JDouble._canConvertToJava(1.2345), "exact")

    def testFromLong(self):
        self.assertEqual(JDouble(12345), 12345)
        self.assertEqual(JDouble._canConvertToJava(12345), "implicit")

    def testFromObject(self):
        with self.assertRaises(TypeError):
            JDouble(object())
        with self.assertRaises(TypeError):
            JDouble(JObject())
        with self.assertRaises(TypeError):
            JDouble(JString("A"))
        self.assertEqual(JDouble._canConvertToJava(object()), "none")
        ja = JArray(JDouble)(5)
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.common.Fixture")
        with self.assertRaises(TypeError):
            jf.static_double_field = object()
        with self.assertRaises(TypeError):
            jf().double_field = object()

    def testCallDoubleFromNone(self):
        with self.assertRaises(TypeError):
            self.fixture.callDouble(None)
        with self.assertRaises(TypeError):
            self.fixture.static_double_field = None
        with self.assertRaises(TypeError):
            self.fixture.double_field = None

    def testThrow(self):
        with self.assertRaises(JException):
            self.fixture.throwDouble()
        with self.assertRaises(JException):
            self.cls.throwStaticDouble()
        with self.assertRaises(JException):
            self.fixture.throwStaticDouble()

    def checkType(self, q):
        #  Check field
        self.fixture.double_field = q
        self.assertEqual(self.fixture.double_field, q)
        self.assertEqual(self.fixture.getDouble(), q)
        #  Check static field
        self.cls.static_double_field = q
        self.assertEqual(self.fixture.static_double_field, q)
        self.assertEqual(self.fixture.getStaticDouble(), q)
        self.assertEqual(self.cls.getStaticDouble(), q)
        #  Check call
        self.assertEqual(self.fixture.callDouble(q), q)
        self.assertEqual(self.cls.callStaticDouble(q), q)

    def testCheckInt(self):
        self.checkType(1)

    def testCheckFloat(self):
        self.checkType(2.0)

    def testCheckRange(self):
        self.checkType(float(1e340))
        self.checkType(float(-1e340))

    def testCheckNaN(self):
        import math
        nan = float("nan")
        self.assertTrue(math.isnan(self.fixture.callDouble(nan)))
        self.fixture.static_double_field = nan
        self.assertTrue(math.isnan(self.fixture.static_double_field))
        self.fixture.double_field = nan
        self.assertTrue(math.isnan(self.fixture.double_field))

    def testCheckInf(self):
        import math
        inf = float("inf")
        self.assertTrue(math.isinf(self.fixture.callDouble(inf)))
        self.fixture.static_double_field = inf
        self.assertTrue(math.isinf(self.fixture.static_double_field))
        self.fixture.double_field = inf
        self.assertTrue(math.isinf(self.fixture.double_field))

    def testCheckBool(self):
        self.checkType(True)
        self.checkType(False)

    def testCheckJBoolean(self):
        # FIXME fails
        # self.checkType(JBoolean(True))
        # self.checkType(JBoolean(False))
        pass

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
        with self.useEqualityFunc(self.compareDoubleEqual):
            self.checkType(JLong(-2**63 + 1))
            self.checkType(JLong(2**63 - 1))

    def testCheckJFloat(self):
        self.checkType(JFloat(1.515313))

    def testCheckDouble(self):
        self.checkType(JDouble(11.85193))

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
        self.checkType(np.random.randint(-2**31, 2**31 - 1, dtype=np.int32))
        self.checkType(np.random.randint(0, 2**32 - 1, dtype=np.uint32))
        self.checkType(np.uint32(0))
        self.checkType(np.uint32(2**32 - 1))
        self.checkType(np.int32(-2**31))
        self.checkType(np.int32(2**31 - 1))

    @common.requireNumpy
    def testCheckNumpyInt64(self):
        self.checkType(np.random.randint(-2**63, 2**63 - 1, dtype=np.int64))
        self.checkType(
            np.uint64(np.random.randint(0, 2**64 - 1, dtype=np.uint64)))
        self.checkType(np.uint64(0))
        self.checkType(np.uint64(2**64 - 1))
        self.checkType(np.int64(-2**63))
        self.checkType(np.int64(2**63 - 1))

    @common.requireNumpy
    def testCheckNumpyFloat32(self):
        self.checkType(np.float32(np.random.rand()))

    @common.requireNumpy
    def testCheckNumpyFloat64(self):
        self.checkType(np.float64(np.random.rand()))

    def testArrayConversionDouble(self):
        VALUES = [float(random.random()) for i in range(10)]
        jarr = JArray(JDouble)(VALUES)
        self.assertElementsAlmostEqual(VALUES, jarr)
        result = jarr[:]
        self.assertElementsAlmostEqual(VALUES, result)
        result = jarr[2:10]
        self.assertEqual(len(VALUES[2:10]), len(result))
        self.assertElementsAlmostEqual(VALUES[2:10], result)

        # empty slice
        result = jarr[-1:3]
        expected = VALUES[-1:3]
        self.assertElementsAlmostEqual(expected, result)

        result = jarr[3:-2]
        expected = VALUES[3:-2]
        self.assertElementsEqual(expected, result)

    @common.requireNumpy
    def testArraySetFromNPDouble(self):
        a = np.random.random(100).astype(np.float64)
        jarr = JArray(JDouble)(100)
        jarr[:] = a
        self.assertElementsAlmostEqual(a, jarr)

    @common.requireNumpy
    def testArrayInitFromNPFloat(self):
        a = np.random.random(100).astype(np.float)
        jarr = JArray(JDouble)(a)
        self.assertElementsAlmostEqual(a, jarr)

    @common.requireNumpy
    def testArrayInitFromNPFloat32(self):
        a = np.random.random(100).astype(np.float32)
        jarr = JArray(JDouble)(a)
        self.assertElementsAlmostEqual(a, jarr)

    @common.requireNumpy
    def testArrayInitFromNPFloat64(self):
        a = np.random.random(100).astype(np.float64)
        jarr = JArray(JDouble)(a)
        self.assertElementsAlmostEqual(a, jarr)

    def testArraySetRange(self):
        ja = JArray(JDouble)(3)
        ja[0:1] = [123]
        self.assertEqual(ja[0], 123)
        ja[0:1] = [-1]
        self.assertEqual(ja[0], -1)
        ja[0:1] = [java.lang.Double(321)]
        self.assertEqual(ja[0], 321)
        with self.assertRaises(TypeError):
            ja[0:1] = [object()]

    def testArrayHash(self):
        ja = JArray(JDouble)([1, 2, 3])
        self.assertIsInstance(hash(ja), int)

    @common.requireNumpy
    def testArrayBufferDims(self):
        ja = JArray(JDouble)(5)
        a = np.zeros((5, 2))
        with self.assertRaisesRegex(TypeError, "incorrect"):
            ja[:] = a

    def testArrayBadItem(self):
        class q(object):
            def __float__(self):
                raise SystemError("nope")
        ja = JArray(JDouble)(5)
        a = [1, -1, q(), 3, 4]
        with self.assertRaisesRegex(SystemError, "nope"):
            ja[:] = a

    def testArrayBadDims(self):
        class q(bytes):
            # Lie about our length
            def __len__(self):
                return 5
        a = q([1, 2, 3])
        ja = JArray(JDouble)(5)
        with self.assertRaisesRegex(ValueError, "Slice"):
            ja[:] = [1, 2, 3]
        with self.assertRaisesRegex(ValueError, "mismatch"):
            ja[:] = a

    def testCastBoolean(self):
        self.assertEqual(JDouble._canConvertToJava(JBoolean(True)), "none")
