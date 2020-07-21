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
import common
import jpype
import _jpype
from jpype.types import *


class ConversionTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.jc1 = jpype.JClass("java.lang.String")
        self.jc2 = jpype.JClass("java.lang.Integer")

    def testList(self):
        cls = JClass('jpype.collection.CollectionTest')
        self.assertIsInstance(cls.testList(
            [1, 2, 3]), JClass('java.util.List'))

    def testMap(self):
        cls = JClass('jpype.collection.CollectionTest')
        self.assertIsInstance(cls.testMap(
            {'a': 1, 'b': 2}), JClass('java.util.Map'))

    def testCanConvertExact(self):
        self.assertEqual(self.jc1._canConvertToJava("a"), "exact")

    def testCanConvertNone(self):
        self.assertEqual(self.jc1._canConvertToJava(1), "none")

    def testCanConvertExplicit(self):
        self.assertEqual(
            self.jc2._canConvertToJava(1), "explicit")

    def testCanConvertImplicit(self):
        self.assertEqual(
            self.jc1._canConvertToJava(None), "implicit")

    def testConvertExact(self):
        self.assertIsInstance(
            self.jc1._convertToJava("a"), self.jc1)

    def testConvertImplicit(self):
        self.assertIsInstance(
            self.jc1._convertToJava(None), self.jc1)

    def testConvertExplicit(self):
        self.assertIsInstance(
            self.jc2._convertToJava(1), self.jc2)

    def testConvertNone(self):
        with self.assertRaises(TypeError):
            self.jc1._convertToJava(1)

    def testUnbox(self):
        jf = JClass('jpype.common.Fixture')
        java = jpype.java
        jf.static_bool_field = java.lang.Boolean(True)
        self.assertEqual(jf.static_bool_field, True)
        jf.static_char_field = java.lang.Character("a")
        self.assertEqual(jf.static_char_field, "a")
        jf.static_byte_field = java.lang.Byte(123)
        self.assertEqual(jf.static_byte_field, 123)
        jf.static_short_field = java.lang.Short(123)
        self.assertEqual(jf.static_short_field, 123)
        jf.static_int_field = java.lang.Integer(123)
        self.assertEqual(jf.static_int_field, 123)
        jf.static_long_field = java.lang.Long(123)
        self.assertEqual(jf.static_long_field, 123)
        jf.static_float_field = java.lang.Float(123)
        self.assertEqual(jf.static_float_field, 123)
        jf.static_double_field = java.lang.Double(123)
        self.assertEqual(jf.static_double_field, 123)

    def testUnboxFail(self):
        java = jpype.java
        with self.assertRaises(TypeError):
            JBoolean._convertToJava(java.lang.Double(1))
        with self.assertRaises(TypeError):
            JChar._convertToJava(java.lang.Double(1))
        with self.assertRaises(TypeError):
            JByte._convertToJava(java.lang.Boolean(1))
        with self.assertRaises(TypeError):
            JShort._convertToJava(java.lang.Boolean(1))
        with self.assertRaises(TypeError):
            JInt._convertToJava(java.lang.Boolean(1))
        with self.assertRaises(TypeError):
            JLong._convertToJava(java.lang.Boolean(1))
        with self.assertRaises(TypeError):
            JFloat._convertToJava(java.lang.Boolean(1))
        with self.assertRaises(TypeError):
            JDouble._convertToJava(java.lang.Boolean(1))

    def testBox(self):
        java = jpype.java
        self.assertEqual(java.lang.Boolean(JBoolean(True)), True)
        # FIXME this one fails
        #self.assertEqual(java.lang.Character(JChar("A")), "A")
        self.assertEqual(java.lang.Byte(JByte(123)), 123)
        self.assertEqual(java.lang.Short(JShort(123)), 123)
        self.assertEqual(java.lang.Integer(JInt(123)), 123)
        self.assertEqual(java.lang.Long(JLong(123)), 123)
        self.assertEqual(java.lang.Float(JFloat(123)), 123)
        self.assertEqual(java.lang.Double(JDouble(123)), 123)

    def testCharConversion(self):
        self.assertEqual(JChar._canConvertToJava("a"), "implicit")
        self.assertEqual(JChar._canConvertToJava(bytes([1])), "implicit")
        self.assertEqual(JChar._canConvertToJava(bytes([1, 1])), "none")

     # This test is wrong 'char q = (char) 1000000;' works in Java
#    def testCharOverflow(self):
#        with self.assertRaises(OverflowError):
#            JChar(1000000)

    def testCharBytes(self):
        # setArrayRange directly calls Char conversion so it is a good way
        # to test without checking if conversion is possible first
        ja = JArray(JChar)(2)
        with self.assertRaises(ValueError):
            ja[0:1] = [bytes([1, 1, 1])]
        with self.assertRaises(OverflowError):
            ja[0:1] = [1000000]
        with self.assertRaises(ValueError):
            ja[0:1] = ["AAA"]
        with self.assertRaises(ValueError):
            ja[0:1] = ["\U0001F600"]
        with self.assertRaises(TypeError):
            ja[0:1] = [object()]
        ja[0:1] = ["\u265E"]
