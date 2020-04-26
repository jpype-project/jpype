import sys
import jpype
import common


class ComparableTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testComparable(self):
        a = jpype.java.time.Instant.ofEpochSecond(10000000)
        b = jpype.java.time.Instant.ofEpochSecond(10000001)
        self.assertFalse(a < a)
        self.assertFalse(a > a)
        self.assertTrue(a >= a)
        self.assertTrue(a <= a)
        self.assertTrue(a == a)
        self.assertFalse(a != a)
        self.assertTrue(a < b)
        self.assertFalse(a > b)
        self.assertFalse(a >= b)
        self.assertTrue(a <= b)
        self.assertFalse(a == b)
        self.assertTrue(a != b)

    def testComparableHash(self):
        i = jpype.java.math.BigInteger("1000000000000")
        self.assertIsInstance(hash(i), int)

    def testComparableNull(self):
        Instant = jpype.JClass("java.time.Instant")
        i1 = Instant.parse("1970-01-01T00:00:00Z")
        i3 = jpype.JObject(None, Instant)

        self.assertTrue(i1 == i1)
        self.assertFalse(i1 == i3)
        self.assertFalse(i3 == i1)
        self.assertTrue(i1 != i3)
        self.assertTrue(i3 != i1)
        with self.assertRaises(ValueError):
            print(i1 < i3)
        with self.assertRaises(ValueError):
            print(i1 <= i3)
        with self.assertRaises(ValueError):
            print(i1 > i3)
        with self.assertRaises(ValueError):
            print(i1 >= i3)
