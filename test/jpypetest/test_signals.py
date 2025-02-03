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
