import jpype
import common


class JStringTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testAddself(self):
        a = jpype.JString("abc")
        a = a + "def"
        self.assertEqual(a, "abcdef")

    def testEq(self):
        a = jpype.JString("abc")
        b = jpype.JClass("java.lang.String")("abc")
        self.assertEqual(a, b)

    def testEqPy(self):
        a = jpype.JString("abc")
        self.assertEqual(a, "abc")

    def testNotEq(self):
        a = jpype.JString("abc")
        b = jpype.JClass("java.lang.String")("def")
        self.assertNotEqual(a, b)

    def testNotEqPy(self):
        a = jpype.JString("abc")
        self.assertNotEqual(a, "def")

    def testLen(self):
        a = jpype.JString("abc")
        self.assertEqual(len(a), 3)

    def testGetItem(self):
        a = jpype.JString("abc")
        self.assertEqual(a[0], 'a')
        self.assertEqual(a[1], 'b')
        self.assertEqual(a[2], 'c')
        self.assertEqual(a[-1], 'c')
        self.assertEqual(a[-2], 'b')
        self.assertEqual(a[-3], 'a')
        with self.assertRaises(IndexError):
            x = a[3]
        with self.assertRaises(IndexError):
            x = a[-4]

    def testLt(self):
        a = jpype.JString("abc")
        b = jpype.JString("def")
        self.assertTrue(a < b)
        self.assertFalse(b < a)
        self.assertFalse(b < "def")
        self.assertTrue(a < "def")

    def testLe(self):
        a = jpype.JString("abc")
        b = jpype.JString("def")
        self.assertTrue(a <= a)
        self.assertTrue(a <= b)
        self.assertFalse(b <= a)
        self.assertTrue(b <= "def")
        self.assertTrue(a <= "def")

    def testGt(self):
        a = jpype.JString("abc")
        b = jpype.JString("def")
        self.assertFalse(a < a)
        self.assertTrue(a < b)
        self.assertFalse(b < a)
        self.assertFalse(b < "def")
        self.assertTrue(a < "def")

    def testGe(self):
        a = jpype.JString("abc")
        b = jpype.JString("def")
        self.assertTrue(a >= a)
        self.assertFalse(a >= b)
        self.assertTrue(b >= a)
        self.assertTrue(b >= "def")
        self.assertFalse(a >= "def")

    def testContains(self):
        a = jpype.JString("abc")
        self.assertTrue("ab" in a)
        self.assertFalse("cd" in a)

    def testHash(self):
        a = jpype.JString("abc")
        self.assertEqual(hash(a), hash("abc"))

    def __repr__(self):
        a = jpype.JString("abc")
        self.assertEqual(repr(a), "'abc'")
