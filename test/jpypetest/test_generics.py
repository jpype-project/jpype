import jpype
import common


class GenericsTestCase(common.JPypeTestCase):
    """Test Java generics support via .typed() method"""

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.ArrayList = jpype.JClass('java.util.ArrayList')
        self.HashMap = jpype.JClass('java.util.HashMap')
        self.String = jpype.JClass('java.lang.String')
        self.Integer = jpype.JClass('java.lang.Integer')

    def testTypedSingleParameter(self):
        """Test creating generic type with single type parameter"""
        StringList = self.ArrayList.typed(self.String)
        self.assertIsNotNone(StringList)
        self.assertEqual(StringList.__name__, 'java.util.ArrayList<java.lang.String>')

    def testTypedMultipleParameters(self):
        """Test creating generic type with multiple type parameters"""
        StringIntMap = self.HashMap.typed(self.String, self.Integer)
        self.assertIsNotNone(StringIntMap)
        self.assertEqual(StringIntMap.__name__, 'java.util.HashMap<java.lang.String,java.lang.Integer>')

    def testTypedCaching(self):
        """Test that generic types are cached"""
        StringList1 = self.ArrayList.typed(self.String)
        StringList2 = self.ArrayList.typed(self.String)
        # Should be the same cached type
        self.assertIs(StringList1, StringList2)

    def testTypedDifferentParameters(self):
        """Test that different type parameters create different types"""
        StringList = self.ArrayList.typed(self.String)
        IntList = self.ArrayList.typed(self.Integer)
        self.assertIsNot(StringList, IntList)
        self.assertNotEqual(StringList.__name__, IntList.__name__)

    def testTypedInheritance(self):
        """Test that generic type inherits from base class"""
        StringList = self.ArrayList.typed(self.String)
        self.assertTrue(issubclass(StringList, self.ArrayList))

    def testTypedHasGenericsAttribute(self):
        """Test that generic types have _generics attribute"""
        StringList = self.ArrayList.typed(self.String)
        self.assertTrue(hasattr(StringList, '_generics'))
        generics = StringList._generics
        self.assertEqual(len(generics), 1)
        self.assertEqual(generics[0], self.String)

    def testTypedMultipleGenericsAttribute(self):
        """Test _generics attribute with multiple parameters"""
        StringIntMap = self.HashMap.typed(self.String, self.Integer)
        generics = StringIntMap._generics
        self.assertEqual(len(generics), 2)
        self.assertEqual(generics[0], self.String)
        self.assertEqual(generics[1], self.Integer)

    def testTypedNoParameters(self):
        """Test that typed() requires at least one parameter"""
        with self.assertRaises(TypeError):
            self.ArrayList.typed()

    def testTypedNonJavaType(self):
        """Test that typed() rejects non-Java types"""
        with self.assertRaises(TypeError):
            self.ArrayList.typed(str)

    def testArrayCreationStillWorks(self):
        """Test that array creation via [] still works"""
        arr = self.String[5]
        self.assertEqual(len(arr), 5)

    def testMultiDimensionalArrayStillWorks(self):
        """Test that multi-dimensional arrays still work"""
        arr = self.String[3, 4]
        self.assertEqual(len(arr), 3)
        self.assertEqual(len(arr[0]), 4)
