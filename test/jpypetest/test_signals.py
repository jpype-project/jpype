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
import glob
import os
import signal
import subprocess
import sys
import tempfile
import threading
import unittest
import jpype
import subrun


@subrun.TestCase
class SignalsTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # set up signal handling before starting jpype
        cls.sigint_event = threading.Event()
        cls.sigterm_event = threading.Event()

        def sigint_handler(sig, frame):
            cls.sigint_event.set()

        def sigterm_handler(sig, frame):
            cls.sigterm_event.set()

        signal.signal(signal.SIGINT, sigint_handler)
        signal.signal(signal.SIGTERM, sigterm_handler)

        # start jpype with interrupt=False to pass back control to python
        jpype.startJVM(interrupt=False)

    def setUp(self):
        if sys.platform == "win32":
            raise unittest.SkipTest("signals test not applicable on windows")
        self.sigint_event.clear()
        self.sigterm_event.clear()

    def testSigInt(self):
        os.kill(os.getpid(), signal.SIGINT)

        # the test is executed in the main thread. The signal cannot interrupt the threading.Event.wait() call
        # so asserting the return value of `wait` does not work.
        # However, after returning from the wait, the control should go to the signal handler, and the next `is_set`
        # call will reflect the actual value of the flag.
        self.sigint_event.wait(0.1)
        self.assertTrue(self.sigint_event.is_set())
        self.assertFalse(self.sigterm_event.is_set())

    def testSigTerm(self):
        os.kill(os.getpid(), signal.SIGTERM)

        self.sigterm_event.wait(0.1)
        if sys.version_info < (3, 10):
            # python versions below 3.10 do not support PyErr_SetInterruptEx
            # so SIGTERM will be sent as SIGINT to the interpreter
            self.assertTrue(self.sigint_event.is_set())
            self.assertFalse(self.sigterm_event.is_set())
        else:
            self.assertTrue(self.sigterm_event.is_set())
            self.assertFalse(self.sigint_event.is_set())


# Child process for SignalInterruptRaceTest.  Each trial blocks the main
# thread in a Java call that waits (Thread.sleep or Object.wait) and sends
# SIGINT to the process from a timer thread after a short random delay.
# The signal handler thread of the JVM then wakes the main thread, and the
# main thread has to see KeyboardInterrupt and not InterruptedException.
_RACE_SCRIPT = """
import os, random, signal, sys, threading
import jpype

trials = int(sys.argv[1])
# A process started in the background of a shell can inherit SIGINT as ignored,
# in which case Python does not install its own handler.
signal.signal(signal.SIGINT, signal.default_int_handler)
jpype.startJVM(interrupt=False)
Thread = jpype.JClass("java.lang.Thread")
Object = jpype.JClass("java.lang.Object")
done = 0
result = "OK"
try:
    for i in range(trials):
        timer = threading.Timer(random.uniform(0.01, 0.05), os.kill,
                                (os.getpid(), signal.SIGINT))
        timer.start()
        try:
            if i % 2 == 0:
                Thread.sleep(30000)
                result = "trial %d: Thread.sleep returned" % i
                break
            lock = Object()
            with jpype.synchronized(lock):
                # Object.wait is allowed to return without a notify (a
                # spurious wakeup), so wait again until the interrupt arrives.
                while True:
                    lock.wait()
        except KeyboardInterrupt:
            timer.join()
            done += 1
        except BaseException as ex:
            result = "trial %d: %s: %s" % (i, type(ex).__name__, ex)
            break
except KeyboardInterrupt:
    # The interrupt was not delivered by the blocking call that was waiting
    # for it and showed up later.
    result = "trial %d: KeyboardInterrupt arrived late" % done
print("RESULT %d %s" % (done, result), flush=True)
os._exit(0 if result == "OK" else 1)
"""


class SignalInterruptRaceTest(unittest.TestCase):
    """A SIGINT that arrives while the main thread is blocked in a Java call
    must raise KeyboardInterrupt in Python.

    The signal handler that startJVM(interrupt=False) installs wakes the main
    thread with Thread.interrupt() and records the signal for Python.  If the
    main thread is woken first, the InterruptedException can be converted
    before the signal is recorded, and Python sees java.lang.InterruptedException
    instead of KeyboardInterrupt (or the process aborts).  That is a race
    between two threads, so the test repeats the interrupt many times in one
    child process (the first failure ends the test) and relies on the race
    being lost at least once if the order of the two steps is wrong.
    """

    trials = 200

    def setUp(self):
        if sys.platform == "win32":
            raise unittest.SkipTest("signals test not applicable on windows")

    def testBlockingCallRaisesKeyboardInterrupt(self):
        result = subprocess.run([sys.executable, "-c", _RACE_SCRIPT, str(self.trials)],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=120)
        out = result.stdout.decode("utf-8", "replace")
        err = result.stderr.decode("utf-8", "replace")
        self.assertEqual(result.returncode, 0, out + err)
        self.assertIn("RESULT %d OK" % self.trials, out)


# Child process for SignalTermDefaultHandlerTest.  Python's handler for SIGTERM
# is left at the default, which is the normal state of a script.
_TERM_SCRIPT = """
import resource, sys, time
import jpype

# a crash of this process must not leave a core file behind
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
jpype.startJVM(interrupt=False)
print("READY", flush=True)
time.sleep(60)
"""


