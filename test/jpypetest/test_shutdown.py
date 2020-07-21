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
import unittest
import jpype
import subrun


@subrun.TestCase
class ShutdownTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        jpype.startJVM(convertStrings=False)

        # Create some resources
        cls.jstr = jpype.java.lang.String("good morning")
        cls.jobj = jpype.java.lang.Object()
        cls.jcls = jpype.JClass("java.lang.String")
        cls.jarray = jpype.JArray(jpype.JInt)([1, 2, 3, 4])

        # Then blow everything up
        jpype.shutdownJVM()

    def testArrayGet(self):
        with self.assertRaises(jpype.JVMNotRunning):
            type(self).jarray[0]

    def testArraySet(self):
        with self.assertRaises(jpype.JVMNotRunning):
            type(self).jarray[0] = 1

    def testArrayGetSlice(self):
        with self.assertRaises(jpype.JVMNotRunning):
            type(self).jarray[0:2]

    def testArraySetSlice(self):
        with self.assertRaises(jpype.JVMNotRunning):
            type(self).jarray[0:2] = [1, 2]

    def testArrayStr(self):
        with self.assertRaises(jpype.JVMNotRunning):
            str(type(self).jarray)

    def testClassCtor(self):
        with self.assertRaises(jpype.JVMNotRunning):
            obj = type(self).jcls()

    def testObjectInvoke(self):
        with self.assertRaises(jpype.JVMNotRunning):
            type(self).jobj.wait()

    def testObjectStr(self):
        with self.assertRaises(jpype.JVMNotRunning):
            str(type(self).jobj)

    def testStringInvoke(self):
        with self.assertRaises(jpype.JVMNotRunning):
            type(self).jstr.substring(1)

    def testStringStr(self):
        with self.assertRaises(jpype.JVMNotRunning):
            str(type(self).jstr)
