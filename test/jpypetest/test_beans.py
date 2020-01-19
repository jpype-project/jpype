# Tests for beans
import jpype
import unittest
import os
import sys
import subrun
from subrun import *

##############################################################################
# Test methods


def startup(path):
    import jpype
    import jpype.beans
    jpype.startJVM(classpath=path, convertStrings=False)


def runHasProperties():
    cls = jpype.JClass("jpype/properties/TestBean")
    obj = cls()
    assertTrue(isinstance(cls.__dict__['propertyMember'], property))
    assertTrue(isinstance(cls.__dict__['readOnly'], property))
    assertTrue(isinstance(cls.__dict__['writeOnly'], property))
    assertTrue(isinstance(cls.__dict__['with_'], property))


def runPropertyMember():
    # Test member
    obj = jpype.JClass("jpype/properties/TestBean")()
    obj.propertyMember = "q"
    assertEqual(obj.propertyMember, "q")


def runPropertyKeyword():
    # Test keyword property
    obj = jpype.JClass("jpype/properties/TestBean")()
    obj.with_ = "a"
    assertEqual(obj.with_, "a")
    assertEqual(obj.m5, "a")


def runPropertyReadOnly():
    # Test readonly
    obj = jpype.JClass("jpype/properties/TestBean")()
    obj.m3 = "b"
    assertEqual(obj.readOnly, "b")
    with assertRaises(AttributeError):
        obj.readOnly = "c"


def runPropertyWriteOnly():
    # Test writeonly
    obj = jpype.JClass("jpype/properties/TestBean")()
    obj.writeOnly = "c"
    assertEqual(obj.m4, "c")
    with assertRaises(AttributeError):
        x = obj.writeOnly


def runNoProperties():
    cls = jpype.JClass("jpype/properties/TestBean")
    with assertRaises(KeyError):
        cls.__dict__['failure1']
    with assertRaises(KeyError):
        cls.__dict__['failure2']

##############################################################################
#  Test runner


class BeansTest(unittest.TestCase):

    def setup(self):
        self.client = BeansTest.client

    @classmethod
    def setUpClass(cls):
        BeansTest.client = subrun.Client()
        BeansTest.client.execute(startup, os.path.abspath("test/classes"))

    @classmethod
    def tearDownClass(cls):
        cls.client.stop()

    def testHasProperties(self):
        self.client.execute(runHasProperties)

    def testPropertyMember(self):
        self.client.execute(runPropertyMember)

    def testPropertyKeyword(self):
        self.client.execute(runPropertyKeyword)

    def testPropertyReadOnly(self):
        self.client.execute(runPropertyReadOnly)

    def testPropertyWriteOnly(self):
        self.client.execute(runPropertyWriteOnly)

    def testNoProperties(self):
        self.client.execute(runNoProperties)
