import jpype
from jpype.types import *
from jpype import JPackage, java
import common

class FieldsTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.cls = JClass("jpype.fields.Fields")
        self.obj = self.cls()

    def testBoolean(self):
        self.obj.member_bool = True
        self.assertTrue(self.obj.member_bool)
        self.assertTrue(self.obj.getMemberBool())
        self.obj.member_bool = False
        self.assertFalse(self.obj.member_bool)
        self.assertFalse(self.obj.getMemberBool())

        self.obj.static_bool = True
        self.assertTrue(self.obj.static_bool)
        self.assertTrue(self.cls.static_bool)
        self.obj.static_bool = False
        self.assertFalse(self.obj.static_bool)
        self.assertFalse(self.cls.static_bool)

        with self.assertRaises(AttributeError):
            self.cls.member_bool = True
        with self.assertRaises(AttributeError):
            self.cls.member_bool = False

        self.cls.static_bool = True
        self.assertTrue(self.cls.static_bool)
        self.assertTrue(self.cls.getStaticBool())
        self.cls.static_bool = False
        self.assertFalse(self.cls.static_bool)
        self.assertFalse(self.cls.getStaticBool())

    def testChar(self):
        self.obj.member_char = 'Q'
        self.assertEqual(self.obj.member_char, 'Q')
        self.assertEqual(self.obj.getMemberChar(), 'Q')

        self.obj.static_char = 'S'
        self.assertEqual(self.obj.static_char, 'S')
        self.assertEqual(self.cls.static_char, 'S')
        self.assertEqual(self.obj.getStaticChar(), 'S')
        self.assertEqual(self.cls.getStaticChar(), 'S')

        with self.assertRaises(AttributeError):
            self.cls.member_char = 'U'
        with self.assertRaises(AttributeError):
            self.cls.member_char = 'V'

        self.cls.static_char = 'W'
        self.assertEqual(self.cls.static_char, 'W')
        self.assertEqual(self.obj.getStaticChar(), 'W')
        self.assertEqual(self.cls.getStaticChar(), 'W')

    def testByte(self):
        self.obj.member_byte = 34
        self.assertEqual(self.obj.member_byte, 34)
        self.assertEqual(self.obj.getMemberByte(), 34)

        self.obj.static_byte = 36
        self.assertEqual(self.obj.static_byte, 36)
        self.assertEqual(self.cls.static_byte, 36)
        self.assertEqual(self.obj.getStaticByte(), 36)
        self.assertEqual(self.cls.getStaticByte(), 36)

        with self.assertRaises(AttributeError):
            self.cls.member_byte = 38
        with self.assertRaises(AttributeError):
            self.cls.member_byte = 39

        self.cls.static_byte = 40
        self.assertEqual(self.cls.static_byte, 40)
        self.assertEqual(self.obj.getStaticByte(), 40)
        self.assertEqual(self.cls.getStaticByte(), 40)

    def testShort(self):
        self.obj.member_short = 34
        self.assertEqual(self.obj.member_short, 34)
        self.assertEqual(self.obj.getMemberShort(), 34)

        self.obj.static_short = 36
        self.assertEqual(self.obj.static_short, 36)
        self.assertEqual(self.cls.static_short, 36)
        self.assertEqual(self.obj.getStaticShort(), 36)
        self.assertEqual(self.cls.getStaticShort(), 36)

        with self.assertRaises(AttributeError):
            self.cls.member_short = 38
        with self.assertRaises(AttributeError):
            self.cls.member_short = 39

        self.cls.static_short = 40
        self.assertEqual(self.cls.static_short, 40)
        self.assertEqual(self.obj.getStaticShort(), 40)
        self.assertEqual(self.cls.getStaticShort(), 40)

    def testInt(self):
        self.obj.member_int = 34
        self.assertEqual(self.obj.member_int, 34)
        self.assertEqual(self.obj.getMemberInt(), 34)

        self.obj.static_int = 36
        self.assertEqual(self.obj.static_int, 36)
        self.assertEqual(self.cls.static_int, 36)
        self.assertEqual(self.obj.getStaticInt(), 36)
        self.assertEqual(self.cls.getStaticInt(), 36)

        with self.assertRaises(AttributeError):
            self.cls.member_int = 38
        with self.assertRaises(AttributeError):
            self.cls.member_int = 39

        self.cls.static_int = 40
        self.assertEqual(self.cls.static_int, 40)
        self.assertEqual(self.obj.getStaticInt(), 40)
        self.assertEqual(self.cls.getStaticInt(), 40)

    def testLong(self):
        self.obj.member_long = 34
        self.assertEqual(self.obj.member_long, 34)
        self.assertEqual(self.obj.getMemberLong(), 34)

        self.obj.static_long = 36
        self.assertEqual(self.obj.static_long, 36)
        self.assertEqual(self.cls.static_long, 36)
        self.assertEqual(self.obj.getStaticLong(), 36)
        self.assertEqual(self.cls.getStaticLong(), 36)

        with self.assertRaises(AttributeError):
            self.cls.member_long = 38
        with self.assertRaises(AttributeError):
            self.cls.member_long = 39

        self.cls.static_long = 40
        self.assertEqual(self.cls.static_long, 40)
        self.assertEqual(self.obj.getStaticLong(), 40)
        self.assertEqual(self.cls.getStaticLong(), 40)

    def testFloat(self):
        self.obj.member_float = 34
        self.assertEqual(self.obj.member_float, 34)
        self.assertEqual(self.obj.getMemberFloat(), 34)

        self.obj.static_float = 36
        self.assertEqual(self.obj.static_float, 36)
        self.assertEqual(self.cls.static_float, 36)
        self.assertEqual(self.obj.getStaticFloat(), 36)
        self.assertEqual(self.cls.getStaticFloat(), 36)

        with self.assertRaises(AttributeError):
            self.cls.member_float = 38
        with self.assertRaises(AttributeError):
            self.cls.member_float = 39

        self.cls.static_float = 40
        self.assertEqual(self.cls.static_float, 40)
        self.assertEqual(self.obj.getStaticFloat(), 40)
        self.assertEqual(self.cls.getStaticFloat(), 40)

    def testDouble(self):
        self.obj.member_double = 34
        self.assertEqual(self.obj.member_double, 34)
        self.assertEqual(self.obj.getMemberDouble(), 34)

        self.obj.static_double = 36
        self.assertEqual(self.obj.static_double, 36)
        self.assertEqual(self.cls.static_double, 36)
        self.assertEqual(self.obj.getStaticDouble(), 36)
        self.assertEqual(self.cls.getStaticDouble(), 36)

        with self.assertRaises(AttributeError):
            self.cls.member_double = 38
        with self.assertRaises(AttributeError):
            self.cls.member_double = 39

        self.cls.static_double = 40
        self.assertEqual(self.cls.static_double, 40)
        self.assertEqual(self.obj.getStaticDouble(), 40)
        self.assertEqual(self.cls.getStaticDouble(), 40)

    def testObject(self):
        self.obj.member_object = "Alice"
        self.assertEqual(self.obj.member_object, "Alice")
        self.assertEqual(self.obj.getMemberObject(), "Alice")

        self.obj.static_object = "Bob"
        self.assertEqual(self.obj.static_object, "Bob")
        self.assertEqual(self.cls.static_object, "Bob")
        self.assertEqual(self.obj.getStaticObject(), "Bob")
        self.assertEqual(self.cls.getStaticObject(), "Bob")

        with self.assertRaises(AttributeError):
            self.cls.member_object = "Xena"
        with self.assertRaises(AttributeError):
            self.cls.member_object = "Gabrielle"

        self.cls.static_object = "Charlie"
        self.assertEqual(self.cls.static_object, "Charlie")
        self.assertEqual(self.obj.getStaticObject(), "Charlie")
        self.assertEqual(self.cls.getStaticObject(), "Charlie")




