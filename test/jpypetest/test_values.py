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
import sys
import jpype
from jpype.types import *
import common


class ValuesTestCase(common.JPypeTestCase):
    """ Test of type conversion for fields. """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.fixture = JClass('jpype.common.Fixture')()

# Int
    def testIntFromInt(self):
        self.fixture.intField = 1
        self.assertEqual(self.fixture.int_field, 1)

    def testIntFromInt(self):
        with self.assertRaises(TypeError):
            self.fixture.int_field = 7.2

    def testIntFromFloat(self):
        with self.assertRaises(TypeError):
            self.fixture.int_field = 2.1

# Wrappers (must be exact currently)
    def testIntFromIntWrapper(self):
        self.fixture.int_field = JInt(5)
        self.assertEqual(self.fixture.int_field, 5)

# This one fails as it seems to be casting the 6.0 to an integer value literally.  I am not sure if that is intended behavior.
#    def testIntFromFloatWrapper(self):
#        self.fixture.intField = JInt(6.0)
#        self.assertEqual(self.fixture.intField,6)

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
