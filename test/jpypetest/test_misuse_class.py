# *****************************************************************************
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0
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
""" Misuse of the class and module API must raise, not crash the interpreter. """
import subprocess
import sys
import _jpype
import jpype
from jpype.types import *
import common


class AbstractBaseTestCase(common.JPypeTestCase):
    """ JArray, JException and JInterface have no Java class of their own. """

    def testPrivateMethods(self):
        for base in (JArray, JException, jpype.JInterface):
            for name in ("_canCast", "_cast", "_canConvertToJava", "_convertToJava"):
                with self.assertRaises(TypeError, msg="%s.%s" % (base.__name__, name)):
                    getattr(base, name)(1)

    def testHints(self):
        for base in (JArray, JException, jpype.JInterface):
            self.assertIsNone(base._hints)
            with self.assertRaises(AttributeError):
                base._hints = object()

    def testMatmul(self):
        with self.assertRaises(TypeError):
            JArray @ 1


class ReflectedMatmulTestCase(common.JPypeTestCase):
    """ ``obj @ JClass`` calls the slot with the left operand as ``self``. """

    def testClass(self):
        with self.assertRaises(TypeError):
            [1, 2] @ JString
        with self.assertRaises(TypeError):
            1 @ JClass("java.lang.Object")

    def testPackage(self):
        with self.assertRaises(TypeError):
            1 @ jpype.JPackage("java")
        with self.assertRaises(TypeError):
            [1] @ jpype.JPackage("java")

    def testStillCasts(self):
        # The forward direction is unaffected.
        self.assertIsInstance(JClass("java.lang.Object") @ JString("a"),
                              JClass("java.lang.Object"))


class SubclassCheckTestCase(common.JPypeTestCase):

    def testNonTypeArgument(self):
        # A java.lang.Class instance is an object, not a Python type.
        with self.assertRaises(TypeError):
            issubclass(JClass("java.lang.String").class_, _jpype._JNumberLong)

    def testStillWorks(self):
        self.assertTrue(issubclass(JClass("java.lang.Integer"), JClass("java.lang.Number")))
        self.assertFalse(issubclass(JClass("java.lang.String"), JClass("java.lang.Number")))


class ModuleFunctionTestCase(common.JPypeTestCase):

    def testCollectArguments(self):
        with self.assertRaises(TypeError):
            _jpype._collect()
        with self.assertRaises(TypeError):
            _jpype._collect("x")


class GcStatsNotRunningTestCase(common.JPypeTestCase):
    """ gcStats() needs a running JVM; that is an exception, not an abort. """

    def testBeforeStart(self):
        code = ("import jpype, _jpype\n"
                "try:\n"
                "    _jpype.gcStats()\n"
                "except Exception as ex:\n"
                "    print(type(ex).__name__)\n")
        p = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True)
        self.assertEqual(p.returncode, 0, p.stderr)
        self.assertIn("JVMNotRunning", p.stdout)

    def testAfterShutdown(self):
        code = ("import jpype, _jpype\n"
                "jpype.startJVM()\n"
                "jpype.shutdownJVM()\n"
                "try:\n"
                "    _jpype.gcStats()\n"
                "except Exception as ex:\n"
                "    print(type(ex).__name__)\n")
        p = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True)
        self.assertEqual(p.returncode, 0, p.stderr)
        self.assertIn("JVMNotRunning", p.stdout)
