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
import _jpype
from jpype.types import *
from jpype import java
import common
try:
    import numpy as np
except ImportError:
    pass


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

    def testObjectBoolTrue(self):
        self.fixture.object_field = True
        self.assertIsInstance(self.fixture.object_field,
                              JClass('java.lang.Boolean'))
        self.assertEqual(str(self.fixture.object_field), str(True))
        self.assertEqual(self.fixture.object_field, True)

    def testObjectBoolFalse(self):
        self.fixture.object_field = False
        self.assertIsInstance(self.fixture.object_field,
                              JClass('java.lang.Boolean'))
        self.assertEqual(str(self.fixture.object_field), str(False))
        self.assertEqual(self.fixture.object_field, False)

    def testObjectBoolJValue(self):
        self.fixture.object_field = JBoolean(True)
        self.assertIsInstance(self.fixture.object_field,
                              JClass('java.lang.Boolean'))
        self.assertEqual(self.fixture.object_field, True)

    def testObjectShort(self):
        self.fixture.object_field = JShort(1)
        self.assertEqual(self.fixture.object_field, 1)
        self.assertIsInstance(self.fixture.object_field,
                              JClass('java.lang.Short'))

    def testObjectInteger(self):
        self.fixture.object_field = JInt(2)
        self.assertEqual(self.fixture.object_field, 2)
        self.assertIsInstance(self.fixture.object_field,
                              JClass('java.lang.Integer'))

    def testObjectLong(self):
        self.fixture.object_field = JLong(3)
        self.assertEqual(self.fixture.object_field, 3)
        self.assertIsInstance(self.fixture.object_field,
                              JClass('java.lang.Long'))

    def testObjectFloat(self):
        self.fixture.object_field = JFloat(1.125)
        self.assertEqual(self.fixture.object_field, 1.125)
        self.assertIsInstance(self.fixture.object_field,
                              JClass('java.lang.Float'))

    def testObjectDouble(self):
        self.fixture.object_field = JDouble(2.6125)
        self.assertEqual(self.fixture.object_field, 2.6125)
        self.assertIsInstance(self.fixture.object_field,
                              JClass('java.lang.Double'))

    def testObjectField(self):
        with self.assertRaises(TypeError):
            self.fixture.object_field = object()
        with self.assertRaises(TypeError):
            self.fixture.static_object_field = object()

    def testArraySetRangeFail(self):
        ja = JArray(JObject)(4)
        with self.assertRaises(TypeError):
            ja[:] = [1, 2, object(), 3]
        ja[:] = [1, 2, 3, 4]

    @common.requireInstrumentation
    def testArraySetRangeFault(self):
        _jpype.fault("JPClass::setArrayRange")
        ja = JArray(JObject)(4)
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[:] = [1, 2, 3, 4]

    def testAssignClass(self):
        self.fixture.object_field = JClass("java.lang.StringBuilder")
        self.assertIsInstance(self.fixture.object_field, jpype.java.lang.Class)
        self.assertEqual(self.fixture.object_field,
                         JClass("java.lang.StringBuilder"))

    @common.requireInstrumentation
    def testSetFinalField(self):
        _jpype.fault("JPField::setStaticAttribute")
        with self.assertRaisesRegex(SystemError, "fault"):
            self.fixture.static_object_field = None

    def testHashNone(self):
        self.assertEqual(hash(JObject(None)), hash(None))

    def testStrPrimitive(self):
        with self.assertRaisesRegex(TypeError, "requires a Java object"):
            _jpype._JObject.__str__(JInt(1))

    def testGetAttrFail(self):
        jo = JClass("java.lang.Object")()
        with self.assertRaisesRegex(TypeError, "must be string"):
            getattr(jo, object())

    def testSetAttrFail(self):
        jo = JClass("java.lang.Object")()
        with self.assertRaisesRegex(TypeError, "must be string"):
            setattr(jo, object(), 1)

    def testSetAttrFail2(self):
        fixture = JClass("jpype.common.Fixture")()
        with self.assertRaisesRegex(AttributeError, "is not settable"):
            setattr(fixture, "callObject", 4)

    def testJavaPrimitives(self):
        self.assertIsInstance(
            self.fixture.callObject(JByte(1)), java.lang.Byte)
        self.assertIsInstance(
            self.fixture.callObject(JShort(1)), java.lang.Short)
        self.assertIsInstance(
            self.fixture.callObject(JInt(1)), java.lang.Integer)
        self.assertIsInstance(
            self.fixture.callObject(JLong(1)), java.lang.Long)
        self.assertIsInstance(
            self.fixture.callObject(JFloat(1)), java.lang.Float)
        self.assertIsInstance(self.fixture.callObject(
            JDouble(1)), java.lang.Double)

    def testPythonPrimitives(self):
        self.assertIsInstance(self.fixture.callObject(1), java.lang.Long)
        self.assertIsInstance(self.fixture.callObject(1.0), java.lang.Double)

    @common.requireNumpy
    def testNumpyPrimitives(self):
        self.assertIsInstance(
            self.fixture.callObject(np.int8(1)), java.lang.Byte)
        self.assertIsInstance(self.fixture.callObject(
            np.int16(1)), java.lang.Short)
        self.assertIsInstance(self.fixture.callObject(
            np.int32(1)), java.lang.Integer)
        self.assertIsInstance(self.fixture.callObject(
            np.int64(1)), java.lang.Long)
        self.assertIsInstance(self.fixture.callObject(
            np.float32(1)), java.lang.Float)
        self.assertIsInstance(self.fixture.callObject(
            np.float64(1)), java.lang.Double)

    def testCompare(self):
        jo = JClass("java.lang.Object")()
        with self.assertRaises(TypeError):
            jo < 0
        with self.assertRaises(TypeError):
            jo <= 0
        with self.assertRaises(TypeError):
            jo > 0
        with self.assertRaises(TypeError):
            jo >= 0

    def testCompareNull(self):
        jo = JClass("java.lang.Object")
        jv = JObject(None, jo)
        self.assertEqual(jv, None)
        self.assertEqual(None, jv)
        self.assertNotEqual(JInt(1), jv)
        self.assertNotEqual(jv, JInt(1))

    def testRepr(self):
        jo = JClass("java.lang.Object")
        jv = jo()
        jvn = JObject(None, jo)
        self.assertIsInstance(repr(jv), str)
        self.assertIsInstance(repr(jvn), str)
        self.assertEqual(repr(jv), "<java object 'java.lang.Object'>")
        self.assertEqual(repr(jvn), "<java object 'java.lang.Object'>")

    def testDeprecated(self):
        # this one should issue a warning
        jo = JClass("java.lang.Object")
        self.assertIsInstance(JObject(None, object), jo)

    def testGetSetBad(self):
        JS = JClass("java.lang.String")
        js = JS()
        with self.assertRaises(TypeError):
            JS.__getattribute__(js, object())
        with self.assertRaises(TypeError):
            setattr(js, object(), 1)

    def testGetSetBad(self):
        jo = JClass("java.lang.Object")()
        self.assertTrue(jo != JInt(0))
        self.assertFalse(jo == JInt(0))
        self.assertTrue(JInt(0) != jo)
        self.assertFalse(JInt(0) == jo)
