# Tests for beans
import jpype
import unittest
import os
import sys
import subrun

@subrun.TestCase
class BeansTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        import jpype
        import jpype.beans
        jpype.startJVM(classpath=os.path.abspath("test/classes"), convertStrings=False)

    def testHasProperties(self):
        cls = jpype.JClass("jpype/properties/TestBean")
        obj = cls()
        self.assertTrue(isinstance(cls.__dict__['propertyMember'], property))
        self.assertTrue(isinstance(cls.__dict__['readOnly'], property))
        self.assertTrue(isinstance(cls.__dict__['writeOnly'], property))
        self.assertTrue(isinstance(cls.__dict__['with_'], property))

    def testPropertyMember(self):
        obj = jpype.JClass("jpype/properties/TestBean")()
        obj.propertyMember = "q"
        self.assertEqual(obj.propertyMember, "q")

    def testPropertyKeyword(self):
        obj = jpype.JClass("jpype/properties/TestBean")()
        obj.with_ = "a"
        self.assertEqual(obj.with_, "a")
        self.assertEqual(obj.m5, "a")

    def testPropertyReadOnly(self):
        # Test readonly
        obj = jpype.JClass("jpype/properties/TestBean")()
        obj.m3 = "b"
        self.assertEqual(obj.readOnly, "b")
        with self.assertRaises(AttributeError):
            obj.readOnly = "c"

    def testPropertyWriteOnly(self):
        # Test writeonly
        obj = jpype.JClass("jpype/properties/TestBean")()
        obj.writeOnly = "c"
        self.assertEqual(obj.m4, "c")
        with self.assertRaises(AttributeError):
            x = obj.writeOnly

    def testNoProperties(self):
        cls = jpype.JClass("jpype/properties/TestBean")
        with self.assertRaises(KeyError):
            cls.__dict__['failure1']
        with self.assertRaises(KeyError):
            cls.__dict__['failure2']

