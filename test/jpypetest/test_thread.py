# *****************************************************************************
#   Copyright 2019 Karl Nelson
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
# *****************************************************************************
import jpype
import sys
import time
import common


class ThreadTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

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
