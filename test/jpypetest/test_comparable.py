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
