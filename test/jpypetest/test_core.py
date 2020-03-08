import mock
import jpype
import common
from jpype.types import *


class JCharTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    @mock.patch('jpype._core.sys')
    def testVersion(self, mock_sys):
        mock_sys.version_info = (2, 7)
        with self.assertRaises(ImportError):
            jpype._core.versionTest()
        mock_sys.version_info = (3, 8)
        jpype._core.versionTest()
