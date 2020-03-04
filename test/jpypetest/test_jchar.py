import _jpype
import jpype
import common
from jpype.types import *

class JCharTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testCharRange(self):
        self.assertEqual(ord(str(jpype.JChar(65))), 65)
        self.assertEqual(ord(str(jpype.JChar(512))), 512)


