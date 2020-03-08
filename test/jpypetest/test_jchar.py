import jpype
import common
from jpype.types import *


class JCharTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testCharRange(self):
        self.assertEqual(ord(str(jpype.JChar(65))), 65)
        self.assertEqual(ord(str(jpype.JChar(512))), 512)

    @common.requireInstrumentation
    def testJPChar_new(self):
        _jpype.fault("PyJPChar_new")
        with self.assertRaisesRegex(SystemError, "fault"):
            JChar("a")
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            JChar("a")
        JChar("a")

    @common.requireInstrumentation
    def testJPChar_str(self):
        jc = JChar("a")
        _jpype.fault("PyJPChar_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jc)
        _jpype.fault("PyJPModule_getContext")
        str(jc)
