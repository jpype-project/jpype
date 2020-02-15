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


class ConversionBuffer(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanToNP(self):
        data = [1, 0, 256, 0.5, 1e9]
        ja = JArray(JBoolean)(data)
        na = np.array(data, dtype=np.bool)
        self.assertTrue(np.all(np.array(ja, dtype=np.double)
                               == np.array(na, dtype=np.double)))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testCharToNP(self):
        data = [1, 0, 0.5, 5.5, 65535]
        ja = JArray(JChar)(data)
        na = np.array(data, dtype=np.uint16)
        self.assertTrue(np.all(np.array(ja, dtype=np.double)
                               == np.array(na, dtype=np.double)))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteToNP(self):
        data = [1, 0, -128, -1, 0.5, 127]
        ja = JArray(JByte)(data)
        na = np.array(data, dtype=np.byte)
        self.assertTrue(np.all(np.array(ja, dtype=np.double)
                               == np.array(na, dtype=np.double)))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortToNP(self):
        data = [1, 0, 256, -1, 0.5, -32768, 32767]
        ja = JArray(JShort)(data)
        na = np.array(data, dtype=np.int16)
        self.assertTrue(np.all(np.array(ja, dtype=np.double)
                               == np.array(na, dtype=np.double)))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntToNP(self):
        data = [1, 0, 256, -1, 0.5, 1e9]
        ja = JArray(JInt)(data)
        na = np.array(data, dtype=np.int32)
        self.assertTrue(np.all(np.array(ja, dtype=np.double)
                               == np.array(na, dtype=np.double)))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongToNP(self):
        data = [1, 0, 256, -1, 0.5, 1e9]
        ja = JArray(JLong)(data)
        na = np.array(data, dtype=np.int64)
        self.assertTrue(np.all(np.array(ja, dtype=np.double)
                               == np.array(na, dtype=np.double)))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatToNP(self):
        data = [1, 0, 256, -1, 0.5, 1e9]
        ja = JArray(JFloat)(data)
        na = np.array(data, dtype=np.float)
        self.assertTrue(np.all(np.array(ja, dtype=np.double)
                               == np.array(na, dtype=np.double)))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleToNP(self):
        data = [1, 0, 256, -1, 0.5, 1e9]
        ja = JArray(JDouble)(data)
        na = np.array(data, dtype=np.double)
        self.assertTrue(np.all(np.array(ja, dtype=np.double)
                               == np.array(na, dtype=np.double)))

    def testToMemoryview(self):
        data = [1, 2, 3, 4, 5]
        ja = JArray(JInt)(data)
        m1 = memoryview(ja)

    def testDoubleOpen(self):
        data = [1, 2, 3, 4, 5]
        ja=JArray(JInt)(data)
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
        self.assertEqual(len(bytes(ja)), 6*2)

    def testIntToBytes(self):
        data = [0, 1, 2, 3, 4, 5]
        ja = JArray(JInt)(data)
        self.assertEqual(len(bytes(ja)), 6*4)

    def testLongToBytes(self):
        data = [0, 1, 2, 3, 4, 5]
        ja = JArray(JLong)(data)
        self.assertEqual(len(bytes(ja)), 6*8)

    def testMemoryViewWrite(self):
        data = [1, 2, 3, 4, 5]
        ja=JArray(JInt)(data)
        b = memoryview(ja) # Create a view
        b[0] = 123 # Alter the memory using the view
        del b # Commit to Java
        self.assertEqual(ja[0], 123)

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


    def executeConvert(self, ja, na):
        # Force every conversion to be executed
        self.assertTrue(np.all(np.array(ja, dtype=np.bool) == np.array(na, dtype=np.bool)))
        self.assertTrue(np.all(np.array(ja, dtype=np.uint8) == np.array(na, dtype=np.uint8)))
        self.assertTrue(np.all(np.array(ja, dtype=np.int8) == np.array(na, dtype=np.int8)))
        self.assertTrue(np.all(np.array(ja, dtype=np.uint16) == np.array(na, dtype=np.uint16)))
        self.assertTrue(np.all(np.array(ja, dtype=np.int16) == np.array(na, dtype=np.int16)))
        self.assertTrue(np.all(np.array(ja, dtype=np.uint32) == np.array(na, dtype=np.uint32)))
        self.assertTrue(np.all(np.array(ja, dtype=np.int32) == np.array(na, dtype=np.int32)))
        self.assertTrue(np.all(np.array(ja, dtype=np.uint64) == np.array(na, dtype=np.uint64)))
        self.assertTrue(np.all(np.array(ja, dtype=np.int64) == np.array(na, dtype=np.int64)))
        self.assertTrue(np.all(np.array(ja, dtype=np.float32) == np.array(na, dtype=np.float32)))
        self.assertTrue(np.all(np.array(ja, dtype=np.float64) == np.array(na, dtype=np.float64)))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBoolConvert(self):
        n = 100
        na = np.random.randint(0, 1, size=n).astype(np.bool)
        ja = JArray(JBoolean)(na)
        self.executeConvert(ja, na)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testCharConvert(self):
        # This is not technically correct as Char is not a number type, but we are
        # testing buffer transfer here
        n = 100
        na = np.random.randint(0, 65535, size=n).astype(np.uint16)
        ja = JArray(JChar)(na)
        self.executeConvert(ja, na)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteConvert(self):
        n = 100
        na = np.random.randint(-128, 127, size=n).astype(np.byte)
        ja = JArray(JByte)(na)
        self.executeConvert(ja, na)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortConvert(self):
        n = 100
        na = np.random.randint(-32768, 32767, size=n).astype(np.int16)
        ja = JArray(JShort)(na)
        self.executeConvert(ja, na)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntConvert(self):
        n = 100
        na = np.random.randint(-2**31, 2**31 - 1, size=n).astype(np.int32)
        ja = JArray(JInt)(na)
        self.executeConvert(ja, na)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatConvert(self):
        n = 100
        na = np.random.randint(-2**63, 2**63 - 1, size=n, dtype=np.int64)
        ja = JArray(JLong)(na)
        self.executeConvert(ja, na)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatConvert(self):
        n = 100
        na = np.random.random(n).astype(np.float32)
        ja = JArray(JFloat)(na)
        self.executeConvert(ja, na)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatConvert(self):
        n = 100
        na = np.random.random(n).astype(np.float64)
        ja = JArray(JDouble)(na)
        self.executeConvert(ja, na)

