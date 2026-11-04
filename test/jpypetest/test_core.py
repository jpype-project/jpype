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
from unittest import mock
import subprocess
import sys
import unittest
import jpype
import common
from jpype.types import *


class JCharTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    @common.threadUnsafe
    def testShutdownHook(self):
        Thread = JClass("java.lang.Thread")
        Runnable = JClass("java.lang.Runnable")
        Runtime = JClass("java.lang.Runtime")
        @jpype.JImplements(Runnable)
        class Run:
            @jpype.JOverride
            def run(self):
                pass
        th = Thread(Run())
        Runtime.getRuntime().addShutdownHook(th)
        self.assertTrue(Runtime.getRuntime().removeShutdownHook(th))

    def testShutdownWrongThread(self):
        Thread = JClass("java.lang.Thread")
        Runnable = JClass("java.lang.Runnable")
        @jpype.JImplements(Runnable)
        class Run:
            def __init__(self):
                self.rc = False

            @jpype.JOverride
            def run(self):
                try:
                    jpype.shutdownJVM()
                except:
                    self.rc = True
        run = Run()
        th = Thread(run)
        th.start()
        th.join()
        self.assertTrue(run.rc)


class JVMStartupConcurrencyTestCase(unittest.TestCase):

    @common.threadUnsafe
    def testConcurrentStartOnlyOneWins(self):
        # A fresh interpreter is required: the test JVM is already running.
        # Without a lifecycle lock, free-threaded callers can all pass the
        # isStarted() check and race inside JPContext::startJVM.
        script = (
            "import threading, traceback\n"
            "import jpype\n"
            "count = 8\n"
            "barrier = threading.Barrier(count)\n"
            "outcomes = []\n"
            "outcomes_lock = threading.Lock()\n"
            "def worker():\n"
            "    barrier.wait()\n"
            "    try:\n"
            "        jpype.startJVM(convertStrings=False)\n"
            "    except OSError:\n"
            "        result = 'already-started'\n"
            "    except BaseException:\n"
            "        result = traceback.format_exc()\n"
            "    else:\n"
            "        result = 'started'\n"
            "    finally:\n"
            "        # The winner created the JVM on this Python thread; detach it\n"
            "        # so interpreter shutdown does not wait on it forever.\n"
            "        try:\n"
            "            jpype.java.lang.Thread.detach()\n"
            "        except BaseException:\n"
            "            pass\n"
            "    with outcomes_lock:\n"
            "        outcomes.append(result)\n"
            "threads = [threading.Thread(target=worker) for _ in range(count)]\n"
            "for thread in threads:\n"
            "    thread.start()\n"
            "for thread in threads:\n"
            "    thread.join()\n"
            "assert outcomes.count('started') == 1, outcomes\n"
            "assert outcomes.count('already-started') == count - 1, outcomes\n"
            "assert jpype.isJVMStarted()\n"
            "print('ONE-START')\n"
        )
        result = subprocess.run([sys.executable, "-c", script],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=180)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(b"ONE-START", result.stdout)
