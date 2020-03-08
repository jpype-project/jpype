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
        self.value = 1.0+1.0/65536
        self.cls = JClass("jpype.common.Fixture")
        self.fixture = self.cls()

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
        _jpype.fault("JPDoubleType::getJavaConversion")
        with self.assertRaises(SystemError):
            JDouble(1.0)

    @common.requireInstrumentation
    def testSetArrayRangeFault(self):
        ja = JArray(JDouble)(3)
        _jpype.fault("JPDoubleType::setArrayRange")
        with self.assertRaises(SystemError):
            ja[0:1] = [123]
        ja[0:1] = [123]

    @common.requireInstrumentation
    def testJPDoubleFaults(self):
        ja = JArray(JDouble)(5)  # lgtm [py/similar-function]
        _jpype.fault("JPDoubleType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0, 0]
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.common.Fixture")
        with self.assertRaises(TypeError):
            jf.static_double_field = object()
        with self.assertRaises(TypeError):
            jf().double_field = object()

    @common.requireInstrumentation
    def testJDoubleGetJavaConversion(self):
        _jpype.fault("JPDoubleType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JDouble._canConvertToJava(object())

    @common.requireInstrumentation
    def testJPJavaFrameDoubleArray(self):
        _jpype.fault("JPJavaFrame::NewDoubleArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JDouble)(1)
        ja = JArray(JDouble)(5)
        _jpype.fault("JPJavaFrame::SetDoubleArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 0
        _jpype.fault("JPJavaFrame::GetDoubleArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])
        _jpype.fault("JPJavaFrame::GetDoubleArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            memoryview(ja[0:3])
        _jpype.fault("JPJavaFrame::ReleaseDoubleArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:3] = bytes([1,2,3])
        _jpype.fault("JPJavaFrame::ReleaseDoubleArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            jpype.JObject(ja[::2], jpype.JObject)
        _jpype.fault("JPJavaFrame::ReleaseDoubleArrayElements")
        def f():
            # Special case no fault is allowed
            memoryview(ja[0:3])
        f()

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

    def testCallDoubleFromNone(self):
        with self.assertRaises(TypeError):
            self.fixture.callDouble(None)
        with self.assertRaises(TypeError):
            self.fixture.static_double_field = None
        with self.assertRaises(TypeError):
            self.fixture.double_field = None

    def checkType(self, q, approx = False):
        if approx:
            assertion = self.assertAlmostEqual
        else:
            assertion = self.assertEqual
        #  Check field
        self.fixture.double_field = q
        assertion(self.fixture.double_field, q)
        assertion(self.fixture.getDouble(), q)
        #  Check static field
        self.cls.static_double_field = q
        assertion(self.fixture.static_double_field, q)
        assertion(self.fixture.getStaticDouble(), q)
        assertion(self.cls.getStaticDouble(), q)
        #  Check call
        assertion(self.fixture.callDouble(q), q)
        assertion(self.cls.callStaticDouble(q), q)
        #  Check throw
        with self.assertRaises(JException):
            self.fixture.throwDouble()
        with self.assertRaises(JException):
            self.cls.throwStaticDouble()
        with self.assertRaises(JException):
            self.fixture.throwStaticDouble()

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
        #self.checkType(JBoolean(True))
        #self.checkType(JBoolean(False))
        pass

    def testCheckJChar(self):
        self.checkType(JChar("A"))

    def testCheckJByte(self):
        self.checkType(JByte(-128))
        self.checkType(JByte(127))

    def testCheckJShort(self):
        self.checkType(JShort(-2**15))
        self.checkType(JShort(2**15-1))

    def testCheckJInt(self):
        self.checkType(JInt(-2**31))
        self.checkType(JInt(2**31-1))

    def testCheckJLong(self):
        self.checkType(JLong(-2**63), approx=True)
        self.checkType(JLong(2**63-1), approx=True)

    def testCheckJFloat(self):
        self.checkType(JFloat(1.515313))

    def testCheckDouble(self):
        self.checkType(JDouble(11.85193))

    @common.requireNumpy
    def testCheckNumpyInt8(self):
        self.checkType(np.random.randint(-127,128, dtype=np.int8))
        self.checkType(np.random.randint(0,255, dtype=np.uint8))
        self.checkType(np.uint8(0))
        self.checkType(np.uint8(255))
        self.checkType(np.int8(-128))
        self.checkType(np.int8(127))

    @common.requireNumpy
    def testCheckNumpyInt16(self):
        self.checkType(np.random.randint(-2**15,2**15-1, dtype=np.int16))
        self.checkType(np.random.randint(0,2**16-1, dtype=np.uint16))
        self.checkType(np.uint16(0))
        self.checkType(np.uint16(2**16-1))
        self.checkType(np.int16(-2**15))
        self.checkType(np.int16(2**15-1))

    @common.requireNumpy
    def testCheckNumpyInt32(self):
        self.checkType(np.random.randint(-2**31,2**31-1, dtype=np.int32))
        self.checkType(np.random.randint(0,2**32-1, dtype=np.uint32))
        self.checkType(np.uint32(0))
        self.checkType(np.uint32(2**32-1))
        self.checkType(np.int32(-2**31))
        self.checkType(np.int32(2**31-1))

    @common.requireNumpy
    def testCheckNumpyInt64(self):
        self.checkType(np.random.randint(-2**63,2**63-1, dtype=np.int64))
        # FIXME OverflowError 
        #self.checkType(np.uint64(np.random.randint(0,2**64-1, dtype=np.uint64)))
        self.checkType(np.uint64(0))
        # FIXME OverflowError 
        #self.checkType(np.uint64(2**64-1))
        self.checkType(np.int64(-2**63))
        self.checkType(np.int64(2**63-1))

    @common.requireNumpy
    def testCheckNumpyFloat32(self):
        self.checkType(np.float32(np.random.rand()))

    @common.requireNumpy
    def testCheckNumpyFloat64(self):
        self.checkType(np.float64(np.random.rand()))

    def testJArrayConversionDouble(self):
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
    def testSetFromNPDoubleArray(self):
        a = np.random.random(100).astype(np.float64)
        jarr = JArray(JDouble)(100)
        jarr[:] = a
        self.assertElementsAlmostEqual(a, jarr)

    @common.requireNumpy
    def testInitFromNPDoubleArray(self):
        a = np.random.random(100).astype(np.float)
        jarr = JArray(JDouble)(a)
        self.assertElementsAlmostEqual(a, jarr)

    @common.requireNumpy
    def testInitFromNPDoubleArrayFloat32(self):
        a = np.random.random(100).astype(np.float32)
        jarr = JArray(JDouble)(a)
        self.assertElementsAlmostEqual(a, jarr)

    @common.requireNumpy
    def testInitFromNPDoubleArrayFloat64(self):
        a = np.random.random(100).astype(np.float64)
        jarr = JArray(JDouble)(a)
        self.assertElementsAlmostEqual(a, jarr)

    def testSetArrayRange(self):
        ja = JArray(JDouble)(3)
        ja[0:1] = [123]
        self.assertEqual(ja[0], 123)
        ja[0:1] = [-1]
        self.assertEqual(ja[0], -1)
        ja[0:1] = [java.lang.Double(321)]
        self.assertEqual(ja[0], 321)
        with self.assertRaises(TypeError):
            ja[0:1] = [object()]

