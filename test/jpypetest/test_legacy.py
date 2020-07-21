# *****************************************************************************
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   See NOTICE file for details.
#
# *****************************************************************************
import jpype
import common
import subrun
import unittest
import os


def proxy(s):
    if not isinstance(s, str):
        raise TypeError("Fail")
    return s

# This is a test case to exercise all of the paths that pass through
# the string conversion to make sure all are exercised.
@subrun.TestCase
class LegacyTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Run with automatic string conversion
        jpype.startJVM(classpath=os.path.abspath(
            "test/classes"), convertStrings=True)

    def setUp(self):
        self._test = jpype.JClass("jpype.str.Test")
        self._intf = jpype.JClass("jpype.str.StringFunction")

    def testStaticField(self):
        s = self._test.staticField
        self.assertEqual(s, "staticField")
        self.assertIsInstance(s, str)

    def testMemberField(self):
        s = self._test().memberField
        self.assertEqual(s, "memberField")
        self.assertIsInstance(s, str)

    def testStaticMethod(self):
        s = self._test.staticCall()
        self.assertEqual(s, "staticCall")
        self.assertIsInstance(s, str)

    def testMemberMethod(self):
        s = self._test().memberCall()
        self.assertEqual(s, "memberCall")
        self.assertIsInstance(s, str)

    def testArrayItem(self):
        tc = ('apples', 'banana', 'cherries', 'dates', 'elderberry')
        for i in range(0, 5):
            s = self._test.array[i]
            self.assertEqual(s, tc[i])
            self.assertIsInstance(s, str)

    def testArrayRange(self):
        tc = ('apples', 'banana', 'cherries', 'dates', 'elderberry')
        slc = self._test.array[1:-1]
        self.assertEqual(tuple(slc), tc[1:-1])
        self.assertIsInstance(slc[1], str)

    def testProxy(self):
        p = jpype.JProxy([self._intf], dict={'call': proxy})
        r = self._test().callProxy(p, "roundtrip")
        self.assertEqual(r, "roundtrip")
        self.assertIsInstance(r, str)
