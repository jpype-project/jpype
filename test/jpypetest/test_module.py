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
# Tests for module functionality including failures that cannot
# be triggered in normal operations
import _jpype
import jpype
import unittest
import subrun
import common
import unittest.mock as mock


@subrun.TestCase(individual=True)
class ModuleStartTestCase(unittest.TestCase):

    def testNoJObject(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.JObject
            jpype.startJVM()

    def testNoJInterface(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.JInterface
            jpype.startJVM()

    def testNoJArray(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.JArray
            jpype.startJVM()

    def testNoJException(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.JException
            jpype.startJVM()

    def testNoJClassPre(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype._jclassPre
            jpype.startJVM()

    def testNoJClassPost(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype._jclassPost
            jpype.startJVM()

    def testNoMethodAnnotations(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.getMethodAnnotations
            jpype.startJVM()

    def testNoMethodCode(self):
        with self.assertRaises(RuntimeError):
            import jpype
            del _jpype.getMethodCode
            jpype.startJVM()

    def testShutdown(self):
        import jpype
        jpype.startJVM(convertStrings=False)
        jpype.shutdownJVM()


class ModuleTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testIsPackage(self):
        self.assertTrue(_jpype.isPackage("java"))
        self.assertFalse(_jpype.isPackage("jva"))
        with self.assertRaises(TypeError):
            _jpype.isPackage(object())

    def testGetClass(self):
        self.assertIsInstance(_jpype._getClass("java.lang.String"), _jpype._JClass)
        with self.assertRaises(TypeError):
            _jpype._getClass(object())

    def testHasClass(self):
        self.assertTrue(_jpype._hasClass("java.lang.String"))
        with self.assertRaises(TypeError):
            _jpype._hasClass(object())


class JInitTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testJInit(self):
        with mock.patch("_jpype.isStarted") as started:
            started.return_value = False
            self.assertEqual(len(jpype._jinit.JInitializers), 0)
            A = []

            def func():
                A.append(1)
            jpype.onJVMStart(func)
            self.assertEqual(len(A), 0)
            self.assertEqual(jpype._jinit.JInitializers[0], func)
            started.return_value = True
            jpype.onJVMStart(func)
            self.assertEqual(len(A), 1)
            jpype._jinit.runJVMInitializers()
            self.assertEqual(len(A), 2)
