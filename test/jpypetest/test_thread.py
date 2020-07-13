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
import sys
import time
import common
import pytest


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
