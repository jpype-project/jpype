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


class ArrayShortTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testJArrayConversionShort(self):
        # filter out values, which can not be converted to jshort
        self.VALUES = [v for v in self.VALUES if v < (2**16 / 2 - 1)
                       and v > (2**16 / 2 * -1)]
        jarr = jpype.JArray(jpype.JShort)(self.VALUES)
        result = jarr[0: len(jarr)]
        self.assertCountEqual(self.VALUES, result)

        result = jarr[2:10]
        self.assertCountEqual(self.VALUES[2:10], result)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testSetFromNPShortArray(self):
        import numpy as np
        n = 100
        a = np.random.randint(-32768, 32767, size=n, dtype=np.short)
        jarr = jpype.JArray(jpype.JShort)(n)
        jarr[:] = a
        self.assertCountEqual(a, jarr)


