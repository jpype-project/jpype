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
import _jpype
from jpype.types import *
from jpype import java
import common
try:
    import numpy as np
except ImportError:
    pass


class JBufferTestCase(common.JPypeTestCase):
    """ Test for direct buffers.
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.fixture = JClass('jpype.common.Fixture')()
        self.bo = JClass("java.nio.ByteOrder")
        self.cls = JClass("java.nio.ByteBuffer")

    def testIsDirect(self):
        obj = self.cls.allocate(10)
        with self.assertRaises(BufferError):
            memoryview(obj)
        obj = self.cls.allocateDirect(10)
        memoryview(obj)

    def testReadOnly(self):
        obj = self.cls.allocateDirect(10).asReadOnlyBuffer()
        mv = memoryview(obj)
        self.assertTrue(mv.readonly)

    def testOrder(self):
        obj = self.cls.allocateDirect(10)
        obj.order(self.bo.LITTLE_ENDIAN)
        v1 = obj.asIntBuffer()
        self.assertEqual(memoryview(v1).format, '<i')
        obj.order(self.bo.BIG_ENDIAN)
        v1 = obj.asIntBuffer()
        self.assertEqual(memoryview(v1).format, '>i')

    def testMemoryViewByte(self):
        obj = self.cls.allocateDirect(12)
        mv = memoryview(obj)
        self.assertEqual(mv.itemsize, 1)
        self.assertEqual(mv.strides, (1,))
        self.assertEqual(mv.suboffsets, tuple())
        self.assertFalse(mv.readonly)
        self.assertEqual(mv.shape, (12,))
        self.assertEqual(mv.format, ">b")

    def testMemoryViewChar(self):
        obj = self.cls.allocateDirect(12).asCharBuffer()
        mv = memoryview(obj)
        self.assertEqual(mv.itemsize, 2)
        self.assertEqual(mv.strides, (2,))
        self.assertEqual(mv.suboffsets, tuple())
        self.assertFalse(mv.readonly)
        self.assertEqual(mv.shape, (6,))
        self.assertEqual(mv.format, ">H")

    def testMemoryViewShort(self):
        obj = self.cls.allocateDirect(12).asShortBuffer()
        mv = memoryview(obj)
        self.assertEqual(mv.itemsize, 2)
        self.assertEqual(mv.strides, (2,))
        self.assertEqual(mv.suboffsets, tuple())
        self.assertFalse(mv.readonly)
        self.assertEqual(mv.shape, (6,))
        self.assertEqual(mv.format, ">h")

    def testMemoryViewInt(self):
        obj = self.cls.allocateDirect(12).asIntBuffer()
        mv = memoryview(obj)
        self.assertEqual(mv.itemsize, 4)
        self.assertEqual(mv.strides, (4,))
        self.assertEqual(mv.suboffsets, tuple())
        self.assertFalse(mv.readonly)
        self.assertEqual(mv.shape, (3,))
        self.assertEqual(mv.format, ">i")

    def testMemoryViewLong(self):
        obj = self.cls.allocateDirect(24).asLongBuffer()
        mv = memoryview(obj)
        self.assertEqual(mv.itemsize, 8)
        self.assertEqual(mv.strides, (8,))
        self.assertEqual(mv.suboffsets, tuple())
        self.assertFalse(mv.readonly)
        self.assertEqual(mv.shape, (3,))
        self.assertEqual(mv.format, ">q")

    def testMemoryViewFloat(self):
        obj = self.cls.allocateDirect(24).asFloatBuffer()
        mv = memoryview(obj)
        self.assertEqual(mv.itemsize, 4)
        self.assertEqual(mv.strides, (4,))
        self.assertEqual(mv.suboffsets, tuple())
        self.assertFalse(mv.readonly)
        self.assertEqual(mv.shape, (6,))
        self.assertEqual(mv.format, ">f")

    def testMemoryViewDouble(self):
        obj = self.cls.allocateDirect(24).asDoubleBuffer()
        mv = memoryview(obj)
        self.assertEqual(mv.itemsize, 8)
        self.assertEqual(mv.strides, (8,))
        self.assertEqual(mv.suboffsets, tuple())
        self.assertFalse(mv.readonly)
        self.assertEqual(mv.shape, (3,))
        self.assertEqual(mv.format, ">d")

    def checkNP(self, method, dtype, sz):
        obj = self.cls.allocateDirect(sz * 5)
        obj.order(self.bo.BIG_ENDIAN)
        bf = method(obj)
        mv = np.asarray(memoryview(bf))
        ja = JArray(dtype)(5)
        mv[:] = [1, 2, 3, 4, 5]
        bf.get(ja)
        self.assertEqual(list(ja), [1, 2, 3, 4, 5])
        obj.order(self.bo.LITTLE_ENDIAN)
        bf = method(obj)
        mv = np.asarray(memoryview(bf))
        ja = JArray(dtype)(5)
        mv[:] = [1, 2, 3, 4, 5]
        bf.get(ja)
        self.assertEqual(list(ja), [1, 2, 3, 4, 5])

    @common.requireNumpy
    def testMemoryViewShortNP(self):
        self.checkNP(self.cls.asShortBuffer, JShort, 2)

    @common.requireNumpy
    def testMemoryViewIntNP(self):
        self.checkNP(self.cls.asIntBuffer, JInt, 4)

    @common.requireNumpy
    def testMemoryViewLongNP(self):
        self.checkNP(self.cls.asLongBuffer, JLong, 8)

    @common.requireNumpy
    def testMemoryViewFloatNP(self):
        self.checkNP(self.cls.asFloatBuffer, JFloat, 4)

    @common.requireNumpy
    def testMemoryViewDoubleNP(self):
        self.checkNP(self.cls.asDoubleBuffer, JDouble, 8)
