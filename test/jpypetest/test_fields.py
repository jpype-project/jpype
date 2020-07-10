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
from jpype import JPackage, java
import common


class FieldsTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.cls = JClass("jpype.common.Fixture")
        self.obj = self.cls()

    def testBoolean(self):
        self.obj.bool_field = True
        self.assertTrue(self.obj.bool_field)
        self.assertTrue(self.obj.getBool())
        self.obj.bool_field = False
        self.assertFalse(self.obj.bool_field)
        self.assertFalse(self.obj.getBool())

        self.obj.static_bool_field = True
        self.assertTrue(self.obj.static_bool_field)
        self.assertTrue(self.cls.static_bool_field)
        self.obj.static_bool_field = False
        self.assertFalse(self.obj.static_bool_field)
        self.assertFalse(self.cls.static_bool_field)

        with self.assertRaises(AttributeError):
            self.cls.bool_field = True
        with self.assertRaises(AttributeError):
            self.cls.bool_field = False

        self.cls.static_bool_field = True
        self.assertTrue(self.cls.static_bool_field)
        self.assertTrue(self.cls.getStaticBool())
        self.cls.static_bool_field = False
        self.assertFalse(self.cls.static_bool_field)
        self.assertFalse(self.cls.getStaticBool())

    def testChar(self):
        self.obj.char_field = 'Q'
        self.assertEqual(self.obj.char_field, 'Q')
        self.assertEqual(self.obj.getChar(), 'Q')

        self.obj.static_char_field = 'S'
        self.assertEqual(self.obj.static_char_field, 'S')
        self.assertEqual(self.cls.static_char_field, 'S')
        self.assertEqual(self.obj.getStaticChar(), 'S')
        self.assertEqual(self.cls.getStaticChar(), 'S')

        with self.assertRaises(AttributeError):
            self.cls.char_field = 'U'
        with self.assertRaises(AttributeError):
            self.cls.char_field = 'V'

        self.cls.static_char_field = 'W'
        self.assertEqual(self.cls.static_char_field, 'W')
        self.assertEqual(self.obj.getStaticChar(), 'W')
        self.assertEqual(self.cls.getStaticChar(), 'W')

    def testByte(self):
        self.obj.byte_field = 34
        self.assertEqual(self.obj.byte_field, 34)
        self.assertEqual(self.obj.getByte(), 34)

        self.obj.static_byte_field = 36
        self.assertEqual(self.obj.static_byte_field, 36)
        self.assertEqual(self.cls.static_byte_field, 36)
        self.assertEqual(self.obj.getStaticByte(), 36)
        self.assertEqual(self.cls.getStaticByte(), 36)

        with self.assertRaises(AttributeError):
            self.cls.byte_field = 38
        with self.assertRaises(AttributeError):
            self.cls.byte_field = 39

        self.cls.static_byte_field = 40
        self.assertEqual(self.cls.static_byte_field, 40)
        self.assertEqual(self.obj.getStaticByte(), 40)
        self.assertEqual(self.cls.getStaticByte(), 40)

    def testShort(self):
        self.obj.short_field = 34
        self.assertEqual(self.obj.short_field, 34)
        self.assertEqual(self.obj.getShort(), 34)

        self.obj.static_short_field = 36
        self.assertEqual(self.obj.static_short_field, 36)
        self.assertEqual(self.cls.static_short_field, 36)
        self.assertEqual(self.obj.getStaticShort(), 36)
        self.assertEqual(self.cls.getStaticShort(), 36)

        with self.assertRaises(AttributeError):
            self.cls.short_field = 38
        with self.assertRaises(AttributeError):
            self.cls.short_field = 39

        self.cls.static_short_field = 40
        self.assertEqual(self.cls.static_short_field, 40)
        self.assertEqual(self.obj.getStaticShort(), 40)
        self.assertEqual(self.cls.getStaticShort(), 40)

    def testInt(self):
        self.obj.int_field = 34
        self.assertEqual(self.obj.int_field, 34)
        self.assertEqual(self.obj.getInt(), 34)

        self.obj.static_int_field = 36
        self.assertEqual(self.obj.static_int_field, 36)
        self.assertEqual(self.cls.static_int_field, 36)
        self.assertEqual(self.obj.getStaticInt(), 36)
        self.assertEqual(self.cls.getStaticInt(), 36)

        with self.assertRaises(AttributeError):
            self.cls.int_field = 38
        with self.assertRaises(AttributeError):
            self.cls.int_field = 39

        self.cls.static_int_field = 40
        self.assertEqual(self.cls.static_int_field, 40)
        self.assertEqual(self.obj.getStaticInt(), 40)
        self.assertEqual(self.cls.getStaticInt(), 40)

    def testLong(self):
        self.obj.long_field = 34
        self.assertEqual(self.obj.long_field, 34)
        self.assertEqual(self.obj.getLong(), 34)

        self.obj.static_long_field = 36
        self.assertEqual(self.obj.static_long_field, 36)
        self.assertEqual(self.cls.static_long_field, 36)
        self.assertEqual(self.obj.getStaticLong(), 36)
        self.assertEqual(self.cls.getStaticLong(), 36)

        with self.assertRaises(AttributeError):
            self.cls.long_field = 38
        with self.assertRaises(AttributeError):
            self.cls.long_field = 39

        self.cls.static_long_field = 40
        self.assertEqual(self.cls.static_long_field, 40)
        self.assertEqual(self.obj.getStaticLong(), 40)
        self.assertEqual(self.cls.getStaticLong(), 40)

    def testFloat(self):
        self.obj.float_field = 34
        self.assertEqual(self.obj.float_field, 34)
        self.assertEqual(self.obj.getFloat(), 34)

        self.obj.static_float_field = 36
        self.assertEqual(self.obj.static_float_field, 36)
        self.assertEqual(self.cls.static_float_field, 36)
        self.assertEqual(self.obj.getStaticFloat(), 36)
        self.assertEqual(self.cls.getStaticFloat(), 36)

        with self.assertRaises(AttributeError):
            self.cls.float_field = 38
        with self.assertRaises(AttributeError):
            self.cls.float_field = 39

        self.cls.static_float_field = 40
        self.assertEqual(self.cls.static_float_field, 40)
        self.assertEqual(self.obj.getStaticFloat(), 40)
        self.assertEqual(self.cls.getStaticFloat(), 40)

    def testDouble(self):
        self.obj.double_field = 34
        self.assertEqual(self.obj.double_field, 34)
        self.assertEqual(self.obj.getDouble(), 34)

        self.obj.static_double_field = 36
        self.assertEqual(self.obj.static_double_field, 36)
        self.assertEqual(self.cls.static_double_field, 36)
        self.assertEqual(self.obj.getStaticDouble(), 36)
        self.assertEqual(self.cls.getStaticDouble(), 36)

        with self.assertRaises(AttributeError):
            self.cls.double_field = 38
        with self.assertRaises(AttributeError):
            self.cls.double_field = 39

        self.cls.static_double_field = 40
        self.assertEqual(self.cls.static_double_field, 40)
        self.assertEqual(self.obj.getStaticDouble(), 40)
        self.assertEqual(self.cls.getStaticDouble(), 40)

    def testObject(self):
        self.obj.object_field = "Alice"
        self.assertEqual(self.obj.object_field, "Alice")
        self.assertEqual(self.obj.getObject(), "Alice")

        self.obj.static_object_field = "Bob"
        self.assertEqual(self.obj.static_object_field, "Bob")
        self.assertEqual(self.cls.static_object_field, "Bob")
        self.assertEqual(self.obj.getStaticObject(), "Bob")
        self.assertEqual(self.cls.getStaticObject(), "Bob")

        with self.assertRaises(AttributeError):
            self.cls.object_field = "Xena"
        with self.assertRaises(AttributeError):
            self.cls.object_field = "Gabrielle"

        self.cls.static_object_field = "Charlie"
        self.assertEqual(self.cls.static_object_field, "Charlie")
        self.assertEqual(self.obj.getStaticObject(), "Charlie")
        self.assertEqual(self.cls.getStaticObject(), "Charlie")
