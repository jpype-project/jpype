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
try:
    import unittest2 as unittest
except ImportError:
    import unittest
import sys
import jpype
import common


class ReferenceQueueTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.classLoader = jpype.JClass(
            'org.jpype.classloader.JPypeClassLoader').getInstance()

    def testAccess(self):
        # Access the reference queue using the private class loader
        JPypeReferenceQueue = jpype.JClass(
            'org.jpype.ref.JPypeReferenceQueue', loader=self.classLoader)
        # Make sure we can get the instance
        self.assertTrue(JPypeReferenceQueue.getInstance() != None)

    def testRunning(self):
        # Access the reference queue using the private class loader
        JPypeReferenceQueue = jpype.JClass(
            'org.jpype.ref.JPypeReferenceQueue', loader=self.classLoader)

        # Get the queue instance
        queue = JPypeReferenceQueue.getInstance()
        self.assertTrue(queue.isRunning())


if __name__ == '__main__':
    jpype.startJVM(jpype.getDefaultJVMPath())
    unittest.main()
