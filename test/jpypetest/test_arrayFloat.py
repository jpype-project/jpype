import sys
import jpype
from jpype.types import *
import common
import pytest

def haveNumpy():
    try:
        import numpy
        return True
    except ImportError:
        return False


class ArrayFloatTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testJArrayConversionFloat(self):
        VALUES = [float(x) for x in self.VALUES]
        jarr = jpype.JArray(jpype.JFloat)(VALUES)
        result = jarr[0: len(jarr)]
        self.assertCountEqual(jarr, result)

        result = jarr[2:10]
        self.assertCountEqual(VALUES[2:10], result)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testSetFromNPFloatArray(self):
        import numpy as np
        n = 100
        a = np.random.random(n).astype(np.float32)
        jarr = jpype.JArray(jpype.JFloat)(n)
        jarr[:] = a
        self.assertCountEqual(a, jarr)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testInitFromNPFloatArrayInt(self):
        import numpy as np
        a = np.array([1, 2, 3], dtype=np.int32)
        jarr = jpype.JArray(jpype.JFloat)(a)
        self.assertCountEqual(a, jarr)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testSetFromNPFloatArrayInt(self):
        import numpy as np
        a = np.array([1, 2, 3], np.int32)
        jarr = jpype.JArray(jpype.JFloat)(len(a))
        jarr[:] = a
        self.assertCountEqual(a, jarr)

