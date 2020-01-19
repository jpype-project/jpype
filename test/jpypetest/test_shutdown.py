import unittest
import jpype
import subrun

jstr = None
jarray = None
jcls = None
jobj = None


def startup():
    jpype.startJVM(convertStrings=False)
    global jstr, jarray, jcls, jobj

    # Create some resources
    jstr = jpype.java.lang.String("good morning")
    jobj = jpype.java.lang.Object()
    jcls = jpype.JClass("java.lang.String")
    jarray = jpype.JArray(jpype.JInt)([1, 2, 3, 4])

    # Then blow everything up
    jpype.shutdownJVM()

    # Now run our tests on the otherside of the looking glass


def runArrayGet():
    jarray[0]


def runArraySet():
    jarray[0] = 1


def runArrayGetSlice():
    jarray[0:2]


def runArraySetSlice():
    jarray[0:2] = [1, 2]


def runArrayStr():
    str(jarray)


def runClassCtor():
    obj = jcls()


def runObjectStr():
    str(jobj)


def runObjectInvoke():
    jobj.wait()


def runStringStr():
    str(jstr)


def runStringInvoke():
    jstr.substring(1)


class ShutdownTest(unittest.TestCase):

    def setup(self):
        self.client = ShutdownTest.client

    @classmethod
    def setUpClass(cls):
        cls.client = subrun.Client()
        cls.client.execute(startup)

    @classmethod
    def tearDownClass(cls):
        cls.client.stop()

    def testArrayGet(self):
        with self.assertRaises(RuntimeError):
            self.client.execute(runArrayGet)

    def testArraySet(self):
        with self.assertRaises(RuntimeError):
            self.client.execute(runArraySet)

    def testArrayGetSlice(self):
        with self.assertRaises(RuntimeError):
            self.client.execute(runArrayGetSlice)

    def testArraySetSlice(self):
        with self.assertRaises(RuntimeError):
            self.client.execute(runArraySetSlice)

    def testArrayStr(self):
        with self.assertRaises(RuntimeError):
            self.client.execute(runArrayStr)

    def testClassCtor(self):
        with self.assertRaises(RuntimeError):
            self.client.execute(runClassCtor)

    def testObjectInvoke(self):
        with self.assertRaises(RuntimeError):
            self.client.execute(runObjectInvoke)

    def testObjectStr(self):
        with self.assertRaises(RuntimeError):
            self.client.execute(runObjectStr)

    def testStringInvoke(self):
        with self.assertRaises(RuntimeError):
            self.client.execute(runStringInvoke)

    def testStringStr(self):
        with self.assertRaises(RuntimeError):
            self.client.execute(runStringStr)
