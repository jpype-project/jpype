import jpype
from jpype import JClass, JInt, JLong, JShort, JDouble, JFloat
import common


class BoxingTestCase(common.JPypeTestCase):
    """Test issue #1098: Implicit conversion to boxed types"""

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.BoxingTest = JClass("jpype.boxing.BoxingTest")

    def testBoxedIntegerFromPythonInt(self):
        """Test that Python int can implicitly convert to Integer"""
        result = self.BoxingTest.fnBoxedInteger(42)
        self.assertEqual(result, "Integer:42")

    def testBoxedLongFromPythonInt(self):
        """Test that Python int can implicitly convert to Long"""
        result = self.BoxingTest.fnBoxedLong(42)
        self.assertEqual(result, "Long:42")

    def testBoxedShortFromPythonInt(self):
        """Test that Python int can implicitly convert to Short"""
        result = self.BoxingTest.fnBoxedShort(42)
        self.assertEqual(result, "Short:42")

    def testBoxedDoubleFromPythonFloat(self):
        """Test that Python float can implicitly convert to Double"""
        result = self.BoxingTest.fnBoxedDouble(42.5)
        self.assertEqual(result, "Double:42.5")

    def testBoxedFloatFromPythonFloat(self):
        """Test that Python float can implicitly convert to Float"""
        result = self.BoxingTest.fnBoxedFloat(42.5)
        self.assertEqual(result, "Float:42.5")

    def testOverloadDisambiguationWithCast(self):
        """Test that explicit casts can still disambiguate overloads"""
        # These should work with explicit casts
        self.assertEqual(self.BoxingTest.overloaded(JShort(42)), "Short:42")
        self.assertEqual(self.BoxingTest.overloaded(JInt(42)), "Integer:42")
        self.assertEqual(self.BoxingTest.overloaded(JLong(42)), "Long:42")
