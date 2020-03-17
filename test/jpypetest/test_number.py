import sys
import jpype
import common
import random
import _jpype
import jpype
from jpype import java
from jpype.types import *
try:
    import numpy as np
except ImportError:
    pass


class JNumberTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.cls = JClass("jpype.common.Fixture")
        self.fixture = self.cls()

    def testJavaPrimitives(self):
        self.assertIsInstance(
            self.fixture.callNumber(JByte(1)), java.lang.Byte)
        self.assertIsInstance(
            self.fixture.callNumber(JShort(1)), java.lang.Short)
        self.assertIsInstance(
            self.fixture.callNumber(JInt(1)), java.lang.Integer)
        self.assertIsInstance(
            self.fixture.callNumber(JLong(1)), java.lang.Long)
        self.assertIsInstance(
            self.fixture.callNumber(JFloat(1)), java.lang.Float)
        self.assertIsInstance(self.fixture.callNumber(
            JDouble(1)), java.lang.Double)

    def testPythonPrimitives(self):
        self.assertIsInstance(self.fixture.callNumber(1), java.lang.Long)
        self.assertIsInstance(self.fixture.callNumber(1.0), java.lang.Double)

    @common.requireNumpy
    def testNumpyPrimitives(self):
        self.assertIsInstance(
            self.fixture.callNumber(np.int8(1)), java.lang.Byte)
        self.assertIsInstance(self.fixture.callNumber(
            np.int16(1)), java.lang.Short)
        self.assertIsInstance(self.fixture.callNumber(
            np.int32(1)), java.lang.Integer)
        self.assertIsInstance(self.fixture.callNumber(
            np.int64(1)), java.lang.Long)
        self.assertIsInstance(self.fixture.callNumber(
            np.float32(1)), java.lang.Float)
        self.assertIsInstance(self.fixture.callNumber(
            np.float64(1)), java.lang.Double)
