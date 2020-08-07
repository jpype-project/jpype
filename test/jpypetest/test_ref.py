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
import _jpype
import jpype
from jpype import JImplements, JOverride
from jpype.types import *
import common


class ReferenceQueueTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.refqueue = jpype.JClass(
            'org.jpype.ref.JPypeReferenceQueue').getInstance()

    def testAccess(self):
        # Make sure we can get the instance
        self.assertTrue(self.refqueue != None)

    def testRunning(self):
        # Get the queue instance
        self.assertTrue(self.refqueue.isRunning())

    def testRefs(self):
        # This routine will exercise each of the clean up paths once
        fixture = JClass("jpype.common.Fixture")()

        def f():
            # Create a proxy to test the proxy path
            @JImplements("java.util.function.Supplier")
            class MySupplier(object):
                @JOverride
                def get(self):
                    # Send a Python exc to trigger Python ref path
                    raise RuntimeError("foo")
            try:
                u = MySupplier()
                fixture.callSupplier(u)
            except RuntimeError as ex:
                pass
        f()

        # Force a direct byffer and then trash it
        b = bytearray([1, 2, 3])
        _jpype.convertToDirectBuffer(b)

        # Then force a GC to clean it up
        jpype.java.lang.System.gc()

        # We can't check the results here as the GC may chose not
        # to run which would trigger a failure
