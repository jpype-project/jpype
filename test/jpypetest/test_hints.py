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
import jpype.protocol as proto


class HintsTestCase(common.JPypeTestCase):

    def testCache(self):
        cls = JClass('java.lang.Object')
        hints = cls._hints
        hints2 = cls._hints
        self.assertEqual(hints, hints2)

    def testProtocol(self):
        protocol = jpype.protocol.SupportsFloat
        annot = protocol.__float__.__annotations__
        self.assertEqual(annot['return'], float)
        protocol = jpype.protocol.SupportsIndex
        annot = protocol.__index__.__annotations__
        self.assertEqual(annot['return'], int)
        self.assertTrue(hasattr(proto, "Sequence"))
        self.assertTrue(hasattr(proto, "Mapping"))
        self.assertTrue(hasattr(proto, "Protocol"))

    def testObject(self):
        cls = JClass('java.lang.Object')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(str in hints.implicit)
        self.assertTrue(bool in hints.implicit)
        self.assertTrue(proto.SupportsIndex in hints.implicit)
        self.assertTrue(proto.SupportsFloat in hints.implicit)
        self.assertTrue(proto._JClass in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(len(hints.none) == 0)

    def testNumber(self):
        cls = JClass('java.lang.Number')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(proto._JNumberLong in hints.implicit)
        self.assertTrue(proto._JNumberFloat in hints.implicit)
        self.assertTrue(proto.SupportsIndex in hints.implicit)
        self.assertTrue(proto.SupportsFloat in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(len(hints.none) == 0)

    def testString(self):
        cls = JClass('java.lang.String')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(str in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(len(hints.none) == 0)

    def testClass(self):
        cls = JClass('java.lang.Class')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(proto._JClass in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(len(hints.none) == 0)

    def testCollection(self):
        cls = JClass('java.util.Collection')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(proto.Sequence in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(str in hints.none)

    def testList(self):
        cls = JClass('java.util.List')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(len(hints.implicit) == 0)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(len(hints.none) == 0)

    def testJBoolean(self):
        cls = JBoolean
        hints = cls._hints
        self.assertTrue(bool in hints.returns)
        self.assertTrue(bool in hints.exact)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JClass("java.lang.Boolean") in hints.implicit)
        self.assertTrue(proto.SupportsIndex in hints.explicit)
        self.assertTrue(proto.SupportsFloat in hints.explicit)
        self.assertTrue(len(hints.none) == 0)

    def testJChar(self):
        cls = JChar
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JClass("java.lang.Character") in hints.implicit)
        self.assertTrue(str in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(len(hints.none) == 0)

    def testJShort(self):
        cls = JShort
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JByte in hints.implicit)
        self.assertTrue(JChar in hints.implicit)
        self.assertTrue(JClass("java.lang.Short") in hints.implicit)
        self.assertTrue(proto.SupportsIndex in hints.implicit)
        self.assertTrue(proto.SupportsFloat in hints.explicit)
        self.assertTrue(len(hints.none) == 0)

    def testJInt(self):
        cls = JInt
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JByte in hints.implicit)
        self.assertTrue(JChar in hints.implicit)
        self.assertTrue(JShort in hints.implicit)
        self.assertTrue(JClass('java.lang.Integer') in hints.implicit)
        self.assertTrue(proto.SupportsIndex in hints.implicit)
        self.assertTrue(proto.SupportsFloat in hints.explicit)
        self.assertTrue(len(hints.none) == 0)

    def testJLong(self):
        cls = JLong
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JByte in hints.implicit)
        self.assertTrue(JChar in hints.implicit)
        self.assertTrue(JShort in hints.implicit)
        self.assertTrue(JInt in hints.implicit)
        self.assertTrue(JClass('java.lang.Long') in hints.implicit)
        self.assertTrue(proto.SupportsIndex in hints.implicit)
        self.assertTrue(proto.SupportsFloat in hints.explicit)
        self.assertTrue(len(hints.none) == 0)

    def testJFloat(self):
        cls = JFloat
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JByte in hints.implicit)
        self.assertTrue(JChar in hints.implicit)
        self.assertTrue(JShort in hints.implicit)
        self.assertTrue(JInt in hints.implicit)
        self.assertTrue(JLong in hints.implicit)
        self.assertTrue(JClass('java.lang.Float') in hints.implicit)
        self.assertTrue(int in hints.implicit)
        self.assertTrue(proto.SupportsIndex in hints.implicit)
        self.assertTrue(proto.SupportsFloat in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(len(hints.none) == 0)

    def testJDouble(self):
        cls = JDouble
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JByte in hints.implicit)
        self.assertTrue(JChar in hints.implicit)
        self.assertTrue(JShort in hints.implicit)
        self.assertTrue(JInt in hints.implicit)
        self.assertTrue(JLong in hints.implicit)
        self.assertTrue(JFloat in hints.implicit)
        self.assertTrue(JClass('java.lang.Double') in hints.implicit)
        self.assertTrue(proto.SupportsIndex in hints.implicit)
        self.assertTrue(proto.SupportsFloat in hints.implicit)
        self.assertTrue(int in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(len(hints.none) == 0)

    def testBoxedByte(self):
        cls = JClass('java.lang.Byte')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JByte in hints.implicit)
        self.assertTrue(proto.SupportsFloat in hints.explicit)
        self.assertTrue(JClass('java.lang.Byte') in hints.explicit)
        self.assertTrue(proto.SupportsIndex in hints.explicit)
        self.assertTrue(len(hints.none) == 0)

    def testBoxedBoolean(self):
        cls = JClass('java.lang.Boolean')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(bool in hints.implicit)
        self.assertTrue(JBoolean in hints.implicit)
        self.assertTrue(proto.SupportsIndex in hints.explicit)
        self.assertTrue(proto.SupportsFloat in hints.explicit)
        self.assertTrue(JClass('java.lang.Boolean') in hints.explicit)
        self.assertTrue(len(hints.none) == 0)

    def testBoxedCharacter(self):
        cls = JClass('java.lang.Character')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JChar in hints.implicit)
        self.assertTrue(JClass('java.lang.Character') in hints.explicit)
        self.assertTrue(str in hints.explicit)
        self.assertTrue(len(hints.none) == 0)

    def testBoxedShort(self):
        cls = JClass('java.lang.Short')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JShort in hints.implicit)
        self.assertTrue(proto.SupportsFloat in hints.explicit)
        self.assertTrue(JByte in hints.explicit)
        self.assertTrue(JChar in hints.explicit)
        self.assertTrue(JClass('java.lang.Short') in hints.explicit)
        self.assertTrue(proto.SupportsIndex in hints.explicit)
        self.assertTrue(len(hints.none) == 0)

    def testBoxedInteger(self):
        cls = JClass('java.lang.Integer')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JInt in hints.implicit)
        self.assertTrue(proto.SupportsFloat in hints.explicit)
        self.assertTrue(JByte in hints.explicit)
        self.assertTrue(JChar in hints.explicit)
        self.assertTrue(JShort in hints.explicit)
        self.assertTrue(JClass('java.lang.Integer') in hints.explicit)
        self.assertTrue(proto.SupportsIndex in hints.explicit)
        self.assertTrue(len(hints.none) == 0)

    def testBoxedLong(self):
        cls = JClass('java.lang.Long')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JLong in hints.implicit)
        self.assertTrue(proto.SupportsFloat in hints.explicit)
        self.assertTrue(JByte in hints.explicit)
        self.assertTrue(JChar in hints.explicit)
        self.assertTrue(JShort in hints.explicit)
        self.assertTrue(JInt in hints.explicit)
        self.assertTrue(JClass('java.lang.Long') in hints.explicit)
        self.assertTrue(proto.SupportsIndex in hints.explicit)
        self.assertTrue(len(hints.none) == 0)

    def testBoxedFloat(self):
        cls = JClass('java.lang.Float')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JFloat in hints.implicit)
        self.assertTrue(JByte in hints.explicit)
        self.assertTrue(JChar in hints.explicit)
        self.assertTrue(JShort in hints.explicit)
        self.assertTrue(JInt in hints.explicit)
        self.assertTrue(JLong in hints.explicit)
        self.assertTrue(JClass('java.lang.Float') in hints.explicit)
        self.assertTrue(int in hints.explicit)
        self.assertTrue(proto.SupportsIndex in hints.explicit)
        self.assertTrue(proto.SupportsFloat in hints.explicit)
        self.assertTrue(len(hints.none) == 0)

    def testBoxedDouble(self):
        cls = JClass('java.lang.Double')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(JDouble in hints.implicit)
        self.assertTrue(JByte in hints.explicit)
        self.assertTrue(JChar in hints.explicit)
        self.assertTrue(JShort in hints.explicit)
        self.assertTrue(JInt in hints.explicit)
        self.assertTrue(JLong in hints.explicit)
        self.assertTrue(JFloat in hints.explicit)
        self.assertTrue(JClass('java.lang.Double') in hints.explicit)
        self.assertTrue(proto.SupportsIndex in hints.explicit)
        self.assertTrue(proto.SupportsFloat in hints.explicit)
        self.assertTrue(int in hints.explicit)
        self.assertTrue(len(hints.none) == 0)

    def testObjectArray(self):
        cls = JClass('java.lang.Object[]')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(proto.Sequence in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(str in hints.none)

    def testBooleanArray(self):
        cls = JClass('boolean[]')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(proto.Sequence in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(str in hints.none)

    def testCharArray(self):
        cls = JClass('char[]')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(str in hints.implicit)
        self.assertTrue(proto.Sequence in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(len(hints.none) == 0)

    def testShortArray(self):
        cls = JClass('short[]')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(proto.Sequence in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(str in hints.none)

    def testIntArray(self):
        cls = JClass('int[]')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(proto.Sequence in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(str in hints.none)

    def testLongArray(self):
        cls = JClass('long[]')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(proto.Sequence in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(str in hints.none)

    def testFloatArray(self):
        cls = JClass('float[]')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(proto.Sequence in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(str in hints.none)

    def testDoubleArray(self):
        cls = JClass('double[]')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(proto.Sequence in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(str in hints.none)

    def testPath(self):
        cls = JClass('java.nio.file.Path')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(proto.SupportsPath in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(len(hints.none) == 0)

    def testFile(self):
        cls = JClass('java.io.File')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(proto.SupportsPath in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(len(hints.none) == 0)

    def testInstant(self):
        import datetime
        cls = JClass('java.time.Instant')
        hints = cls._hints
        self.assertTrue(cls in hints.returns)
        self.assertTrue(cls in hints.exact)
        self.assertTrue(datetime.datetime in hints.implicit)
        self.assertTrue(len(hints.explicit) == 0)
        self.assertTrue(len(hints.none) == 0)

    def testDate(self):
        cls = JClass("java.sql.Date")
        d = cls(120, 0, 5)
        d2 = d._py()
        d3 = JObject(d2, cls)
        self.assertEqual(d, d2)
        self.assertEqual(d, d3)

    def testTimestamp(self):
        cls = JClass("java.sql.Timestamp")
        d = cls(120, 0, 5, 9, 22, 51, 123456000)
        d2 = d._py()
        d3 = JObject(d2, cls)
        self.assertEqual(d, d2)
        self.assertEqual(d, d3)

    def testTime(self):
        cls = JClass("java.sql.Time")
        d = cls(11, 53, 1)
        d2 = d._py()
        d3 = JObject(d2, cls)
        self.assertEqual(d, d2)
        self.assertEqual(d, d3)

    def testBigDecimal(self):
        cls = JClass("java.math.BigDecimal")
        d = cls('1000234600000000000000')
        d2 = d._py()
        d3 = JObject(d2, cls)
        self.assertEqual(d, d2)
        self.assertEqual(d, d3)

    def testAddTypeBad(self):
        cls = JClass('java.lang.Object')
        with self.assertRaises(TypeError):
            cls._hints._addTypeConversion()
        with self.assertRaisesRegex(TypeError, "callable method is required"):
            cls._hints._addTypeConversion(object, object(), True)
        with self.assertRaisesRegex(TypeError, "type or protocol is required"):
            def foo():
                pass
            cls._hints._addTypeConversion(object(), foo, True)

    def testExcludeBad(self):
        cls = JClass('java.lang.Object')
        with self.assertRaisesRegex(TypeError, "type or protocol is required, not 'object'"):
            cls._hints._excludeConversion(object())
        with self.assertRaisesRegex(TypeError, "type or protocol is required, not 'object'"):
            cls._hints._excludeConversion((object(),))
