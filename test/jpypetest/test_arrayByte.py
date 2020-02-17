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


class ArrayByteTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testByteArrayAsString(self):
        t = JClass("jpype.array.TestArray")()
        v = t.getByteArray()
        self.assertEqual(str(v), 'avcd')

    def testByteArrayIntoVector(self):
        ba = jpype.JArray(jpype.JByte)(b'123')
        v = jpype.java.util.Vector(1)
        v.add(ba)
        self.assertEqual(len(v), 1)
        self.assertNotEqual(v[0], None)

    def testByteArraySimple(self):
        a = JArray(JByte)(2)
        a[1] = 2
        self.assertEqual(a[1], 2)

    def testJArrayConversionByte(self):
        expected = (0, 1, 2, 3)
        ByteBuffer = jpype.java.nio.ByteBuffer
        bb = ByteBuffer.allocate(4)
        buf = bb.array()
        for i in range(len(expected)):
            buf[i] = expected[i]
        for i in range(len(expected)):
            self.assertEqual(expected[i], buf[i])


