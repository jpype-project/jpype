import jpype
from jpype import JClass, JInt, JLong, JObject
import common


class BoxingEdgeCaseTestCase(common.JPypeTestCase):
    """Test edge cases for boxed type conversions"""

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.EdgeCaseTest = JClass("jpype.boxing.EdgeCaseTest")

    def testObjectParameter(self):
        """Test that Python int can be passed to Object parameter"""
        # This should work - Python int should convert to Long which is an Object
        result = self.EdgeCaseTest.acceptObject(42)
        self.assertEqual(result, "Object:Long")

    def testMixedPrimitiveAndBoxed(self):
        """Test overload with both primitive int and boxed Integer"""
        # Python int should prefer primitive int over boxed Integer
        result = self.EdgeCaseTest.mixed(42)
        self.assertEqual(result, "int:42")

        # Explicit JInt should still work
        result = self.EdgeCaseTest.mixed(JInt(42))
        self.assertEqual(result, "int:42")

    def testObjectVsBoxed(self):
        """Test that boxed type is preferred over Object for Python int"""
        # Python int should prefer Integer over Object
        result = self.EdgeCaseTest.objectVsBoxed(42)
        self.assertEqual(result, "Integer")

    def testAmbiguousBoxedOverload(self):
        """Test that ambiguous boxed overloads still require explicit cast"""
        # This should be ambiguous and fail
        with self.assertRaises(TypeError) as context:
            self.EdgeCaseTest.ambiguous(42)
        self.assertIn("ambiguous", str(context.exception).lower())

        # But explicit casts should work
        result = self.EdgeCaseTest.ambiguous(JInt(42))
        self.assertEqual(result, "Integer:42")
        result = self.EdgeCaseTest.ambiguous(JLong(42))
        self.assertEqual(result, "Long:42")
