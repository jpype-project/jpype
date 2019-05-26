import jpype
import common

try:
    import unittest2 as unittest
except ImportError:
    import unittest


class PropertiesTestCase(common.JPypeTestCase):
    #    __name__="PropertiesTestCase"

    def setUp(self):
        super(PropertiesTestCase, self).setUp()
        self._bean = jpype.JClass('jpype.properties.TestBean')()

    @unittest.skip("properties disabled")
    def testPropertyPublicMethodOverlap(self):
        self._bean.setProperty1("val")
        self.assertEqual("getsetval", self._bean.getProperty1())

    @unittest.skip("properties disabled")
    def testPublicMethodPropertyOverlap(self):
        self.assertEqual("method", self._bean.property1())

    @unittest.skip("properties disabled")
    def testPropertyProtectedMethodOverlapInvisibleAttribute(self):
        self._bean.property2 = "val"
        self.assertEqual("getsetval", self._bean.property2)

    @unittest.skip("properties disabled")
    def testProtectedMethodPropertyOverlapInvisibleAttribute(self):
        self.assertFalse(hasattr(self._bean.property2, '__call__'))

    @unittest.skip("properties disabled")
    def testPropertyProtectedMethodOverlapAttribute(self):
        self._bean.property3 = "val"
        self.assertEqual("getsetval", self._bean.property3)

    @unittest.skip("properties disabled")
    def testProtectedMethodPropertyOverlapAttribute(self):
        self.assertFalse(hasattr(self._bean.property3, '__call__'))

    @unittest.skip("properties disabled")
    def testPropertyProtectedMethodOverlapAttributeSet(self):
        self._bean.setProperty3("val")
        self.assertEqual("getsetval", self._bean.property3)

    @unittest.skip("properties disabled")
    def testPropertyProtectedMethodOverlapAttributeGet(self):
        self._bean.property3 = "val"
        self.assertEqual("getsetval", self._bean.getProperty3())

    @unittest.skip("properties disabled")
    def testPrivateAttributeNoThreeCharacterMethodMatchCollision(self):
        self._bean.property4 = "val"
        self.assertEqual("abcval", self._bean.abcProperty4())

    @unittest.skip("properties disabled")
    def testPropertyOnlySetter(self):
        self._bean.property5 = "val"
        self.assertEqual("returnsetval", self._bean.returnProperty5())

    @unittest.skip("properties disabled")
    def testPropertyOnlySetterSet(self):
        self._bean.setProperty5("val")
        self.assertEqual("setval", self._bean.property5)

    @unittest.skip("properties disabled")
    def testPropertyDifferentAttribute(self):
        self._bean.property6 = "val"
        self.assertEqual("getsetval", self._bean.property6)
        self.assertEqual("setval", self._bean.property7)

    @unittest.skip("properties disabled")
    def testProertyDifferentAttributeSet(self):
        self._bean.setProperty6("val")
        self.assertEqual("getsetval", self._bean.property6)
        self.assertEqual("setval", self._bean.property7)