def _sigterm_child(wait):
    """Start the child, send it SIGTERM when it is ready, and wait up to `wait`
    seconds for it to stop (it is killed after that).

    Returns the line the child printed when it was ready, whether it stopped
    on its own, its exit status, the crash reports that the JVM wrote in the
    working directory of the child, and what it wrote to stderr.
    """
    with tempfile.TemporaryDirectory() as cwd:
        errfile = os.path.join(cwd, "stderr.txt")
        stopped = False
        with open(errfile, "wb") as err:
            child = subprocess.Popen([sys.executable, "-c", _TERM_SCRIPT],
                                     cwd=cwd, stdout=subprocess.PIPE,
                                     stderr=err)
            # Never leave the child behind if something goes wrong.
            guard = threading.Timer(120, child.kill)
            guard.start()
            try:
                ready = child.stdout.readline().decode("utf-8", "replace").strip()
                if ready != "READY":
                    child.kill()
                else:
                    child.send_signal(signal.SIGTERM)
                    try:
                        child.wait(timeout=wait)
                        stopped = True
                    except subprocess.TimeoutExpired:
                        child.kill()
                child.wait()
            finally:
                guard.cancel()
                child.stdout.close()
        with open(errfile, "rb") as err:
            stderr = err.read().decode("utf-8", "replace")
        reports = glob.glob(os.path.join(cwd, "hs_err_pid*.log"))
    return ready, stopped, child.returncode, reports, stderr


class SignalTermDefaultHandlerTest(unittest.TestCase):
    """SIGTERM must not crash a process that started the JVM with
    interrupt=False and did not register a Python handler for SIGTERM.

    The handler that startJVM(interrupt=False) installs for SIGTERM runs on a
    thread of the JVM that Python has never seen.  It used to call
    PyErr_SetInterruptEx() without holding the GIL, and CPython then crashed
    in PyObject_RichCompareBool() while comparing the (default) Python handler
    with SIG_DFL.  SignalsTest registers Python handlers before startJVM,
    which takes a different path through CPython and so does not cover this.
    """

    def setUp(self):
        if sys.platform == "win32":
            raise unittest.SkipTest("signals test not applicable on windows")

    def testSigTermDoesNotCrash(self):
        ready, stopped, returncode, reports, stderr = _sigterm_child(wait=3)
        self.assertEqual(ready, "READY", stderr)
        self.assertEqual(reports, [], "the JVM wrote a crash report\n" + stderr)
        # Whether the process should stop on SIGTERM is not what this test is
        # about, only that it does not crash.
        if stopped:
            self.assertNotIn(returncode,
                             (-signal.SIGSEGV, -signal.SIGABRT, 128 + signal.SIGSEGV,
                              128 + signal.SIGABRT), stderr)


# Child process for SignalStaleInterruptTest.  Each trial runs a loop of short
# Java calls until a SIGINT from a timer thread raises KeyboardInterrupt, then
# makes a blocking Java call.
_STALE_SCRIPT = """
import os, random, signal, sys, threading, time
import jpype

trials = int(sys.argv[1])
signal.signal(signal.SIGINT, signal.default_int_handler)
jpype.startJVM(interrupt=False)
Thread = jpype.JClass("java.lang.Thread")
Math = jpype.JClass("java.lang.Math")
result = "OK"
for i in range(trials):
    timer = threading.Timer(random.uniform(0.005, 0.02), os.kill,
                            (os.getpid(), signal.SIGINT))
    timer.start()
    try:
        end = time.monotonic() + 5.0
        while time.monotonic() < end:
            Math.abs(1)
        result = "trial %d: the SIGINT was not delivered" % i
        break
    except KeyboardInterrupt:
        pass
    timer.join()
    try:
        Thread.sleep(20)
    except BaseException as ex:
        result = "trial %d: %s: %s" % (i, type(ex).__name__, ex)
        break
print("RESULT " + result, flush=True)
os._exit(0 if result == "OK" else 1)
"""


class SignalStaleInterruptTest(unittest.TestCase):
    """A SIGINT that Python has handled must not leave an interrupt behind.

    The handler of startJVM(interrupt=False) sets the interrupt flag of the
    main thread.  When the main thread is running Python or making Java calls
    it handles the KeyboardInterrupt first, and the next Java call of the main
    thread clears the flag (hasInterrupt() then clearInterrupt()) provided the
    flag was set before that call.  If the signal handler announces the signal
    before it sets the flag, a main thread that makes Java calls in a loop
    consumes the announcement too early, and the flag that is set a moment
    later makes the next blocking Java call raise InterruptedException.
    """

    trials = 300

    def setUp(self):
        if sys.platform == "win32":
            raise unittest.SkipTest("signals test not applicable on windows")

    def testBlockingCallAfterHandledSigint(self):
        result = subprocess.run([sys.executable, "-c", _STALE_SCRIPT, str(self.trials)],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=120)
        out = result.stdout.decode("utf-8", "replace")
        err = result.stderr.decode("utf-8", "replace")
        self.assertEqual(result.returncode, 0, out + err)
        self.assertIn("RESULT OK", out)
