import jpype
from jpype.types import *
from jpype import java
import common


class HtmlTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testEntities(self):
        html = JClass("org.jpype.html.Html")
        for k, v in html.ENTITIES.items():
            u = html.decode("&" + str(k) + ";")
            self.assertIsInstance(u, JString)
            self.assertEqual(len(u), 1)
            self.assertEqual(ord(u[0][0]), v)

    def testClass(self):
        JC = jpype.JClass("jpype.doc.Test")
        jd = JC.__doc__
        self.assertIsInstance(jd, str)
        self.assertRegex(jd, "random stuff")

    def testMethod(self):
        JC = jpype.JClass("jpype.doc.Test")
        jd = JC.methodOne.__doc__
        self.assertIsInstance(jd, str)
        self.assertRegex(jd, "something special")
