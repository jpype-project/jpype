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


class ArrayBoolTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testJArrayConversionBool(self):
        expected = [True, False, False, True]
        jarr = jpype.JArray(jpype.JBoolean)(expected)
        self.assertEqual(expected, list(jarr[:]))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testSetFromNPBoolArray(self):
        import numpy as np
        n = 100
        a = np.random.randint(0, 1, size=n, dtype=np.bool)
        jarr = jpype.JArray(jpype.JBoolean)(n)
        jarr[:] = a
        self.assertCountEqual(a, jarr)

