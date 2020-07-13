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


class ClosedTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testObjects(self):
        from jpype import java

        s = java.lang.String('foo')
        s._allowed = 1
        try:
            s.forbidden = 1
        except AttributeError:
            pass
        else:
            raise AssertionError("AttributeError not raised")

    def testArrays(self):
        s = jpype.JArray(jpype.JInt)(5)
        # Setting private members is allowed
        s._allowed = 1
        try:
            # Setting public members is prohibited
            s.forbidden = 1
        except AttributeError:
            pass
        else:
            raise AssertionError("AttributeError not raised")

    def testStatic(self):
        static = jpype.JClass('jpype.objectwrapper.StaticTest')
        self.assertEqual(static.i, 1)
        self.assertEqual(static.d, 1.2345)
        self.assertEqual(static.s, "hello")
