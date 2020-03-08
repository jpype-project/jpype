import sys
import jpype
from jpype.types import *
import common
import pytest
import random


def haveNumpy():
    try:
        import numpy
        return True
    except ImportError:
        return False


class ArrayShortTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.VALUES = [random.randint(-2**15, 2**15-1) for i in range(10)]

    def assertElementsEqual(self, a, b):
        self.assertEqual(len(a), len(b))
        for i in range(len(a)):
            self.assertEqual(a[i], b[i])

    def testJShortArrayConversionFail(self):
        jarr = JArray(JShort)(self.VALUES)
        with self.assertRaises(TypeError):
            jarr[1] = 'a'

    def testJShortArraySliceLength(self):
        jarr = JArray(JShort)(self.VALUES)
        jarr[1:2] = [1]
        with self.assertRaises(ValueError):
            jarr[1:2] = [1, 2, 3]

    def testJShortArrayConversion(self):
        jarr = JArray(JShort)(self.VALUES)
        result = jarr[0: len(jarr)]
        self.assertElementsEqual(self.VALUES, result)
        result = jarr[2:10]
        self.assertElementsEqual(self.VALUES[2:10], result)

    def testJShortArrayConversionError(self):
        jarr = JArray(JShort, 1)(10)
        with self.assertRaises(TypeError):
            jarr[1:2] = [dict()]
        # -1 is returned by python, if conversion fails also, ensure this works
        jarr[1:2] = [-1]

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testSetFromNPShortArray(self):
        import numpy as np
        n = 100
        a = np.random.randint(-32768, 32767, size=n, dtype=np.short)
        jarr = JArray(JShort)(n)
        jarr[:] = a
        self.assertElementsEqual(a, jarr)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testJShortArrayInitFromNP(self):
        import numpy as np
        n = 100
        a = np.random.random(n).astype(np.int)
        jarr = JArray(JShort)(a)
        self.assertElementsEqual(a, jarr)

    def testJShortArrayClone(self):
        array = jpype.JArray(JShort, 2)([[1, 2], [3, 4]])
        carray = array.clone()
        # Verify the first dimension is cloned
        self.assertFalse(array.equals(carray))
        # Copy is shallow
        self.assertTrue(array[0].equals(carray[0]))

    def testJShortArrayGetSlice(self):
        contents = self.VALUES
        array = JArray(JShort)(contents)
        self.assertEqual(list(array[1:]), contents[1:])
        self.assertEqual(list(array[:-1]), contents[:-1])
        self.assertEqual(list(array[1:-1]), contents[1:-1])

    def testJShortArraySetSlice(self):
        contents = [1, 2, 3, 4]
        array = JArray(JShort)(contents)
        array[1:] = [5, 6, 7]
        contents[1:] = [5, 6, 7]
        self.assertEqual(list(array[:]), contents[:])
        array[:-1] = [8, 9, 10]
        contents[:-1] = [8, 9, 10]
        self.assertEqual(list(array[:]), contents[:])

    def testJShortArrayGetSliceStep(self):
        contents = self.VALUES
        array = JArray(JShort)(contents)
        self.assertEqual(list(array[::2]), contents[::2])
        self.assertEqual(list(array[::3]), contents[::3])
        self.assertEqual(list(array[::4]), contents[::4])
        self.assertEqual(list(array[::5]), contents[::5])
        self.assertEqual(list(array[::6]), contents[::6])
        self.assertEqual(list(array[::7]), contents[::7])
        self.assertEqual(list(array[::8]), contents[::8])
        self.assertEqual(list(array[1::3]), contents[1::3])
        self.assertEqual(list(array[1:-2:3]), contents[1:-2:3])

    def testJShortArraySliceStepNeg(self):
        contents = self.VALUES
        array = JArray(JShort)(contents)
        self.assertEqual(list(array[::-1]), contents[::-1])
        self.assertEqual(list(array[::-2]), contents[::-2])
        self.assertEqual(list(array[::-3]), contents[::-3])
        self.assertEqual(list(array[::-4]), contents[::-4])
        self.assertEqual(list(array[::-5]), contents[::-5])
        self.assertEqual(list(array[::-6]), contents[::-6])
        self.assertEqual(list(array[2::-3]), contents[2::-3])
        self.assertEqual(list(array[-2::-3]), contents[-2::-3])

    def testJShortArraySetArraySliceStep(self):
        contents = [1, 2, 3, 4, 5, 6]
        array = JArray(JShort)(contents)
        array[::2] = [5, 6, 7]
        contents[::2] = [5, 6, 7]
        self.assertEqual(list(array[:]), contents[:])

    def testJShortArrayEquals(self):
        contents = self.VALUES
        array = JArray(JShort)(contents)
        array2 = JArray(JShort)(contents)
        self.assertEqual(array, array)
        self.assertNotEqual(array, array2)

    def testJShortArrayIter(self):
        contents = self.VALUES
        array = JArray(JShort)(contents)
        contents2 = [i for i in array]
        self.assertEqual(contents, contents2)

    def testJShortArrayGetOutOfBounds(self):
        contents = [1, 2, 3, 4]
        array = JArray(JShort)(contents)
        with self.assertRaises(IndexError):
            array[5]
        self.assertEqual(array[-1], contents[-1])
        self.assertEqual(array[-4], contents[-4])
        with self.assertRaises(IndexError):
            array[-5]

    def testJShortArraySetOutOfBounds(self):
        contents = [1, 2, 3, 4]
        array = JArray(JShort)(contents)
        with self.assertRaises(IndexError):
            array[5] = 1
        array[-1] = 5
        contents[-1] = 5
        array[-4] = 6
        contents[-4] = 6
        self.assertEqual(list(array[:]), contents)
        with self.assertRaises(IndexError):
            array[-5] = 1

    def testJShortArraySliceCast(self):
        JA = JArray(JShort)
        ja = JA(self.VALUES)
        ja2 = ja[::2]
        jo = jpype.JObject(ja2, object)
        ja3 = jpype.JObject(jo, JA)
        self.assertEqual(type(jo), jpype.JClass("java.lang.Object"))
        self.assertEqual(type(ja2), JA)
        self.assertEqual(type(ja3), JA)
        self.assertEqual(list(ja2), list(ja3))

    def testJShortArrayReverse(self):
        n = self.VALUES
        JA = JArray(JShort)
        ja = JA(n)
        a = [i for i in reversed(ja)]
        n = [i for i in reversed(n)]
        self.assertEqual(a, n)
