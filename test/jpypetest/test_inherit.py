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
from jpype.types import *
from jpype import java, JInterface
import common


class InheritTestCase(common.JPypeTestCase):
    """ test for isinstance and issubclass """

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def assertIsSubclass(self, a, b):
        if not issubclass(a, b):
            raise AssertionError("'%s' is not a subclass of '%s'" % (a, b))

    def assertNotIsSubclass(self, a, b):
        if issubclass(a, b):
            raise AssertionError("'%s' is a subclass of '%s'" % (a, b))

    def testPrimitivesAsObjects(self):
        self.assertNotIsInstance(JBoolean(1), JObject)
        self.assertNotIsInstance(JByte(1), JObject)
        self.assertNotIsInstance(JChar(1), JObject)
        self.assertNotIsInstance(JShort(1), JObject)
        self.assertNotIsInstance(JInt(1), JObject)
        self.assertNotIsInstance(JLong(1), JObject)
        self.assertNotIsInstance(JFloat(1), JObject)
        self.assertNotIsInstance(JDouble(1), JObject)
        self.assertNotIsSubclass(JBoolean, JObject)
        self.assertNotIsSubclass(JByte, JObject)
        self.assertNotIsSubclass(JChar, JObject)
        self.assertNotIsSubclass(JShort, JObject)
        self.assertNotIsSubclass(JInt, JObject)
        self.assertNotIsSubclass(JLong, JObject)
        self.assertNotIsSubclass(JFloat, JObject)
        self.assertNotIsSubclass(JDouble, JObject)

    def testPrimitivesAsInterfaces(self):
        self.assertNotIsInstance(JBoolean, JInterface)
        self.assertNotIsInstance(JByte, JInterface)
        self.assertNotIsInstance(JChar, JInterface)
        self.assertNotIsInstance(JShort, JInterface)
        self.assertNotIsInstance(JInt, JInterface)
        self.assertNotIsInstance(JLong, JInterface)
        self.assertNotIsInstance(JFloat, JInterface)
        self.assertNotIsInstance(JDouble, JInterface)

    def testPrimitivesAsClasses(self):
        self.assertIsInstance(JBoolean, JClass)
        self.assertIsInstance(JByte, JClass)
        self.assertIsInstance(JChar, JClass)
        self.assertIsInstance(JShort, JClass)
        self.assertIsInstance(JInt, JClass)
        self.assertIsInstance(JLong, JClass)
        self.assertIsInstance(JFloat, JClass)
        self.assertIsInstance(JDouble, JClass)

    def testString(self):
        self.assertIsSubclass(JString, JObject)
        self.assertIsSubclass(java.lang.String, JObject)
        self.assertIsSubclass(JString, JString)
        self.assertIsSubclass(java.lang.String, JString)
        self.assertIsInstance(JString("f"), JObject)
        self.assertIsInstance(java.lang.String("f"), JObject)
        self.assertIsInstance(JString("f"), JString)
        self.assertIsInstance(java.lang.String("f"), JString)
        self.assertNotIsInstance(JString, JInterface)
        self.assertNotIsInstance(java.lang.String, JInterface)
        self.assertIsSubclass(JString, java.lang.String)

    def testObject(self):
        self.assertIsInstance(JObject, JClass)
        self.assertIsInstance(java.lang.Object, JClass)
        self.assertNotIsSubclass(JObject, JInterface)
        self.assertNotIsSubclass(java.lang.Object, JInterface)
        self.assertIsSubclass(JObject, JObject)
        self.assertIsSubclass(JObject, java.lang.Object)
        self.assertNotIsSubclass(JObject, JException)
        self.assertNotIsInstance(JObject(), JInterface)
        self.assertNotIsInstance(java.lang.Object(), JInterface)
        self.assertIsInstance(JObject(), JObject)
        self.assertIsInstance(JObject(), java.lang.Object)

    def testException(self):
        # JException is a bit of screw ball as it must be
        # a subclass of Exception so we can catch it, but at the same
        # time it is also a meta class in the original API.
        self.assertIsInstance(JException, JClass)
        self.assertIsInstance(java.lang.Throwable, JClass)
        self.assertNotIsSubclass(JException, JInterface)
        self.assertNotIsSubclass(java.lang.Throwable, JInterface)
        self.assertIsSubclass(JException, Exception)
        self.assertIsSubclass(JException, JException)
        self.assertNotIsInstance(java.lang.Throwable("f"), JInterface)
        th = java.lang.Throwable("foo")
        self.assertIsInstance(th, JObject)
        self.assertIsInstance(th, java.lang.Object)
        self.assertIsInstance(th, java.lang.Throwable)
        self.assertIsInstance(th, JException)
        self.assertIsInstance(th, Exception)
        with self.assertRaises(Exception):
            raise th
        with self.assertRaises(JException):
            raise th

    def testInterface(self):
        # JInterface is a meta but was a base class at some point
        self.assertIsInstance(java.io.Serializable, JInterface)
        self.assertIsSubclass(java.io.Serializable, JInterface)
