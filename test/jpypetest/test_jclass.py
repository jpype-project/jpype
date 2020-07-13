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
import _jpype
import jpype
from jpype.types import *
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
        cls = JClass('java.util.Iterator')
        obj = JClass('java.util.ArrayList')()
        obj.add(123)
        # Static python methods should be the same when accessed as classes or objects
        self.assertEqual(obj.iterator().next(), cls.next(obj.iterator()))

    def testGetAttrStaticMethod(self):
        cls = JClass('java.lang.Long')
        obj = cls(10)
        # Static java methods should be the same when accessed as classes or objects
        self.assertEqual(cls.bitCount(123), obj.bitCount(123))

    def testGetAttrStaticField(self):
        cls = JClass('java.lang.String')
        obj = cls("foo")
        # Static fields should be the same when accessed as classes or objects
        self.assertEqual(cls.CASE_INSENSITIVE_ORDER,
                         obj.CASE_INSENSITIVE_ORDER)

    def testSetAttrPythonField(self):
        cls = JClass('java.lang.String')
        # Setting a private field on a Java class is allowed
        cls._allowed = 1
        with self.assertRaises(AttributeError):
            # Setting a public field on a Java class is forbidden
            cls.forbidden = 1

    def testSetAttrFinal(self):
        cls = JClass('java.lang.Long')
        with self.assertRaises(AttributeError):
            # Setting a final field is forbidden
            cls.SIZE = 1

    def testClass(self):
        cls = JClass('java.lang.Long')
        clsType = JClass('java.lang.Class')
        # Get class must return a java.lang.Class instance belonging to the class
        self.assertIsInstance(cls.class_, clsType)
        self.assertEqual(cls.class_.getSimpleName(), "Long")

    def testGetAttrProperty(self):
        cls = JClass('java.lang.RuntimeException')
        with self.assertRaises(AttributeError):
            value = cls.args

    def testSetAttrProperty(self):
        cls = JClass('java.lang.RuntimeException')
        with self.assertRaises(AttributeError):
            cls.args = 1

    def testGetAttrStaticField(self):
        cls = JClass('jpype.common.Fixture')
        cls.static_object_field = "fred"
        self.assertEqual(cls.static_object_field, "fred")

    def testSetAttrStaticField(self):
        cls = JClass('jpype.common.Fixture')
        cls.static_object_field = "fred"

    def testGetAttrField(self):
        cls = JClass('jpype.common.Fixture')
        with self.assertRaises(AttributeError):
            v = cls.object_field

    def testSetAttrField(self):
        cls = JClass('jpype.common.Fixture')
        with self.assertRaises(AttributeError):
            cls.object_field = "fred"

    def testGetAttrPrivateField(self):
        cls = JClass('jpype.common.Fixture')
        with self.assertRaises(AttributeError):
            v = cls.privateObjectField

    def testSetAttrPrivateField(self):
        cls = JClass('jpype.common.Fixture')
        with self.assertRaises(AttributeError):
            cls.private_object_field = "fred"

    def testGetAttrFinalField(self):
        cls = JClass('jpype.common.Fixture')
        with self.assertRaises(AttributeError):
            v = cls.final_object_field

    def testSetAttrFinalField(self):
        cls = JClass('jpype.common.Fixture')
        with self.assertRaises(AttributeError):
            cls.final_object_field = "fred"

    def testGetAttrStaticFinalField(self):
        cls = JClass('jpype.common.Fixture')
        self.assertEqual(cls.final_static_object_field,
                         "final static object field")

    def testSetAttrStaticFinalField(self):
        cls = JClass('jpype.common.Fixture')
        with self.assertRaises(AttributeError):
            cls.final_static_object_field = "bar"

    def testStaticMethod(self):
        cls = JClass('jpype.common.Fixture')
        cls.callStaticObject(JObject())

    def testPrivateStaticMethod(self):
        cls = JClass('jpype.common.Fixture')
        with self.assertRaises(AttributeError):
            cls.callPrivateStaticObject(JObject())

    def testMethod(self):
        cls = JClass('jpype.common.Fixture')
        with self.assertRaises(TypeError):
            cls.callObject(JObject())
        cls.callObject(cls(), JObject())

    def testPrivateMethod(self):
        cls = JClass('jpype.common.Fixture')
        with self.assertRaises(AttributeError):
            cls.callPrivateObject(JObject())

    def testProtectedMethod(self):
        cls = JClass('jpype.common.Fixture')
        with self.assertRaises(AttributeError):
            cls.callProtectedObject(JObject())

    def testJClassFail(self):
        with self.assertRaises(TypeError):
            cls = JClass("asdw.gqyr.jhnw")

    def testGetClassFromClass(self):
        cls = JClass('java.lang.Class')
        self.assertIsInstance(cls.class_, cls)

    def testGetClassFromInterface(self):
        intr = JClass('java.io.Serializable')
        cls = JClass('java.lang.Class')
        self.assertIsInstance(intr.class_, cls)

    def testInterfaceCtor(self):
        intr = JClass('java.io.Serializable')
        with self.assertRaises(TypeError):
            intr()

    def testJClassWithLoader(self):
        cl = JClass('java.lang.Class').class_.getClassLoader()
        self.assertIsInstance(
            JClass('java.lang.StringBuilder', loader=cl), JClass)

    @common.requireInstrumentation
    def testJavaConversionFault(self):
        _jpype.fault("JPClass::findJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(jpype.java.lang.Class._canConvertToJava(None))

    def testJavaConversion(self):
        a = JString("a")
        self.assertEqual(
            jpype.java.lang.Class._canConvertToJava(object()), "none")
        self.assertEqual(
            jpype.java.lang.Class._canConvertToJava(a.getClass()), "exact")
        self.assertEqual(
            jpype.java.lang.Class._canConvertToJava(JString), "exact")

    def testInnerClass(self):
        # This tests for problems when the inner class implements the
        # outer interface which creates a race condition.  Success is
        # not throwing an exception
        test = JClass("jpype.types.InnerTest")()
        test.test()

    def testLookupGeneric(self):
        self.assertEqual(JClass('java.util.ArrayList<>'),
                         JClass("java.util.ArrayList"))

    def testLookupJNI(self):
        self.assertEqual(JClass('java/lang/Object'),
                         JClass("java.lang.Object"))

    def testLookupArray(self):
        self.assertEqual(JClass('int[]'), JArray(JInt))
        self.assertEqual(JClass('int[][]'), JArray(JInt, 2))
        self.assertEqual(JClass('int[][][]'), JArray(JInt, 3))
        self.assertEqual(JClass('java.lang.Object[][][]'), JArray(JObject, 3))

    def testLookupArrayJNI(self):
        self.assertEqual(JClass('[I'), JArray(JInt))
        self.assertEqual(JClass('[[J'), JArray(JLong, 2))
        self.assertEqual(JClass('[Ljava.lang.Object;'), JArray(JObject))
        self.assertEqual(JClass('[Ljava/lang/Object;'), JArray(JObject))

    def testClosed(self):
        with self.assertRaises(TypeError):
            class Q(JBoolean):
                pass
        with self.assertRaises(TypeError):
            class Q(JChar):
                pass
        with self.assertRaises(TypeError):
            class Q(JByte):
                pass
        with self.assertRaises(TypeError):
            class Q(JShort):
                pass
        with self.assertRaises(TypeError):
            class Q(JInt):
                pass
        with self.assertRaises(TypeError):
            class Q(JLong):
                pass
        with self.assertRaises(TypeError):
            class Q(JFloat):
                pass
        with self.assertRaises(TypeError):
            class Q(JDouble):
                pass

    def testAsArray(self):
        jo = JClass('java.lang.Object')
        self.assertEqual(jo[:], JArray(jo))
        self.assertEqual(jo[:][:], JArray(jo, 2))
        self.assertIsInstance(jo[5], JArray(jo))
        self.assertEqual(len(jo[5]), 5)
        self.assertIsInstance(jo[5, :], JArray(jo, 2))
        self.assertEqual(len(jo[5, :]), 5)
        with self.assertRaises(TypeError):
            self.assertEqual(jo[:, 5], JArray(jo, 2))
        with self.assertRaises(TypeError):
            jo['foo']

    def testCastEqual(self):
        jo = JClass('java.lang.Object')
        with self.assertRaises(TypeError):
            jo @= 5

    def testCanCast(self):
        String = JClass("java.lang.String")
        self.assertFalse(String._canCast(1))
        self.assertTrue(String._canCast("foo"))
