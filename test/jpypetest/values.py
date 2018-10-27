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
try:
    import unittest2 as unittest
except ImportError:
    import unittest
import sys
import jpype
from . import common

# Python2/3 support
if sys.version > '3':
    long = int
    unicode = str

# Test code


class ValuesTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Fields = jpype.JClass('jpype.values.FieldsTest')

# Int
    def testIntFromInt(self):
        self.Fields.intField = 1
        self.assertEquals(self.Fields.intField, 1)

    def testIntFromInt(self):
        with self.assertRaises(TypeError):
            self.Fields.intField = 7.2

    def testIntFromFloat(self):
        with self.assertRaises(TypeError):
            self.Fields.intField = 2.1

# Float
    def testFloatFromInt(self):
        self.Fields.floatField = 1
        self.assertEquals(self.Fields.floatField, 1.0)

    def testFloatFromFloat(self):
        self.Fields.floatField = 2.0
        self.assertEquals(self.Fields.floatField, 2.0)

# Double
    def testDoubleFromInt(self):
        self.Fields.doubleField = 1
        self.assertEquals(self.Fields.doubleField, 1.0)

    def testDoubleFromFloat(self):
        self.Fields.doubleField = 2.0
        self.assertEquals(self.Fields.doubleField, 2.0)

# Wrappers (must be exact currently)
    def testIntFromIntWrapper(self):
        self.Fields.intField = jpype.JInt(5)
        self.assertEquals(self.Fields.intField, 5)

# This one fails as it seems to be casting the 6.0 to an integer value literally.  I am not sure if that is intended behavior.
#    def testIntFromFloatWrapper(self):
#        self.Fields.intField = jpype.JInt(6.0)
#        self.assertEquals(self.Fields.intField,6)

    def testFloatFromFloatWrapper(self):
        self.Fields.floatField = jpype.JFloat(5.0)
        self.assertEquals(self.Fields.floatField, 5.0)

    def testDoubleFromDoubleWrapper(self):
        self.Fields.doubleField = jpype.JDouble(5.0)
        self.assertEquals(self.Fields.doubleField, 5.0)

    def testObjectBoolTrue(self):
        self.Fields.objectField = True
        self.assertIsInstance(self.Fields.objectField, jpype.JClass('java.lang.Boolean'))
        self.assertEquals(str(self.Fields.objectField), str(True))
        self.assertEquals(self.Fields.objectField, True)

    def testObjectBoolFalse(self):
        self.Fields.objectField = False
        self.assertIsInstance(self.Fields.objectField, jpype.JClass('java.lang.Boolean'))
        self.assertEquals(str(self.Fields.objectField), str(False))
        self.assertEquals(self.Fields.objectField, False)

    def testObjectBoolJValue(self):
        self.Fields.objectField = jpype.JBoolean(True)
        self.assertIsInstance(self.Fields.objectField, jpype.JClass('java.lang.Boolean'))
        self.assertEquals(self.Fields.objectField, True)

    def testObjectShort(self):
        self.Fields.objectField = jpype.JShort(1)
        self.assertEquals(self.Fields.objectField, 1)
        self.assertIsInstance(self.Fields.objectField, jpype.JClass('java.lang.Short'))

    def testObjectInteger(self):
        self.Fields.objectField = jpype.JInt(2)
        self.assertEquals(self.Fields.objectField, 2)
        self.assertIsInstance(self.Fields.objectField, jpype.JClass('java.lang.Integer'))

    def testObjectLong(self):
        self.Fields.objectField = jpype.JLong(3)
        self.assertEquals(self.Fields.objectField, 3)
        self.assertIsInstance(self.Fields.objectField, jpype.JClass('java.lang.Long'))

    def testObjectFloat(self):
        self.Fields.objectField = jpype.JFloat(1.125)
        self.assertEquals(self.Fields.objectField, 1.125)
        self.assertIsInstance(self.Fields.objectField, jpype.JClass('java.lang.Float'))

    def testObjectDouble(self):
        self.Fields.objectField = jpype.JDouble(2.6125)
        self.assertEquals(self.Fields.objectField, 2.6125)
        self.assertIsInstance(self.Fields.objectField, jpype.JClass('java.lang.Double'))

