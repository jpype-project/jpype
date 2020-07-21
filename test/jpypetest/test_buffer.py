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
import jpype
import sys
import logging
import time
import common
from jpype.types import *


try:
    import numpy as np
    gotNP = True
except ImportError:
    gotNP = False


def haveNumpy():
    return gotNP


class BufferTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testToMemoryview(self):
        data = [1, 2, 3, 4, 5]
        ja = JArray(JInt)(data)
        m1 = memoryview(ja)

    def testDoubleOpen(self):
        data = [1, 2, 3, 4, 5]
        ja = JArray(JInt)(data)
        m1 = memoryview(ja)
        m2 = memoryview(ja)
        del m1
        del m2

    def testBytesToBytes(self):
        data = [0, 1, 2, 3, 4, 5]
        ja = JArray(JByte)(data)
        self.assertEqual(len(bytes(ja)), 6)

    def testShortToBytes(self):
        data = [0, 1, 2, 3, 4, 5]
        ja = JArray(JShort)(data)
        self.assertEqual(len(bytes(ja)), 6 * 2)

    def testIntToBytes(self):
        data = [0, 1, 2, 3, 4, 5]
        ja = JArray(JInt)(data)
        self.assertEqual(len(bytes(ja)), 6 * 4)

    def testLongToBytes(self):
        data = [0, 1, 2, 3, 4, 5]
        ja = JArray(JLong)(data)
        self.assertEqual(len(bytes(ja)), 6 * 8)

    def testMemoryViewWrite(self):
        data = [1, 2, 3, 4, 5]
        ja = JArray(JInt)(data)
        b = memoryview(ja)  # Create a view
        with self.assertRaises(TypeError):
            b[0] = 123  # Alter the memory using the view

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testSlice(self):
        data = [0, 1, 2, 3, 4, 5]
        ja = JArray(JInt)(data)
        self.assertTrue(bytes(ja[1:3]) == bytes(
            np.array(data[1:3], dtype=np.int32)))
        self.assertTrue(bytes(ja[1:]) == bytes(
            np.array(data[1:], dtype=np.int32)))
        self.assertTrue(bytes(ja[:-1]) ==
                        bytes(np.array(data[:-1], dtype=np.int32)))
        self.assertTrue(bytes(ja[::2]) == bytes(
            np.array(data[::2], dtype=np.int32)))
        self.assertTrue(bytes(ja[::-1]) ==
                        bytes(np.array(data[::-1], dtype=np.int32)))
        self.assertTrue(bytes(ja[::-2]) ==
                        bytes(np.array(data[::-2], dtype=np.int32)))

    def executeConvert(self, jtype, dtype):
        n = 100
        na = np.random.randint(0, 1, size=n).astype(np.bool)
        self.assertTrue(np.all(np.array(jtype(na), dtype=dtype)
                               == np.array(na, dtype=dtype)))
        na = np.random.randint(-2**7, 2**7 - 1, size=n, dtype=np.int8)
        self.assertTrue(np.all(np.array(jtype(na), dtype=dtype)
                               == np.array(na, dtype=dtype)))
        na = np.random.randint(0, 2**8 - 1, size=n, dtype=np.uint8)
        self.assertTrue(np.all(np.array(jtype(na), dtype=dtype)
                               == np.array(na, dtype=dtype)))
        na = np.random.randint(-2**15, 2**15 - 1, size=n, dtype=np.int16)
        self.assertTrue(np.all(np.array(jtype(na), dtype=dtype)
                               == np.array(na, dtype=dtype)))
        na = np.random.randint(0, 2**16 - 1, size=n, dtype=np.uint16)
        self.assertTrue(np.all(np.array(jtype(na), dtype=dtype)
                               == np.array(na, dtype=dtype)))
        na = np.random.randint(-2**31, 2**31 - 1, size=n, dtype=np.int32)
        self.assertTrue(np.all(np.array(jtype(na), dtype=dtype)
                               == np.array(na, dtype=dtype)))
        na = np.random.randint(0, 2**32 - 1, size=n, dtype=np.uint32)
        self.assertTrue(np.all(np.array(jtype(na), dtype=dtype)
                               == np.array(na, dtype=dtype)))
        na = np.random.randint(-2**63, 2**63 - 1, size=n, dtype=np.int64)
        self.assertTrue(np.all(np.array(jtype(na), dtype=dtype)
                               == np.array(na, dtype=dtype)))
        na = np.random.randint(0, 2**64 - 1, size=n, dtype=np.uint64)
        self.assertTrue(np.all(np.array(jtype(na), dtype=dtype)
                               == np.array(na, dtype=dtype)))
        na = np.random.random(n).astype(np.float32)
        self.assertTrue(np.all(np.array(jtype(na), dtype=dtype)
                               == np.array(na, dtype=dtype)))
        na = np.random.random(n).astype(np.float64)
        self.assertTrue(np.all(np.array(jtype(na), dtype=dtype)
                               == np.array(na, dtype=dtype)))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBoolConvert(self):
        self.executeConvert(JArray(JBoolean), np.bool)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteConvert(self):
        self.executeConvert(JArray(JByte), np.int8)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortConvert(self):
        self.executeConvert(JArray(JShort), np.int16)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntConvert(self):
        self.executeConvert(JArray(JInt), np.int32)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongConvert(self):
        self.executeConvert(JArray(JLong), np.int64)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatConvert(self):
        self.executeConvert(JArray(JFloat), np.float32)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleConvert(self):
        self.executeConvert(JArray(JDouble), np.float64)

    def executeIntTest(self, jtype, limits, size, dtype, code):
        data = np.random.randint(limits[0], limits[1], size=size, dtype=dtype)
        a = JArray(jtype, data.ndim)(data.tolist())
        u = np.array(a)
        self.assertTrue(np.all(data == u))
        mv = memoryview(a)
        self.assertEqual(mv.format, code)
        self.assertEqual(mv.shape, data.shape)
        self.assertTrue(mv.readonly)

    def executeFloatTest(self, jtype, size, dtype, code):
        data = np.random.rand(*size).astype(dtype)
        a = JArray(jtype, data.ndim)(data)
        u = np.array(a)
        self.assertTrue(np.all(data == u))
        mv = memoryview(a)
        self.assertEqual(mv.format, code)
        self.assertEqual(mv.shape, data.shape)
        self.assertTrue(mv.readonly)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanToNP1D(self):
        self.executeIntTest(JBoolean, [0, 1], (100,), np.bool, "?")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testCharToNP1D(self):
        self.executeIntTest(JChar, [0, 2**16], (100,), np.uint16, "H")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteToNP1D(self):
        self.executeIntTest(JByte, [-2**7, 2**7 - 1], (100,), np.int8, "b")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortToNP1D(self):
        self.executeIntTest(JShort, [-2**15, 2**15 - 1], (100,), np.int16, "h")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntToNP1D(self):
        self.executeIntTest(JInt, [-2**31, 2**31 - 1], (100,), np.int32, "i")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongToNP1D(self):
        self.executeIntTest(JLong, [-2**63, 2**63 - 1], (100,), np.int64, "q")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatToNP1D(self):
        self.executeFloatTest(JFloat, (100,), np.float32, "f")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleToNP1D(self):
        self.executeFloatTest(JDouble, (100,), np.float64, "d")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanToNP2D(self):
        self.executeIntTest(JBoolean, [0, 1], (11, 10), np.bool, "?")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testCharToNP2D(self):
        self.executeIntTest(JChar, [0, 2**16], (11, 10), np.uint16, "H")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteToNP2D(self):
        self.executeIntTest(JByte, [-2**7, 2**7 - 1], (11, 10), np.int8, "b")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortToNP2D(self):
        self.executeIntTest(JShort, [-2**15, 2**15 - 1], (11, 10), np.int16, "h")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntToNP2D(self):
        self.executeIntTest(JInt, [-2**31, 2**31 - 1], (11, 10), np.int32, "i")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongToNP2D(self):
        self.executeIntTest(JLong, [-2**63, 2**63 - 1], (11, 10), np.int64, "q")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatToNP2D(self):
        self.executeFloatTest(JFloat, (11, 10), np.float32, "f")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleToNP2D(self):
        self.executeFloatTest(JDouble, (11, 10), np.float64, "d")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanToNP3D(self):
        self.executeIntTest(JBoolean, [0, 1], (11, 10, 9), np.bool, "?")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testCharToNP3D(self):
        self.executeIntTest(JChar, [0, 2**16], (11, 10, 9), np.uint16, "H")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteToNP3D(self):
        self.executeIntTest(JByte, [-2**7, 2**7 - 1], (11, 10, 9), np.int8, "b")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortToNP3D(self):
        self.executeIntTest(JShort, [-2**15, 2**15 - 1],
                            (11, 10, 9), np.int16, "h")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntToNP3D(self):
        self.executeIntTest(JInt, [-2**31, 2**31 - 1],
                            (11, 10, 9), np.int32, "i")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongToNP3D(self):
        self.executeIntTest(JLong, [-2**63, 2**63 - 1],
                            (11, 10, 9), np.int64, "q")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatToNP3D(self):
        self.executeFloatTest(JFloat, (11, 10, 9), np.float32, "f")

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleToNP3D(self):
        self.executeFloatTest(JDouble, (11, 10, 9), np.float64, "d")
