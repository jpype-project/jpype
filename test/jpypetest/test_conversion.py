import common
import jpype
import _jpype


class ConversionTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.jc1 = jpype.JClass("java.lang.String")
        self.jc2 = jpype.JClass("java.lang.Integer")

    def testCanConvertExact(self):
        self.assertEqual(self.jc1.__javaclass__.canConvertToJava("a"), "exact")

    def testCanConvertNone(self):
        self.assertEqual(self.jc1.__javaclass__.canConvertToJava(1), "none")

    def testCanConvertExplicit(self):
        self.assertEqual(
            self.jc2.__javaclass__.canConvertToJava(1), "explicit")

    def testCanConvertImplicit(self):
        self.assertEqual(
            self.jc1.__javaclass__.canConvertToJava(None), "implicit")

    def testConvertExact(self):
        self.assertIsInstance(
            self.jc1.__javaclass__.convertToJava("a"), _jpype.PyJPValue)

    def testConvertImplicit(self):
        self.assertIsInstance(
            self.jc1.__javaclass__.convertToJava(None), _jpype.PyJPValue)

    def testConvertExplicit(self):
        self.assertIsInstance(
            self.jc2.__javaclass__.convertToJava(1), _jpype.PyJPValue)

    def testConvertNone(self):
        with self.assertRaises(TypeError):
            self.jc1.__javaclass__.convertToJava(1)
