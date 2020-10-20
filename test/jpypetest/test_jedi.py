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
import sys
import jpype
import common
import types
import functools
import inspect


have_jedi = False
try:
    import jedi
    have_jedi = (common.version(jedi.__version__) > (0, 14))
except:
    pass


class JediTestCase(common.JPypeTestCase):
    """Test tab completion on JPype objects
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if common.fast:
            raise common.unittest.SkipTest("fast")
        self.cls = jpype.JClass('java.lang.String')
        self.obj = self.cls('foo')

    @common.unittest.skipUnless(have_jedi, "jedi not available")
    def testCompleteClass(self):
        src = 'self.obj.con'
        script = jedi.Interpreter(src, [locals()])
        compl = [i.name for i in script.complete()]
        self.assertEqual(compl, ['concat', 'contains', 'contentEquals'])

    @common.unittest.skipUnless(have_jedi, "jedi not available")
    def testCompleteMethod(self):
        src = 'self.obj.substring(1).con'
        script = jedi.Interpreter(src, [locals()])
        compl = [i.name for i in script.complete()]
        self.assertEqual(compl, ['concat', 'contains', 'contentEquals'])

    @common.unittest.skipUnless(have_jedi, "jedi not available")
    def testCompleteField(self):
        src = 'self.obj.CASE_INSENSITIVE_ORDER.wa'
        script = jedi.Interpreter(src, [locals()])
        compl = [i.name for i in script.complete()]
        self.assertEqual(compl, ['wait'])

    @common.unittest.skipUnless(have_jedi, "jedi not available")
    def testCompleteMethodField(self):
        src = 'self.obj.substring(1).CAS'
        script = jedi.Interpreter(src, [locals()])
        compl = [i.name for i in script.complete()]
        self.assertEqual(compl, ['CASE_INSENSITIVE_ORDER'])
