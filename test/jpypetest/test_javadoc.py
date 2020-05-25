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
