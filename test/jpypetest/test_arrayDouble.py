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


class ArrayTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.VALUES = [random.random() for i in range(10)]

    def assertElementsAlmostEqual(self, a, b):
        self.assertEqual(len(a), len(b))
        for i in range(len(a)):
                self.assertAlmostEqual(a[i], b[i])

    def testJArrayConversionDouble(self):
        VALUES = [float(x) for x in self.VALUES]
        jarr = jpype.JArray(jpype.JDouble)(VALUES)
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
        self.assertCountEqual(expected, result)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testSetFromNPDoubleArray(self):
        import numpy as np
        n = 100
        a = np.random.random(n).astype(np.float64)
        jarr = jpype.JArray(jpype.JDouble)(n)
        jarr[:] = a
        self.assertElementsAlmostEqual(a, jarr)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testInitFromNPDoubleArray(self):
        import numpy as np
        n = 100
        a = np.random.random(n).astype(np.float)
        jarr = jpype.JArray(jpype.JDouble)(a)
        self.assertElementsAlmostEqual(a, jarr)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testInitFromNPDoubleArrayFloat32(self):
        import numpy as np
        n = 100
        a = np.random.random(n).astype(np.float32)
        jarr = jpype.JArray(jpype.JDouble)(a)
        self.assertElementsAlmostEqual(a, jarr)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testInitFromNPDoubleArrayFloat64(self):
        import numpy as np
        n = 100
        a = np.random.random(n).astype(np.float64)
        jarr = jpype.JArray(jpype.JDouble)(a)
        self.assertElementsAlmostEqual(a, jarr)

