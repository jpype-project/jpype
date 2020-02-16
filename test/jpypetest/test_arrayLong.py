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


class ArrayTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testJArrayConversionLong(self):
        jarr = jpype.JArray(jpype.JLong)(self.VALUES)
        result = jarr[0: len(jarr)]
        self.assertCountEqual(self.VALUES, result)

        result = jarr[2:10]
        self.assertCountEqual(self.VALUES[2:10], result)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testSetFromNPLongArray(self):
        import numpy as np
        n = 100
        # actuall the lower bound should be -2**63 -1, but raises Overflow
        # error in numpy
        a = np.random.randint(-2**63, 2**63 - 1, size=n, dtype=np.int64)
        jarr = jpype.JArray(jpype.JLong)(n)
        jarr[:] = a
        self.assertCountEqual(a, jarr)

