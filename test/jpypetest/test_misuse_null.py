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
""" Typed nulls, bad conversions, monitors and shutdown must raise, not crash. """
import subprocess
import sys
import _jpype
import jpype
from jpype.types import *
from jpype import JImplements, JOverride, JConversion
import common


def _run(code):
    p = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True)
    return p


class TypedNullTestCase(common.JPypeTestCase):

    def testFieldOfNull(self):
        null = JObject(None, JClass("java.awt.Point"))
        with self.assertRaises(JClass("java.lang.NullPointerException")):
            null.x
        with self.assertRaises(JClass("java.lang.NullPointerException")):
            null.x = 1

    def testFieldOfInstanceStillWorks(self):
        p = JClass("java.awt.Point")(3, 4)
        self.assertEqual(p.x, 3)
        p.x = 5
        self.assertEqual(p.x, 5)

    def testBufferOfNullArray(self):
        null = JObject(None, JArray(JInt))
        with self.assertRaises(ValueError):
            memoryview(null)
        nullDims = JObject(None, JArray(JInt, 2))
        with self.assertRaises(ValueError):
            memoryview(nullDims)

    def testRaiseNullException(self):
        # "throw null" is a NullPointerException, as in Java.
        @JImplements("java.lang.Runnable")
        class Thrower:
            @JOverride
            def run(self):
                raise JObject(None, JClass("java.lang.IllegalStateException"))

        thread = JClass("java.lang.Thread")(Thrower())
        with self.assertRaises(JClass("java.lang.NullPointerException")):
            thread.run()


class BadConversionTestCase(common.JPypeTestCase):
    """ A @JConversion result is handed to the JVM, so it must be checked. """

    Target = "java.util.concurrent.atomic.AtomicLong"

    def _convert(self, returns):
        # A private Python type keeps the conversion from affecting anything
        # else, since conversions are registered globally.
        class Source:
            pass

        @JConversion(self.Target, exact=Source)
        def convert(jcls, value):
            return returns()

        return JObject(Source(), JClass(self.Target))

    def testPrimitiveForObject(self):
        with self.assertRaises(TypeError):
            self._convert(lambda: JInt(3))

    def testWrongReferenceType(self):
        with self.assertRaises(TypeError):
            self._convert(lambda: JString("not an AtomicLong"))

    def testCorrectType(self):
        obj = self._convert(lambda: JClass(self.Target)(7))
        self.assertEqual(obj.get(), 7)

    def testSubclassIsAccepted(self):
        # AtomicLong is not final, a subclass instance is a valid result.
        sub = JClass("java.util.concurrent.atomic.AtomicLong")(9)
        self.assertEqual(self._convert(lambda: sub).get(), 9)


class MonitorTestCase(common.JPypeTestCase):

    def testUninitializedMonitor(self):
        monitor = _jpype._JMonitor.__new__(_jpype._JMonitor)
        with self.assertRaises(RuntimeError):
            monitor.__enter__()
        with self.assertRaises(RuntimeError):
            monitor.__exit__(None, None, None)

    def testFailedReinit(self):
        monitor = jpype.synchronized(JClass("java.lang.Object")())
        with self.assertRaises(TypeError):
            monitor.__init__(None)
        with self.assertRaises(RuntimeError):
            monitor.__enter__()

    def testStillWorks(self):
        obj = JClass("java.lang.Object")()
        with jpype.synchronized(obj):
            pass


class ShutdownTestCase(common.JPypeTestCase):
    """ Each of these ran in a JVM that was already shut down. """

    def testProxyClassAfterShutdown(self):
        p = _run("import jpype\n"
                 "from jpype import JImplements, JOverride\n"
                 "jpype.startJVM()\n"
                 "@JImplements('java.lang.Runnable')\n"
                 "class R:\n"
                 "    @JOverride\n"
                 "    def run(self): pass\n"
                 "r = R()\n"
                 "jpype.shutdownJVM()\n"
                 "try:\n"
                 "    r.__javaclass__\n"
                 "except Exception as ex:\n"
                 "    print('raised', type(ex).__name__)\n")
        self.assertEqual(p.returncode, 0, p.stderr)
        self.assertIn("raised", p.stdout)

    def testReleasingOneViewKeepsTheOthers(self):
        # The exported buffers of one array share a single view; releasing one
        # after the JVM is gone must not free it under the others.
        p = _run("import jpype\n"
                 "from jpype.types import *\n"
                 "jpype.startJVM()\n"
                 "a = JArray(JInt, 2)([[1, 2], [3, 4]])\n"
                 "views = [memoryview(a) for i in range(4)]\n"
                 "jpype.shutdownJVM()\n"
                 "views[0].release()\n"
                 "views[1].release()\n"
                 "import array\n"
                 "expected = array.array('i', [1, 2, 3, 4]).tobytes()\n"
                 "print('same' if views[2].tobytes() == expected and\n"
                 "      views[3].tobytes() == expected else 'corrupt')\n")
        self.assertEqual(p.returncode, 0, p.stderr)
        self.assertIn("same", p.stdout)
