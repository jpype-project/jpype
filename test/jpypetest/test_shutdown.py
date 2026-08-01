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
import subprocess
import sys
import unittest
import jpype
import subrun


@subrun.TestCase
class ShutdownSignalRestoreTest(unittest.TestCase):
    """The JVM's fault handlers can be replaced while it runs; shutdownJVM
    must reinstate them before destroying the JVM or the first armed
    safepoint kills the process (see userguide, "Errors reported by Python
    fault handler").  This reproduces the exact kill chain: faulthandler
    enabled before the JVM exists, disabled after, restoring pre-JVM
    handlers over HotSpot's."""

    @classmethod
    def setUpClass(cls):
        import faulthandler
        faulthandler.enable()
        jpype.startJVM(convertStrings=False)
        faulthandler.disable()
        jpype.shutdownJVM()

    def testShutdownSurvived(self):
        # Reaching this line at all means the subprocess survived a shutdown
        # with the JVM's fault handlers clobbered.
        self.assertFalse(jpype.isJVMStarted())


class ShutdownSignalWarningTest(unittest.TestCase):

    def testRestoreWarning(self):
        if sys.platform == "win32":
            raise unittest.SkipTest("POSIX signal handling only")
        script = (
            "import faulthandler, jpype\n"
            "faulthandler.enable()\n"
            "jpype.startJVM(convertStrings=False)\n"
            "faulthandler.disable()\n"
            "jpype.shutdownJVM()\n"
        )
        result = subprocess.run([sys.executable, "-c", script],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=120)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(b"signal handlers were replaced", result.stderr)

    def testPreJVMHandlerRestored(self):
        # The JVM's lifetime must be handler-transparent: a handler installed
        # before startJVM works again after shutdownJVM.  The signal is sent
        # with os.kill so it is delivered asynchronously (no fault context);
        # without the post-destroy restore it would land in the dead JVM's
        # handler instead of Python's.
        if sys.platform == "win32":
            raise unittest.SkipTest("POSIX signal handling only")
        script = (
            "import jpype, os, signal\n"
            "hits = []\n"
            "signal.signal(signal.SIGSEGV, lambda s, f: hits.append(s))\n"
            "jpype.startJVM(convertStrings=False)\n"
            "jpype.shutdownJVM()\n"
            "os.kill(os.getpid(), signal.SIGSEGV)\n"
            "assert hits == [signal.SIGSEGV], hits\n"
            "print('PRE-JVM-RESTORED')\n"
        )
        result = subprocess.run([sys.executable, "-c", script],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=120)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(b"PRE-JVM-RESTORED", result.stdout)


@subrun.TestCase
class ShutdownTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        jpype.startJVM(convertStrings=False)

        # Create some resources
        cls.jstr = jpype.java.lang.String("good morning")
        cls.jobj = jpype.java.lang.Object()
        cls.jcls = jpype.JClass("java.lang.String")
        cls.jarray = jpype.JArray(jpype.JInt)([1, 2, 3, 4])

        # Then blow everything up
        jpype.shutdownJVM()

    def testArrayGet(self):
        with self.assertRaises(jpype.JVMNotRunning):
            type(self).jarray[0]

    def testArraySet(self):
        with self.assertRaises(jpype.JVMNotRunning):
            type(self).jarray[0] = 1

    def testArrayGetSlice(self):
        with self.assertRaises(jpype.JVMNotRunning):
            type(self).jarray[0:2]

    def testArraySetSlice(self):
        with self.assertRaises(jpype.JVMNotRunning):
            type(self).jarray[0:2] = [1, 2]

    def testArrayStr(self):
        with self.assertRaises(jpype.JVMNotRunning):
            str(type(self).jarray)

    def testClassCtor(self):
        with self.assertRaises(jpype.JVMNotRunning):
            obj = type(self).jcls()

    def testObjectInvoke(self):
        with self.assertRaises(jpype.JVMNotRunning):
            type(self).jobj.wait()

    def testObjectStr(self):
        with self.assertRaises(jpype.JVMNotRunning):
            str(type(self).jobj)

    def testStringInvoke(self):
        with self.assertRaises(jpype.JVMNotRunning):
            type(self).jstr.substring(1)

    def testStringStr(self):
        with self.assertRaises(jpype.JVMNotRunning):
            str(type(self).jstr)
