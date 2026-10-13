import jpype
from jpype import JClass, JInt, JBoolean
import common


class BoxingComprehensiveTestCase(common.JPypeTestCase):
    """Comprehensive edge case tests for boxed type conversions"""

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.ComprehensiveTest = JClass("jpype.boxing.ComprehensiveTest")

    def testNullToBoxedType(self):
        """Test that None can still be passed to boxed types"""
        result = self.ComprehensiveTest.acceptInteger(None)
        self.assertEqual(result, "null")

    def testBooleanBoxing(self):
        """Test that Python bool works with Boolean"""
        result = self.ComprehensiveTest.acceptBoolean(True)
        self.assertEqual(result, "Boolean:true")

    def testListOfBoxedTypes(self):
        """Test that lists work with boxed type parameters"""
        ArrayList = JClass("java.util.ArrayList")
        list = ArrayList()
        list.add(42)
        list.add(43)
        result = self.ComprehensiveTest.acceptList(list)
        self.assertEqual(result, "List<Integer>:size=2")

    def testVarargsWithBoxedTypes(self):
        """Test varargs with boxed types"""
        result = self.ComprehensiveTest.varargs(1, 2, 3)
        self.assertEqual(result, "varargs:count=3")

    def testReturnBoxedType(self):
        """Test that boxed return values work"""
        result = self.ComprehensiveTest.returnBoxed(42)
        self.assertIsNotNone(result)
        # Result should be a Java Integer
        self.assertEqual(int(result), 42)

    def testNumberHierarchy(self):
        """Test that Integer works with Number parameter"""
        # Python int should convert to Integer which is a Number
        result = self.ComprehensiveTest.acceptNumber(42)
        # Note: Python int defaults to Long in Java
        self.assertIn(result, ["Number:Long", "Number:Integer"])

    def testComparableInterface(self):
        """Test that Integer works with Comparable interface"""
        # Create an actual Integer object
        Integer = JClass("java.lang.Integer")
        intObj = Integer.valueOf(42)
        result = self.ComprehensiveTest.acceptComparable(intObj)
        self.assertEqual(result, "Comparable")
