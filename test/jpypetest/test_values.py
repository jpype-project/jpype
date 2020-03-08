import sys
import jpype
from jpype.types import *
import common


class ValuesTestCase(common.JPypeTestCase):
    """ Test of type conversion for fields. """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.fixture = JClass('jpype.common.Fixture')()

    def testIntFromFloatWrapper(self):
        self.fixture.int_field = JInt(6.0)
        self.assertEqual(self.fixture.int_field, 6)

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

    def testStringDictKey1(self):
        d = dict()
        d['foo'] = 'a'
        self.assertEqual(d[JString('foo')], 'a')

    def testStringDictKey2(self):
        d = dict()
        d[JString('foo')] = 'a'
        self.assertEqual(d['foo'], 'a')

    def testStringDictKey3(self):
        d = dict()
        d[JString('foo')] = 'a'
        self.assertEqual(d[JString('foo')], 'a')
