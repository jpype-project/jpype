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
import jpype
import common
import pytest

from jpype.imports import *


class ThreadTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    @pytest.mark.filterwarnings("ignore::DeprecationWarning")
    def testAttach(self):
        # Make sure we are attached to start the test
        jpype.attachThreadToJVM()
        self.assertTrue(jpype.isThreadAttachedToJVM())

        # Detach from the thread
        jpype.detachThreadFromJVM()
        self.assertFalse(jpype.isThreadAttachedToJVM())

        # Reattach to the thread
        jpype.attachThreadToJVM()
        self.assertTrue(jpype.isThreadAttachedToJVM())

        # Detach again
        jpype.detachThreadFromJVM()
        self.assertFalse(jpype.isThreadAttachedToJVM())

        # Call a Java method which will cause it to attach automatically
        s = jpype.JString("foo")
        self.assertTrue(jpype.isThreadAttachedToJVM())

    @pytest.mark.filterwarnings("ignore::DeprecationWarning")
    def testAttachTwiceRegistersAutoDetachOnce(self):
        # Attaching an already-attached thread a second time without an
        # intervening detach must not re-register the auto-detach TLS
        # destructor a second time (native/common/jp_context.cpp's
        # registerAutoDetach() skips re-registering if a value is already
        # set for this thread) -- this is a no-op, not a leak or a
        # double-detach-at-exit hazard, but exercises a branch no other
        # test reaches.
        jpype.attachThreadToJVM()
        self.assertTrue(jpype.isThreadAttachedToJVM())
        jpype.attachThreadToJVM()
        self.assertTrue(jpype.isThreadAttachedToJVM())
        jpype.detachThreadFromJVM()
        self.assertFalse(jpype.isThreadAttachedToJVM())

    def testAttachNew(self):
        import java
        # Detach the thread
        java.lang.Thread.detach()
        self.assertFalse(java.lang.Thread.isAttached())

        # Attach as a main thread
        java.lang.Thread.attach()
        self.assertTrue(java.lang.Thread.isAttached())
        self.assertFalse(java.lang.Thread.currentThread().isDaemon())

        # Detach the thread
        java.lang.Thread.detach()
        self.assertFalse(java.lang.Thread.isAttached())

        # Attach as a daemon thread
        java.lang.Thread.attachAsDaemon()
        self.assertTrue(java.lang.Thread.isAttached())
        self.assertTrue(java.lang.Thread.currentThread().isDaemon())

    def testAutoDetachAfterManualDetach(self):
        # Regression test for the auto-detach-on-thread-exit fix
        # (native/common/jp_context.cpp): a thread that calls into Java
        # with no prior attachThreadToJVM() gets implicitly attached
        # (JPContext::getEnv()), and that implicit attach registers a
        # per-thread TLS destructor that auto-detaches when the thread
        # exits, in case the thread never detaches itself. If a thread
        # DOES explicitly call detachThreadFromJVM() before exiting, the
        # TLS bookkeeping must be cleared too -- otherwise the
        # auto-detach destructor fires again on thread exit and tries to
        # detach an already-detached thread a second time. This proves
        # that sequence (auto-attach, then a real user detach, then
        # thread exit) does not crash, hang, or otherwise corrupt the
        # JVM's thread accounting for threads started afterward.
        import threading
        import java
        N = 20
        results = [None] * N

        def worker(i):
            # Implicit ("auto") attach: first call into Java on this
            # brand-new native thread, no explicit attach first.
            jpype.JString("worker")
            attached_after_auto_attach = java.lang.Thread.isAttached()
            # Explicit user detach.
            java.lang.Thread.detach()
            attached_after_manual_detach = java.lang.Thread.isAttached()
            results[i] = (attached_after_auto_attach, attached_after_manual_detach)
            # Thread exits here -- the TLS auto-detach destructor runs
            # next, and must be a no-op since the manual detach above
            # already cleared its bookkeeping.

        threads = [threading.Thread(target=worker, args=(i,)) for i in range(N)]
        for t in threads:
            t.start()
        for t in threads:
            t.join(timeout=10)
            self.assertFalse(t.is_alive())

        for r in results:
            self.assertEqual(r, (True, False))

        # The JVM must still be fully functional after all N threads
        # auto-attached, explicitly detached, and exited (each exit
        # running the now-inert auto-detach destructor).
        self.assertEqual(str(jpype.JString("still alive")), "still alive")
