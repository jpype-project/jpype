# *****************************************************************************
#   Copyright 2018 Karl Einar Nelson
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
# *****************************************************************************
import sys
import jpype
import common


class ReferenceQueueTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.classLoader = jpype.JClass(
            'org.jpype.JPypeContext').getInstance().getBootLoader()
        self.refqueue = jpype.JClass(
            'org.jpype.JPypeContext').getInstance().getReferenceQueue()

    def testAccess(self):
        # Make sure we can get the instance
        self.assertTrue(self.refqueue != None)

    def testRunning(self):
        # Get the queue instance
        self.assertTrue(self.refqueue.isRunning())
