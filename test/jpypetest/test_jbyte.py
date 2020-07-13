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
import _jpype
import jpype
from jpype.types import *
from jpype import java
import sys
import logging
import time
import common
try:
    import numpy as np
except ImportError:
    pass


class JByteTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.fixture = jpype.JClass("jpype.common.Fixture")()

    @common.requireInstrumentation
    def testConversionFault(self):
        _jpype.fault("JPByteType::findJavaConversion")
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
        with self.assertRaises(BufferError):
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

    def testExplicitRange(self):
        # These will not overflow as they are explicit casts
        self.assertEqual(JByte(2**8), 0)
        self.assertEqual(JByte(-2**8), 0)

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
        ja = JArray(JByte)([1, 2, 3])
        self.assertIsInstance(hash(ja), int)

    @common.requireNumpy
    def testArrayBufferDims(self):
        ja = JArray(JByte)(5)
        a = np.zeros((5, 2))
        with self.assertRaisesRegex(TypeError, "incorrect"):
            ja[:] = a

    def testArrayBadItem(self):
        class q(object):
            def __int__(self):
                raise SystemError("nope")

            def __index__(self):
                raise SystemError("nope")
        ja = JArray(JByte)(5)
        a = [1, -1, q(), 3, 4]
        with self.assertRaisesRegex(SystemError, "nope"):
            ja[:] = a

    def testArrayBadDims(self):
        class q(bytes):
            # Lie about our length
            def __len__(self):
                return 5
        a = q([1, 2, 3])
        ja = JArray(JByte)(5)
        with self.assertRaisesRegex(ValueError, "Slice"):
            ja[:] = [1, 2, 3]
        with self.assertRaisesRegex(ValueError, "mismatch"):
            ja[:] = a

    def testArraySetRange(self):
        ja = JArray(JByte)(3)
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
