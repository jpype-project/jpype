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
import _jpype
import common


class DocStringTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testDocClass(self):
        cls = jpype.JClass('java.util.Iterator')
        self.assertIsNotNone(cls.__doc__)
        cls = jpype.JClass('java.lang.String')
        self.assertIsNotNone(cls.__doc__)

    def testDocMethod(self):
        cls = jpype.JClass('java.lang.String')
        self.assertIsNotNone(cls.substring.__doc__)

    def testDocEnumClass(self):
        cls = jpype.JClass('java.lang.Character.UnicodeScript')
        self.assertIsNotNone(cls.__doc__)

    def testDocUnbound(self):
        # __doc__ on the metaclasses themselves (not on one of their
        # instances) must behave like an ordinary str-or-None attribute,
        # rather than leaking the raw getset descriptor object used to
        # implement per-class dynamic Javadoc lookup. This is what tools
        # doing full introspection (e.g. mypy stubgen) hit. See #1213.
        for doc in (_jpype._JClass.__doc__, _jpype._JMethod.__doc__):
            self.assertTrue(doc is None or isinstance(doc, str))
