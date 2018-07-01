from . import common

try:
    import unittest2 as unittest
except ImportError:
    import unittest


class PropertiesTestCase(common.JPypeTestCase):

    def setUp(self):
        super(PropertiesTestCase, self).setUp()
        self._bean = self.jpype.properties.TestBean()

    @unittest.skip
    def testPropertyPublicMethodOverlap(self):
        self._bean.setProperty1("val")
        self.assertEqual("getsetval", self._bean.getProperty1())

    @unittest.skip
    def testPublicMethodPropertyOverlap(self):
        self.assertEqual("method", self._bean.property1())

    @unittest.skip
    def testPropertyProtectedMethodOverlapInvisibleAttribute(self):
        self._bean.property2 = "val"
        self.assertEqual("getsetval", self._bean.property2)

    @unittest.skip
    def testProtectedMethodPropertyOverlapInvisibleAttribute(self):
        self.assertFalse(hasattr(self._bean.property2, '__call__'))

    @unittest.skip
    def testPropertyProtectedMethodOverlapAttribute(self):
        self._bean.property3 = "val"
        self.assertEqual("getsetval", self._bean.property3)

    @unittest.skip
    def testProtectedMethodPropertyOverlapAttribute(self):
        self.assertFalse(hasattr(self._bean.property3, '__call__'))

    @unittest.skip
    def testPropertyProtectedMethodOverlapAttributeSet(self):
        self._bean.setProperty3("val")
        self.assertEqual("getsetval", self._bean.property3)

    @unittest.skip
    def testPropertyProtectedMethodOverlapAttributeGet(self):
        self._bean.property3 = "val"
        self.assertEqual("getsetval", self._bean.getProperty3())

    @unittest.skip
    def testPrivateAttributeNoThreeCharacterMethodMatchCollision(self):
        self._bean.property4 ="val"
        self.assertEqual("abcval", self._bean.abcProperty4())

    @unittest.skip
    def testPropertyOnlySetter(self):
        self._bean.property5 = "val"
        self.assertEqual("returnsetval", self._bean.returnProperty5())

    @unittest.skip
    def testPropertyOnlySetterSet(self):
        self._bean.setProperty5("val")
        self.assertEqual("setval", self._bean.property5)

    @unittest.skip
    def testPropertyDifferentAttribute(self):
        self._bean.property6 = "val"
        self.assertEqual("getsetval", self._bean.property6)
        self.assertEqual("setval", self._bean.property7)

    @unittest.skip
    def testProertyDifferentAttributeSet(self):
        self._bean.setProperty6("val")
        self.assertEqual("getsetval", self._bean.property6)
        self.assertEqual("setval", self._bean.property7)
