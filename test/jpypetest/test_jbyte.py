import _jpype
import jpype
from jpype.types import *
import sys
import logging
import time
import common


class JByteTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.fixture = jpype.JClass("jpype.common.Fixture")()

    @common.requireInstrumentation
    def testConversionFault(self):
        _jpype.fault("JPByteType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JByte._canConvertToJava(object())

    @common.requireInstrumentation
    def testArrayFaults(self):
        ja = JArray(JByte)(5)
        _jpype.fault("JPByteType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0, 0]
        _jpype.fault("JPJavaFrame::NewByteArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JByte)(1)
        _jpype.fault("JPJavaFrame::SetByteArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 0
        _jpype.fault("JPJavaFrame::GetByteArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])
        _jpype.fault("JPJavaFrame::GetByteArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            memoryview(ja[0:3])
        _jpype.fault("JPJavaFrame::ReleaseByteArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:3] = bytes([1, 2, 3])
        _jpype.fault("JPJavaFrame::ReleaseByteArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            jpype.JObject(ja[::2], jpype.JObject)
        _jpype.fault("JPJavaFrame::ReleaseByteArrayElements")

        def f():
            # Special case no fault is allowed
            memoryview(ja[0:3])
        f()

    def testByteFromInt(self):
        self.assertEqual(self.fixture.callByte(int(123)), 123)

    @common.requireNumpy
    def testByteFromNPInt(self):
        import numpy as np
        self.assertEqual(self.fixture.callByte(np.int(123)), 123)

    @common.requireNumpy
    def testByteFromNPInt8(self):
        import numpy as np
        self.assertEqual(self.fixture.callByte(np.int8(123)), 123)
        self.assertEqual(self.fixture.callByte(np.uint8(123)), 123)

    @common.requireNumpy
    def testByteFromNPInt16(self):
        import numpy as np
        self.assertEqual(self.fixture.callByte(np.int16(123)), 123)
        self.assertEqual(self.fixture.callByte(np.uint16(123)), 123)

    @common.requireNumpy
    def testByteFromNPInt32(self):
        import numpy as np
        self.assertEqual(self.fixture.callByte(np.int32(123)), 123)
        self.assertEqual(self.fixture.callByte(np.uint32(123)), 123)

    @common.requireNumpy
    def testByteFromNPInt64(self):
        import numpy as np
        self.assertEqual(self.fixture.callByte(np.int64(123)), 123)
        self.assertEqual(self.fixture.callByte(np.uint64(123)), 123)

    def testByteFromFloat(self):
        with self.assertRaises(TypeError):
            self.fixture.callByte(float(2))

    @common.requireNumpy
    def testByteFromNPFloat(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.fixture.callByte(np.float(2))

    @common.requireNumpy
    def testByteFromNPFloat32(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.fixture.callByte(np.float32(2))

    @common.requireNumpy
    def testByteFromNPFloat64(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.fixture.callByte(np.float64(2))

    def testByteRange(self):
        with self.assertRaises(OverflowError):
            self.fixture.callByte(int(1e10))
        with self.assertRaises(OverflowError):
            self.fixture.callByte(int(-1e10))

    def testByteFromNone(self):
        with self.assertRaises(TypeError):
            self.fixture.callByte(None)

    def testByteArrayAsString(self):
        t = JClass("jpype.array.TestArray")()
        v = t.getByteArray()
        self.assertEqual(str(v), 'avcd')

    def testByteArrayIntoVector(self):
        ba = jpype.JArray(jpype.JByte)(b'123')
        v = jpype.java.util.Vector(1)
        v.add(ba)
        self.assertEqual(len(v), 1)
        self.assertNotEqual(v[0], None)

    def testByteArraySimple(self):
        a = JArray(JByte)(2)
        a[1] = 2
        self.assertEqual(a[1], 2)

    def testJArrayConversionByte(self):
        expected = (0, 1, 2, 3)
        ByteBuffer = jpype.java.nio.ByteBuffer
        bb = ByteBuffer.allocate(4)
        buf = bb.array()
        for i in range(len(expected)):
            buf[i] = expected[i]
        for i in range(len(expected)):
            self.assertEqual(expected[i], buf[i])

    def testFromObject(self):
        ja = JArray(JByte)(5)
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.common.Fixture")
        with self.assertRaises(TypeError):
            jf.static_byte_field = object()
        with self.assertRaises(TypeError):
            jf().byte_field = object()

    def testArrayHash(self):
        ja = JArray(JByte)([1,2,3])
        self.assertIsInstance(hash(ja), int)
