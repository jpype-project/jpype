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


class ShutdownParkedDaemonProxyTest(unittest.TestCase):
    """A Python proxy invoked from a *daemon* thread can still be blocked
    inside its Python callback when shutdownJVM() runs: DestroyJavaVM()
    only waits for non-daemon Java threads, so it returns - and frees every
    JPClass/context resource - while that thread is still parked
    underneath it. Releasing the callback afterwards used to resume
    execution on top of a destroyed JVM and segfault. JPype must instead
    detect this in hostInvoke() and get the thread off the JVM permanently
    (terminate it, or park it forever) rather than let it touch freed
    state.  Run out-of-process since a regression here is a hard crash.

    Covers both ways a thread can end up daemon-attached and parked inside
    hostInvoke():
      - java.lang.Thread(runnable): a JVM-native thread, never previously
        seen by Python, that calls straight into the proxy (Java -> Python).
      - a plain Python threading.Thread that calls into Java through a
        Java-typed reference to the same proxy, which routes back into the
        same callback on that same OS thread (Python -> Java -> Python).
        This thread auto-attaches as a daemon via JPContext::getEnv()'s
        AttachCurrentThreadAsDaemon fallback the first time it touches
        Java, so it is exposed to the same race, but arrives at
        hostInvoke() with GIL/thread-state history a java.lang.Thread never
        has (it already had a live PyThreadState before touching Java)."""

    _SPAWN_JAVA_THREAD = (
        "jthread = jpype.java.lang.Thread(runnable)\n"
        "jthread.setDaemon(True)\n"
        "jthread.start()\n"
    )

    _SPAWN_PYTHON_THREAD = (
        "jrunnable = jpype.java.lang.Runnable @ runnable\n"
        "def worker():\n"
        "    jrunnable.run()\n"
        "jthread = threading.Thread(target=worker, daemon=True)\n"
        "jthread.start()\n"
    )

    def _run(self, action, spawn):
        script = (
            "import threading, time, jpype\n"
            "from jpype import JImplements, JOverride\n"
            "jpype.startJVM(convertStrings=False)\n"
            "started = threading.Event()\n"
            "release = threading.Event()\n"
            "@JImplements(jpype.java.lang.Runnable)\n"
            "class Parked:\n"
            "    @JOverride\n"
            "    def run(self):\n"
            "        started.set()\n"
            "        release.wait()\n"
            f"        {action}\n"
            "runnable = Parked()\n"
            f"{spawn}"
            "assert started.wait(timeout=10)\n"
            "time.sleep(0.2)\n"
            "jpype.shutdownJVM()\n"
            "release.set()\n"
            "time.sleep(1)\n"
            "print('SURVIVED')\n"
        )
        result = subprocess.run([sys.executable, "-c", script],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=60)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(b"SURVIVED", result.stdout)
        self.assertIn(b"Application error, blocked python proxy attached as "
                      b"a daemon thread and called after JVM shutdown",
                      result.stderr)

    def testRaiseAfterShutdownJavaThread(self):
        # Callback resumes and raises - exercises the exception-conversion
        # path (JPPythonError::toJava), which needs the freed exception
        # classes.
        self._run("raise RuntimeError('boom after JVM destroyed')",
                  self._SPAWN_JAVA_THREAD)

    def testCallAfterShutdownJavaThread(self):
        # Callback resumes and makes a fresh JNI call - exercises the case
        # where the callback doesn't merely return but tries to use the
        # (destroyed) JVM again.
        self._run("jpype.java.lang.System.currentTimeMillis()",
                  self._SPAWN_JAVA_THREAD)

    def testRaiseAfterShutdownPythonThread(self):
        self._run("raise RuntimeError('boom after JVM destroyed')",
                  self._SPAWN_PYTHON_THREAD)

    def testCallAfterShutdownPythonThread(self):
        self._run("jpype.java.lang.System.currentTimeMillis()",
                  self._SPAWN_PYTHON_THREAD)


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
