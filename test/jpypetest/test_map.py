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


class JMapTestCase(common.JPypeTestCase):
    """ Test for methods of java.lang.Map
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testLen(self):
        cls = jpype.JClass('java.util.HashMap')
        obj = cls()
        obj.put("a", 1)
        obj.put("b", 2)
        obj.put("c", 3)
        self.assertEqual(len(obj), 3)

    def testIter(self):
        cls = jpype.JClass('java.util.TreeMap')
        obj = cls()
        obj.put("a", 1)
        obj.put("b", 2)
        obj.put("c", 3)
        self.assertEqual(tuple(i for i in obj), ('a', 'b', 'c'))

    def testGetItem(self):
        cls = jpype.JClass('java.util.HashMap')
        obj = cls()
        obj.put("a", 1)
        obj.put("b", 2)
        obj.put("c", 3)
        self.assertEqual(obj['b'], 2)

    def testSetItem(self):
        cls = jpype.JClass('java.util.HashMap')
        obj = cls()
        obj.put("a", 1)
        obj.put("b", 2)
        obj.put("c", 3)
        obj['b'] = 5
        self.assertEqual(obj['b'], 5)

    def testSetItem(self):
        cls = jpype.JClass('java.util.TreeMap')
        obj = cls()
        obj.put("a", 1)
        obj.put("b", 2)
        obj.put("c", 3)
        del obj['b']
        self.assertEqual(tuple(i for i in obj), ('a', 'c'))
