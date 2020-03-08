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


class ArrayCharTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testCharArrayAsString(self):
        t = JClass("jpype.array.TestArray")()
        v = t.getCharArray()
        self.assertEqual(str(v), 'avcd')

    def testJArrayConversionChar(self):
        t = JClass("jpype.array.TestArray")()
        v = t.getCharArray()
        self.assertEqual(str(v[:]), 'avcd')

    def testEqualsChar(self):
        contents = "abc"
        array = jpype.JArray(jpype.JChar)(contents)
        array2 = jpype.JArray(jpype.JChar)(contents)
        self.assertEqual(array, array)
        self.assertNotEqual(array, array2)
        self.assertEqual(array, "abc")
