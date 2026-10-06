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
import os
import signal
import subprocess
import sys
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
