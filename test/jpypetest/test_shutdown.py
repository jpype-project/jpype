import unittest
import jpype
import subrun


@subrun.TestCase
class ShutdownTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        jpype.startJVM(convertStrings=False)

        # Create some resources
        cls.jstr = jpype.java.lang.String("good morning")
        cls.jobj = jpype.java.lang.Object()
        cls.jcls = jpype.JClass("java.lang.String")
        cls.jarray = jpype.JArray(jpype.JInt)([1, 2, 3, 4])

        # Then blow everything up
        jpype.shutdownJVM()

    def testArrayGet(self):
        with self.assertRaises(RuntimeError):
            type(self).jarray[0]

    def testArraySet(self):
        with self.assertRaises(RuntimeError):
            type(self).jarray[0] = 1

    def testArrayGetSlice(self):
        with self.assertRaises(RuntimeError):
            type(self).jarray[0:2]

    def testArraySetSlice(self):
        with self.assertRaises(RuntimeError):
            type(self).jarray[0:2] = [1, 2]

    def testArrayStr(self):
        with self.assertRaises(RuntimeError):
            str(type(self).jarray)

    def testClassCtor(self):
        with self.assertRaises(RuntimeError):
            obj = type(self).jcls()

    def testObjectInvoke(self):
        with self.assertRaises(RuntimeError):
            type(self).jobj.wait()

    def testObjectStr(self):
        with self.assertRaises(RuntimeError):
            str(type(self).jobj)

    def testStringInvoke(self):
        with self.assertRaises(RuntimeError):
            type(self).jstr.substring(1)

    def testStringStr(self):
        with self.assertRaises(RuntimeError):
            str(type(self).jstr)
