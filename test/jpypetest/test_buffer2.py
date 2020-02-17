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


class Buffer2TestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanToNP(self):
        data = np.random.randint(0, 1, size=(5,7), dtype=np.bool)
        a = JArray(JBoolean, data.ndim)(data.tolist())
        u=np.array(a)
        self.assertTrue(np.all(data==u))
        mv = memoryview(a)
        self.assertEqual(mv.format, "?")
        self.assertEqual(mv.shape, data.shape)
        self.assertTrue(mv.readonly)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testCharToNP(self):
        data = np.random.randint(0, 2**16, size=(5,7), dtype=np.uint16)
        a = JArray(JChar, data.ndim)(data.tolist())
        u=np.array(a)
        self.assertTrue(np.all(data==u))
        mv = memoryview(a)
        self.assertEqual(mv.format, "H")
        self.assertEqual(mv.shape, data.shape)
        self.assertTrue(mv.readonly)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteToNP(self):
        data = np.random.randint(-2**7, 2**7 - 1, size=(5,7), dtype=np.int8)
        a = JArray(JByte, data.ndim)(data.tolist())
        u=np.array(a)
        self.assertTrue(np.all(data==u))
        mv = memoryview(a)
        self.assertEqual(mv.format, "b")
        self.assertEqual(mv.shape, data.shape)
        self.assertTrue(mv.readonly)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortToNP(self):
        data = np.random.randint(-2**15, 2**15 - 1, size=(5,7), dtype=np.int16)
        a = JArray(JShort, data.ndim)(data.tolist())
        u=np.array(a)
        self.assertTrue(np.all(data==u))
        mv = memoryview(a)
        self.assertEqual(mv.format, "h")
        self.assertEqual(mv.shape, data.shape)
        self.assertTrue(mv.readonly)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntToNP(self):
        data = np.random.randint(-2**31, 2**31 - 1, size=(5,7), dtype=np.int32)
        a = JArray(JInt, data.ndim)(data.tolist())
        u=np.array(a)
        self.assertTrue(np.all(data==u))
        mv = memoryview(a)
        self.assertEqual(mv.format, "i")
        self.assertEqual(mv.shape, data.shape)
        self.assertTrue(mv.readonly)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongToNP(self):
        data = np.random.randint(-2**63, 2**63 - 1, size=(5,7), dtype=np.int64)
        a = JArray(JLong, data.ndim)(data.tolist())
        u=np.array(a)
        self.assertTrue(np.all(data==u))
        mv = memoryview(a)
        self.assertEqual(mv.format, "q")
        self.assertEqual(mv.shape, data.shape)
        self.assertTrue(mv.readonly)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatToNP(self):
        data = np.random.rand(5,7).astype(np.float32)
        a = JArray(JFloat, data.ndim)(data.tolist())
        u=np.array(a)
        self.assertTrue(np.all(data==u))
        mv = memoryview(a)
        self.assertEqual(mv.format, "f")
        self.assertEqual(mv.shape, data.shape)
        self.assertTrue(mv.readonly)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleToNP(self):
        data = np.random.rand(5,7).astype(np.float64)
        a = JArray(JDouble, data.ndim)(data.tolist())
        u=np.array(a)
        self.assertTrue(np.all(data==u))
        mv = memoryview(a)
        self.assertEqual(mv.format, "d")
        self.assertEqual(mv.shape, data.shape)
        self.assertTrue(mv.readonly)

