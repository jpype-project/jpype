import unittest
import jpype
from . import common

#jpype.startJVM(jpype.getDefaultJVMPath())

class ClosedTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testObjects(self):
        from jpype import java

        s=java.lang.String('foo')
        s._allowed = 1
        try:
            s.forbidden = 1
        except AttributeError:
            pass
        else:
            raise AssertionError("AttributeError not raised")

    def testArrays(self):
        s=jpype.JArray(jpype.JInt)(5)
        # Setting private members is allowed
        s._allowed = 1
        try:
            # Setting public members is prohibited
            s.forbidden = 1
        except AttributeError:
            pass
        else:
            raise AssertionError("AttributeError not raised")

    def testStatic(self):
        static=jpype.JClass('jpype.objectwrapper.StaticTest')
        self.assertEquals(static.i, 1)
        self.assertEquals(static.d, 1.2345)
        self.assertEquals(static.s, "hello")

