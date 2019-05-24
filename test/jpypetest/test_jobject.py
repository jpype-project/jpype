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
from . import common


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

    def testSetAttrPythonField(self):
        cls = jpype.JClass('java.lang.String')
        obj = cls('foo')
        # Setting a private field on a Java class is allowed
        obj._allowed = 1
        with self.assertRaises(AttributeError):
            # Setting a public field on a Java class is forbidden
            obj.forbidden = 1

    def testSetAttrFinal(self):
        cls = jpype.JClass('java.lang.Long')
        obj = cls(1)
        with self.assertRaises(AttributeError):
            # Setting a final field is forbidden
            obj.SIZE = 1

    def testClass(self):
        obj = jpype.JClass('java.lang.Long')
        clsType = jpype.JClass('java.lang.Class')
        # Get class must return a java.lang.Class instance belonging to the class
        self.assertIsInstance(obj.class_, clsType)
        self.assertEqual(obj.class_.getSimpleName(), "Long")

    def testGetAttrProperty(self):
        obj = jpype.JClass('java.lang.RuntimeException')('oo')
        value = obj.args
        self.assertEqual(value, ('oo',))

    def testSetAttrProperty(self):
        obj = jpype.JClass('java.lang.RuntimeException')('oo')
        with self.assertRaises(AttributeError):
            obj.args = 1

    def testGetAttrStaticField(self):
        obj = jpype.JClass('jpype.types.FieldsTest')()
        obj.staticObjectField = "fred"
        self.assertEqual(obj.staticObjectField, "fred")

    def testSetAttrStaticField(self):
        obj = jpype.JClass('jpype.types.FieldsTest')()
        obj.staticObjectField = "fred"

    def testGetAttrField(self):
        obj = jpype.JClass('jpype.types.FieldsTest')()
        v = obj.objectField

    def testSetAttrField(self):
        obj = jpype.JClass('jpype.types.FieldsTest')()
        obj.objectField = "fred"

    def testGetAttrPrivateField(self):
        obj = jpype.JClass('jpype.types.FieldsTest')()
        with self.assertRaises(AttributeError):
            v = obj.privateObjectField

    def testSetAttrPrivateField(self):
        obj = jpype.JClass('jpype.types.FieldsTest')()
        with self.assertRaises(AttributeError):
            obj.privateObjectField = "fred"

    def testGetAttrFinalField(self):
        obj = jpype.JClass('jpype.types.FieldsTest')()
        v = obj.finalObjectField

    def testSetAttrFinalField(self):
        obj = jpype.JClass('jpype.types.FieldsTest')()
        with self.assertRaises(AttributeError):
            obj.finalObjectField = "fred"

    def testGetAttrStaticFinalField(self):
        obj = jpype.JClass('jpype.types.FieldsTest')()
        self.assertEqual(obj.finalStaticObjectField,
                         "final static object field")

    def testSetAttrStaticFinalField(self):
        obj = jpype.JClass('jpype.types.FieldsTest')()
        with self.assertRaises(AttributeError):
            obj.finalStaticObjectField = "bar"

    def testStaticMethod(self):
        obj = jpype.JClass('jpype.types.MethodsTest')()
        obj.callStaticObject(jpype.JObject())

    def testPrivateStaticMethod(self):
        obj = jpype.JClass('jpype.types.MethodsTest')()
        with self.assertRaises(AttributeError):
            obj.callPrivateStaticObject(jpype.JObject())

    def testMethod(self):
        obj = jpype.JClass('jpype.types.MethodsTest')()
        obj.callObject(jpype.JObject())

    def testPrivateMethod(self):
        obj = jpype.JClass('jpype.types.MethodsTest')()
        with self.assertRaises(AttributeError):
            obj.callPrivateObject(jpype.JObject())

    def testProtectedMethod(self):
        obj = jpype.JClass('jpype.types.MethodsTest')()
        with self.assertRaises(AttributeError):
            obj.callProtectedObject(jpype.JObject())
