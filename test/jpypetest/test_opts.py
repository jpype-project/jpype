import sys
import jpype
import common

# This is an example of how to pass arguments to pytest.
# Options are defined in the file conftest.py and
# passed the a fixture function.  This fixture function
# is then applied to each TestCase class.
#
# class OptsTestCase(common.JPypeTestCase):
#
#    def setUp(self):
#        common.JPypeTestCase.setUp(self)
#
#    def testOpts(self):
#        self.assertTrue(hasattr(self, "_jar"))
#        self.assertTrue(hasattr(self, "_convertStrings"))
#
#    def testConvertStrings(self):
#        self.assertTrue(self._convertStrings)
#
#    def testJar(self):
#        self.assertEqual(self._jar, "foo")
