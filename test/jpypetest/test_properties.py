# *****************************************************************************
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   See NOTICE file for details.
#
# *****************************************************************************
import subrun
import unittest
import jpype


@subrun.TestCase
class PropertiesTestCase(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        import jpype.beans
        jpype.startJVM(classpath="test/classes", convertStrings=False)

    def setUp(self):
        self._bean = jpype.JClass('jpype.properties.TestBean')()

    def testPropertyPublicMethodOverlap(self):
        self._bean.setProperty1("val")
        self.assertEqual("getsetval", self._bean.getProperty1())

    def testPublicMethodPropertyOverlap(self):
        self.assertEqual("method", self._bean.property1())

    def testPropertyProtectedMethodOverlapInvisibleAttribute(self):
        self._bean.property2 = "val"
        self.assertEqual("getsetval", self._bean.property2)

    def testProtectedMethodPropertyOverlapInvisibleAttribute(self):
        self.assertFalse(hasattr(self._bean.property2, '__call__'))

    def testPropertyProtectedMethodOverlapAttribute(self):
        self._bean.property3 = "val"
        self.assertEqual("getsetval", self._bean.property3)

    def testProtectedMethodPropertyOverlapAttribute(self):
        self.assertFalse(hasattr(self._bean.property3, '__call__'))

    def testPropertyProtectedMethodOverlapAttributeSet(self):
        self._bean.setProperty3("val")
        self.assertEqual("getsetval", self._bean.property3)

    def testPropertyProtectedMethodOverlapAttributeGet(self):
        self._bean.property3 = "val"
        self.assertEqual("getsetval", self._bean.getProperty3())

    def testPrivateAttributeNoThreeCharacterMethodMatchCollision(self):
        self._bean.property4 = "val"
        self.assertEqual("abcval", self._bean.abcProperty4())

    def testPropertyOnlySetter(self):
        self._bean.property5 = "val"
        self.assertEqual("returnsetval", self._bean.returnProperty5())

    def testPropertyOnlySetterSet(self):
        self._bean.setProperty5("val")
        with self.assertRaises(AttributeError):
            self.assertEqual("setval", self._bean.property5)

    def testPropertyDifferentAttribute(self):
        self._bean.property6 = "val"
        self.assertEqual("getsetval", self._bean.property6)
        self.assertEqual("setval", self._bean.property7)

    def testProertyDifferentAttributeSet(self):
        self._bean.setProperty6("val")
        self.assertEqual("getsetval", self._bean.property6)
        self.assertEqual("setval", self._bean.property7)

    def testHasProperties(self):
        cls = jpype.JClass("jpype.properties.TestBean")
        obj = cls()
        self.assertTrue(isinstance(cls.__dict__['propertyMember'], property))
        self.assertTrue(isinstance(cls.__dict__['readOnly'], property))
        self.assertTrue(isinstance(cls.__dict__['writeOnly'], property))
        self.assertTrue(isinstance(cls.__dict__['with_'], property))

    def testPropertyMember(self):
        obj = jpype.JClass("jpype.properties.TestBean")()
        obj.propertyMember = "q"
        self.assertEqual(obj.propertyMember, "q")

    def testPropertyKeyword(self):
        obj = jpype.JClass("jpype.properties.TestBean")()
        obj.with_ = "a"
        self.assertEqual(obj.with_, "a")
        self.assertEqual(obj.m5, "a")

    def testPropertyReadOnly(self):
        # Test readonly
        obj = jpype.JClass("jpype.properties.TestBean")()
        obj.m3 = "b"
        self.assertEqual(obj.readOnly, "b")
        with self.assertRaises(AttributeError):
            obj.readOnly = "c"

    def testPropertyWriteOnly(self):
        # Test writeonly
        obj = jpype.JClass("jpype.properties.TestBean")()
        obj.writeOnly = "c"
        self.assertEqual(obj.m4, "c")
        with self.assertRaises(AttributeError):
            x = obj.writeOnly

    def testNoProperties(self):
        cls = jpype.JClass("jpype.properties.TestBean")
        with self.assertRaises(KeyError):
            cls.__dict__['failure1']
        with self.assertRaises(KeyError):
            cls.__dict__['failure2']
