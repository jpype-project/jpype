import jpype
from jpype.types import *
from jpype import JPackage, java
import common


class ReprTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testClass(self):
        cls = JClass("java.lang.String")
        self.assertIsInstance(str(cls), str)
        self.assertIsInstance(repr(cls), str)

    def testMethod(self):
        JS = JClass("java.lang.String")
        method = JS.substring
        self.assertIsInstance(str(method), str)
        self.assertIsInstance(repr(method), str)

    def testField(self):
        JS = JClass("java.lang.String")
        field = JS.__dict__['CASE_INSENSITIVE_ORDER']
        self.assertIsInstance(str(JS.substring), str)
        self.assertIsInstance(repr(JS.substring), str)

    def testMonitor(self):
        JI = JClass("java.lang.Integer")
        with jpype.synchronized(JI) as monitor:
            self.assertIsInstance(str(monitor), str)
            self.assertIsInstance(repr(monitor), str)

    def testArray(self):
        array = JArray(JInt)([1, 2, 3])
        self.assertIsInstance(str(array), str)
        self.assertIsInstance(repr(array), str)

    def testObject(self):
        obj = JObject("abc", JObject)
        self.assertIsInstance(str(obj), str)
        self.assertIsInstance(repr(obj), str)
