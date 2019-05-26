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
import common


class JClassTestCase(common.JPypeTestCase):
    """ Test for methods of JClass

    Should test:
    - ``__getattribute__`` against methods, fields, python methods, and python properties
    - ``__setattr__`` against fields, final fields, python private fields
    - ``class_`` property
    - ``mro``
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testGetAttrPyMethod(self):
        cls = jpype.JClass('java.util.Iterator')
        obj = jpype.JClass('java.util.ArrayList')()
        obj.add(123)
        # Static python methods should be the same when accessed as classes or objects
        self.assertEqual(obj.iterator().next(), cls.next(obj.iterator()))

    def testGetAttrStaticMethod(self):
        cls = jpype.JClass('java.lang.Long')
        obj = cls(10)
        # Static java methods should be the same when accessed as classes or objects
        self.assertEqual(cls.bitCount(123), obj.bitCount(123))

    def testGetAttrStaticField(self):
        cls = jpype.JClass('java.lang.String')
        obj = cls("foo")
        # Static fields should be the same when accessed as classes or objects
        self.assertEqual(cls.CASE_INSENSITIVE_ORDER,
                         obj.CASE_INSENSITIVE_ORDER)

    def testSetAttrPythonField(self):
        cls = jpype.JClass('java.lang.String')
        # Setting a private field on a Java class is allowed
        cls._allowed = 1
        with self.assertRaises(AttributeError):
            # Setting a public field on a Java class is forbidden
            cls.forbidden = 1

    def testSetAttrFinal(self):
        cls = jpype.JClass('java.lang.Long')
        with self.assertRaises(AttributeError):
            # Setting a final field is forbidden
            cls.SIZE = 1

    def testClass(self):
        cls = jpype.JClass('java.lang.Long')
        clsType = jpype.JClass('java.lang.Class')
        # Get class must return a java.lang.Class instance belonging to the class
        self.assertIsInstance(cls.class_, clsType)
        self.assertEqual(cls.class_.getSimpleName(), "Long")

    def testGetAttrProperty(self):
        cls = jpype.JClass('java.lang.RuntimeException')
        with self.assertRaises(AttributeError):
            value = cls.args

    def testSetAttrProperty(self):
        cls = jpype.JClass('java.lang.RuntimeException')
        with self.assertRaises(AttributeError):
            cls.args = 1

    def testGetAttrStaticField(self):
        cls = jpype.JClass('jpype.types.FieldsTest')
        cls.staticObjectField = "fred"
        self.assertEqual(cls.staticObjectField, "fred")

    def testSetAttrStaticField(self):
        cls = jpype.JClass('jpype.types.FieldsTest')
        cls.staticObjectField = "fred"

    def testGetAttrField(self):
        cls = jpype.JClass('jpype.types.FieldsTest')
        with self.assertRaises(AttributeError):
            v = cls.objectField

    def testSetAttrField(self):
        cls = jpype.JClass('jpype.types.FieldsTest')
        with self.assertRaises(AttributeError):
            cls.objectField = "fred"

    def testGetAttrPrivateField(self):
        cls = jpype.JClass('jpype.types.FieldsTest')
        with self.assertRaises(AttributeError):
            v = cls.privateObjectField

    def testSetAttrPrivateField(self):
        cls = jpype.JClass('jpype.types.FieldsTest')
        with self.assertRaises(AttributeError):
            cls.privateObjectField = "fred"

    def testGetAttrFinalField(self):
        cls = jpype.JClass('jpype.types.FieldsTest')
        with self.assertRaises(AttributeError):
            v = cls.finalObjectField

    def testSetAttrFinalField(self):
        cls = jpype.JClass('jpype.types.FieldsTest')
        with self.assertRaises(AttributeError):
            cls.finalObjectField = "fred"

    def testGetAttrStaticFinalField(self):
        cls = jpype.JClass('jpype.types.FieldsTest')
        self.assertEqual(cls.finalStaticObjectField,
                         "final static object field")

    def testSetAttrStaticFinalField(self):
        cls = jpype.JClass('jpype.types.FieldsTest')
        with self.assertRaises(AttributeError):
            cls.finalStaticObjectField = "bar"

    def testStaticMethod(self):
        cls = jpype.JClass('jpype.types.MethodsTest')
        cls.callStaticObject(jpype.JObject())

    def testPrivateStaticMethod(self):
        cls = jpype.JClass('jpype.types.MethodsTest')
        with self.assertRaises(AttributeError):
            cls.callPrivateStaticObject(jpype.JObject())

    def testMethod(self):
        cls = jpype.JClass('jpype.types.MethodsTest')
        with self.assertRaises(RuntimeError):
            cls.callObject(jpype.JObject())
        cls.callObject(cls(), jpype.JObject())

    def testPrivateMethod(self):
        cls = jpype.JClass('jpype.types.MethodsTest')
        with self.assertRaises(AttributeError):
            cls.callPrivateObject(jpype.JObject())

    def testProtectedMethod(self):
        cls = jpype.JClass('jpype.types.MethodsTest')
        with self.assertRaises(AttributeError):
            cls.callProtectedObject(jpype.JObject())
