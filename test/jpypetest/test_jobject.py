# *****************************************************************************
#   Copyright 2017 Karl Einar Nelson
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
import jpype
from jpype.types import *
import common

class JClassTestCase(common.JPypeTestCase):
    """ Test for methods of JObject

    Should test:
    - ``__getattribute__`` against methods, fields, python methods, and python properties
    - ``__setattr__`` against fields, final fields, python private fields
    - ``class_`` property
    - ``mro``
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.fixture = JClass('jpype.common.Fixture')()

    def testSetAttrPythonField(self):
        cls = JClass('java.lang.String')
        obj = cls('foo')
        # Setting a private field on a Java class is allowed
        obj._allowed = 1
        with self.assertRaises(AttributeError):
            # Setting a public field on a Java class is forbidden
            obj.forbidden = 1

    def testSetAttrFinal(self):
        cls = JClass('java.lang.Long')
        obj = cls(1)
        with self.assertRaises(AttributeError):
            # Setting a final field is forbidden
            obj.SIZE = 1

    def testClass(self):
        obj = JClass('java.lang.Long')
        clsType = JClass('java.lang.Class')
        # Get class must return a java.lang.Class instance belonging to the class
        self.assertIsInstance(obj.class_, clsType)
        self.assertEqual(obj.class_.getSimpleName(), "Long")

    def testGetAttrProperty(self):
        obj = JClass('java.lang.RuntimeException')('oo')
        value = obj.args
        self.assertEqual(value, ('oo',))

    def testSetAttrProperty(self):
        obj = JClass('java.lang.RuntimeException')('oo')
        with self.assertRaises(AttributeError):
            obj.args = 1

    def testAttrStaticField(self):
        self.fixture.static_object_field = "fred"
        self.assertEqual(self.fixture.static_object_field, "fred")

    def testGetAttrField(self):
        v = self.fixture.object_field

    def testSetAttrField(self):
        self.fixture.object_field = "fred"

    def testGetAttrPrivateField(self):
        with self.assertRaises(AttributeError):
            v = self.fixture.private_object_field

    def testSetAttrPrivateField(self):
        with self.assertRaises(AttributeError):
            self.fixture.private_object_field = "fred"

    def testGetAttrFinalField(self):
        v = self.fixture.final_object_field

    def testSetAttrFinalField(self):
        with self.assertRaises(AttributeError):
            self.fixture.final_object_field = "fred"

    def testGetAttrStaticFinalField(self):
        self.assertEqual(self.fixture.final_static_object_field,
                         "final static object field")

    def testSetAttrStaticFinalField(self):
        with self.assertRaises(AttributeError):
            self.fixture.finalStaticObjectField = "bar"

    def testStaticMethod(self):
        self.fixture.callStaticObject(JObject())

    def testPrivateStaticMethod(self):
        with self.assertRaises(AttributeError):
            self.fixture.callPrivateStaticObject(JObject())

    def testMethod(self):
        self.fixture.callObject(JObject())

    def testPrivateMethod(self):
        with self.assertRaises(AttributeError):
            self.fixture.callPrivateObject(JObject())

    def testProtectedMethod(self):
        with self.assertRaises(AttributeError):
            self.fixture.callProtectedObject(JObject())
