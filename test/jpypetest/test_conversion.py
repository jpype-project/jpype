import common
import jpype
import _jpype
from jpype.types import *


class ConversionTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.jc1 = jpype.JClass("java.lang.String")
        self.jc2 = jpype.JClass("java.lang.Integer")

    def testCanConvertExact(self):
        self.assertEqual(self.jc1._canConvertToJava("a"), "exact")

    def testCanConvertNone(self):
        self.assertEqual(self.jc1._canConvertToJava(1), "none")

    def testCanConvertExplicit(self):
        self.assertEqual(
            self.jc2._canConvertToJava(1), "explicit")

    def testCanConvertImplicit(self):
        self.assertEqual(
            self.jc1._canConvertToJava(None), "implicit")

    def testConvertExact(self):
        self.assertIsInstance(
            self.jc1._convertToJava("a"), self.jc1)

    def testConvertImplicit(self):
        self.assertIsInstance(
            self.jc1._convertToJava(None), self.jc1)

    def testConvertExplicit(self):
        self.assertIsInstance(
            self.jc2._convertToJava(1), self.jc2)

    def testConvertNone(self):
        with self.assertRaises(TypeError):
            self.jc1._convertToJava(1)

    def testUnbox(self):
        jf = JClass('jpype.fields.Fields')
        java = jpype.java
        jf.static_bool = java.lang.Boolean(True)
        self.assertEqual(jf.static_bool, True)
        jf.static_char = java.lang.Character("a")
        self.assertEqual(jf.static_char, "a")
        jf.static_byte = java.lang.Byte(123)
        self.assertEqual(jf.static_byte, 123)
        jf.static_short = java.lang.Short(123)
        self.assertEqual(jf.static_short, 123)
        jf.static_int = java.lang.Integer(123)
        self.assertEqual(jf.static_int, 123)
        jf.static_long = java.lang.Long(123)
        self.assertEqual(jf.static_long, 123)
        jf.static_float = java.lang.Float(123)
        self.assertEqual(jf.static_float, 123)
        jf.static_double = java.lang.Double(123)
        self.assertEqual(jf.static_double, 123)

    def testUnboxFail(self):
        java = jpype.java
        with self.assertRaises(TypeError):
            JBoolean._convertToJava(java.lang.Double(1))
        with self.assertRaises(TypeError):
            JChar._convertToJava(java.lang.Double(1))
        with self.assertRaises(TypeError):
            JByte._convertToJava(java.lang.Boolean(1))
        with self.assertRaises(TypeError):
            JShort._convertToJava(java.lang.Boolean(1))
        with self.assertRaises(TypeError):
            JInt._convertToJava(java.lang.Boolean(1))
        with self.assertRaises(TypeError):
            JLong._convertToJava(java.lang.Boolean(1))
        with self.assertRaises(TypeError):
            JFloat._convertToJava(java.lang.Boolean(1))
        with self.assertRaises(TypeError):
            JDouble._convertToJava(java.lang.Boolean(1))

    def testBox(self):
        java = jpype.java
        self.assertEqual(java.lang.Boolean(JBoolean(True)), True)
        # FIXME this one fails
        #self.assertEqual(java.lang.Character(JChar("A")), "A")
        self.assertEqual(java.lang.Byte(JByte(123)), 123)
        self.assertEqual(java.lang.Short(JShort(123)), 123)
        self.assertEqual(java.lang.Integer(JInt(123)), 123)
        self.assertEqual(java.lang.Long(JLong(123)), 123)
        self.assertEqual(java.lang.Float(JFloat(123)), 123)
        self.assertEqual(java.lang.Double(JDouble(123)), 123)

    def testCharConversion(self):
        self.assertEqual(JChar._canConvertToJava("a"), "implicit")
        self.assertEqual(JChar._canConvertToJava(bytes([1])), "implicit")
        self.assertEqual(JChar._canConvertToJava(bytes([1,1])), "none")

    def testCharOverflow(self):
        with self.assertRaises(OverflowError):
            JChar(1000000)

    def testCharBytes(self):
        # setArrayRange directly calls Char conversion so it is a good way 
        # to test without checking if conversion is possible first
        ja = JArray(JChar)(2)
        with self.assertRaises(ValueError):
            ja[0:1] = [bytes([1,1,1])]
        with self.assertRaises(OverflowError):
            ja[0:1] = [1000000]
        with self.assertRaises(ValueError):
            ja[0:1] = ["AAA"]
        with self.assertRaises(ValueError):
            ja[0:1] = ["\U0001F600"]
        with self.assertRaises(TypeError):
            ja[0:1] = [ object() ]
        ja[0:1] = ["\u265E"]

