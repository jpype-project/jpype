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
import sys
import threading
import time
import jpype
from jpype import synchronized
import common

fail = False
if fail:
    # Cause a fail
    class _JMonitor(object):
        def __init__(self, obj):
            pass

        def __enter__(self):
            pass

        def __exit__(self, exception_type, exception_value, traceback):
            pass

    def synchronized(obj):
        return _JMonitor(obj)

success = True
glob = 0
obj = None


class MyThread(threading.Thread):
    def run(self):
        global glob, success, obj
        with synchronized(obj):
            # Increment the global resource
            glob += 1

            # Wait for a while
            time.sleep(0.1)

            # If anything else accessed then it was a fail
            if glob != 1:
                success = False

            # Decrement the global resoure
            glob -= 1


class SynchronizedTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        global glob, success, obj
        obj = jpype.JClass("java.lang.Object")()

    def testSynchronized(self):
        global glob, success
        glob = 0
        success = True

        tl = []
        # Create 10 threads trying to access the same resource
        for i in range(0, 10):
            tl.append(MyThread())

        # Release them at the same time
        for i in tl:
            i.start()

        # Wait for all to finish
        for i in tl:
            i.join()

        # Verify they did not trample each other
        self.assertTrue(success)

    def testSyncronizedFail(self):
        with self.assertRaisesRegex(TypeError, "Java object is required"):
            with jpype.synchronized(object()):
                pass
        with self.assertRaisesRegex(TypeError, "Java primitives cannot be used"):
            with jpype.synchronized(jpype.JInt(1)):
                pass
