import common
import jpype
import _jpype


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

