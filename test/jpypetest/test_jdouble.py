import _jpype
import jpype
import common
from jpype.types import *

class JDoubleTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    @common.requireInstrumentation
    def testFault(self):
        _jpype.fault("JPDoubleType::getJavaConversion")
        with self.assertRaises(SystemError):
            JDouble(1.0)

    def testFromJIntWiden(self):
        self.assertEqual(JDouble(JByte(123)), 123)
        self.assertEqual(JDouble(JShort(12345)), 12345)
        self.assertEqual(JDouble(JInt(123456789)), 123456789)
        self.assertEqual(JDouble(JLong(123456789)), 123456789)

    def testFromJFloatWiden(self):
        self.assertEqual(JDouble(JFloat(12345678)), 12345678)

    def testFromNone(self):
        with self.assertRaises(TypeError):
            JDouble(None)
        self.assertEqual(JDouble._canConvertToJava(None), "none")

    def testFromJDouble(self):
        self.assertEqual(JDouble(JDouble(1.2345)), 1.2345)

    def testUnBox(self):
        self.assertEqual(JDouble(jpype.java.lang.Double(1.2345)), 1.2345)

    def testFromFloat(self):
        self.assertEqual(JDouble(1.2345), 1.2345)
        self.assertEqual(JDouble._canConvertToJava(1.2345), "exact")

    def testFromLong(self):
        self.assertEqual(JDouble(12345), 12345)
        self.assertEqual(JDouble._canConvertToJava(12345), "implicit")

    def testFromObject(self):
        with self.assertRaises(TypeError):
            JDouble(object())
        self.assertEqual(JDouble._canConvertToJava(object()), "none")

    def testSetArrayRange(self):
        ja = JArray(JDouble)(3)
        ja[0:1] = [123]
        self.assertEqual(ja[0], 123)
        ja[0:1] = [-1]
        self.assertEqual(ja[0], -1)
        ja[0:1] = [jpype.java.lang.Double(321)]
        self.assertEqual(ja[0], 321)
        with self.assertRaises(TypeError):
            ja[0:1] = [object()]

    @common.requireInstrumentation
    def testSetArrayRangeFault(self):
        ja = JArray(JDouble)(3)
        _jpype.fault("JPDoubleType::setArrayRange")
        with self.assertRaises(SystemError):
            ja[0:1] = [123]
        ja[0:1] = [123]




