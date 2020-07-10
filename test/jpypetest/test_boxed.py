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
import sys
import jpype
import common
from jpype.types import *
from jpype import java


def passThrough(item):
    al = JClass("java.util.ArrayList")()
    al.add(item)
    return al.get(0)


class BoxedTestCase(common.JPypeTestCase):
    __name__ = "BoxedTestCase"

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.TestBoxed = jpype.JClass('jpype.boxed.Boxed')
        self.Number = jpype.JClass('java.lang.Number')
        self.Comparable = jpype.JClass('java.lang.Comparable')

    def testShort(self):
        c1 = 12345
        # Check passed from and passed to
        d1 = self.TestBoxed.newShort(c1)
        d2 = java.lang.Short(c1)
        self.assertEqual(d1, c1)
        self.assertEqual(d2, c1)
        self.assertEqual(c1, d1)
        self.assertEqual(c1, d2)
        self.assertEqual(d1, d2)
        self.assertEqual(self.TestBoxed.callShort(c1),
                         self.TestBoxed.callShort(d2))
        # Verify ops
        self.assertEqual(d1 + 2, d1 + 2)
        self.assertEqual(d1 * 2, d1 * 2)

    def testInteger(self):
        c1 = 12345
        # Check passed from and passed to
        d1 = self.TestBoxed.newInteger(c1)
        d2 = java.lang.Integer(c1)
        self.assertEqual(d1, c1)
        self.assertEqual(d2, c1)
        self.assertEqual(c1, d1)
        self.assertEqual(c1, d2)
        self.assertEqual(d1, d2)
        self.assertEqual(self.TestBoxed.callInteger(c1),
                         self.TestBoxed.callInteger(d2))
        # Verify ops
        self.assertEqual(d1 + 2, d1 + 2)
        self.assertEqual(d1 * 2, d1 * 2)

    def testLong(self):
        c1 = 12345
        # Check passed from and passed to
        d1 = self.TestBoxed.newLong(c1)
        d2 = java.lang.Long(c1)
        self.assertEqual(d1, c1)
        self.assertEqual(d2, c1)
        self.assertEqual(c1, d1)
        self.assertEqual(c1, d2)
        self.assertEqual(d1, d2)
        self.assertEqual(self.TestBoxed.callLong(c1),
                         self.TestBoxed.callLong(d2))
        # Verify ops
        self.assertEqual(d1 + 2, d1 + 2)
        self.assertEqual(d1 * 2, d1 * 2)

    def testDoubleFromFloat(self):
        java.lang.Double(1.0)

    def testFloatFromInt(self):
        java.lang.Float(1)

    def testDoubleFromInt(self):
        java.lang.Double(1)

    def testBoxed2(self):
        java.lang.Short(java.lang.Integer(1))
        java.lang.Integer(java.lang.Integer(1))
        java.lang.Long(java.lang.Integer(1))
        java.lang.Float(java.lang.Integer(1))
        java.lang.Float(java.lang.Long(1))
        java.lang.Double(java.lang.Integer(1))
        java.lang.Double(java.lang.Long(1))
        java.lang.Double(java.lang.Float(1))

    def testFloat(self):
        c1 = 123124 / 256.0
        # Check passed from and passed to
        d1 = self.TestBoxed.newFloat(c1)
        d2 = java.lang.Float(c1)
        self.assertEqual(d1, c1)
        self.assertEqual(d2, c1)
        self.assertEqual(c1, d1)
        self.assertEqual(c1, d2)
        self.assertEqual(d1, d2)
        self.assertEqual(self.TestBoxed.callFloat(c1),
                         self.TestBoxed.callFloat(d2))
        # Verify ops
        self.assertEqual(d1 + 2, d1 + 2)
        self.assertEqual(d1 * 2, d1 * 2)
        self.assertTrue(d2 < c1 + 1)
        self.assertTrue(d2 > c1 - 1)

    def testDouble(self):
        c1 = 123124 / 256.0
        # Check passed from and passed to
        d1 = self.TestBoxed.newDouble(c1)
        d2 = java.lang.Double(c1)
        self.assertEqual(d1, c1)
        self.assertEqual(d2, c1)
        self.assertEqual(c1, d1)
        self.assertEqual(c1, d2)
        self.assertEqual(d1, d2)
        self.assertEqual(self.TestBoxed.callDouble(c1),
                         self.TestBoxed.callDouble(d2))
        # Verify ops
        self.assertEqual(d1 + 2, d1 + 2)
        self.assertEqual(d1 * 2, d1 * 2)
        self.assertTrue(d2 < c1 + 1)
        self.assertTrue(d2 > c1 - 1)

    def testShortResolve(self):
        self.assertEqual(self.TestBoxed.whichShort(1), 1)
        self.assertEqual(self.TestBoxed.whichShort(java.lang.Short(1)), 2)

    def testIntegerResolve(self):
        self.assertEqual(self.TestBoxed.whichInteger(1), 1)
        self.assertEqual(self.TestBoxed.whichInteger(java.lang.Integer(1)), 2)

    def testLongResolve(self):
        self.assertEqual(self.TestBoxed.whichLong(1), 1)
        self.assertEqual(self.TestBoxed.whichLong(java.lang.Long(1)), 2)

    def testFloatResolve(self):
        self.assertEqual(self.TestBoxed.whichFloat(1.0), 1)
        self.assertEqual(self.TestBoxed.whichFloat(java.lang.Float(1.0)), 2)

    def testDoubleResolve(self):
        self.assertEqual(self.TestBoxed.whichDouble(1.0), 1)
        self.assertEqual(self.TestBoxed.whichDouble(java.lang.Double(1.0)), 2)

    def testPrivitiveToBoxed(self):
        java.lang.Boolean(JBoolean(0))
        java.lang.Byte(JByte(0))
        java.lang.Short(JShort(0))
        java.lang.Integer(JInt(0))
        java.lang.Long(JLong(0))
        java.lang.Float(JFloat(0))
        java.lang.Double(JDouble(0))

    def testBooleanBad(self):
        # java.lang.Boolean(X) works like bool(X)
        # Explicit is a cast
        self.assertFalse(java.lang.Boolean(tuple()))
        self.assertFalse(java.lang.Boolean(list()))
        self.assertFalse(java.lang.Boolean(dict()))
        self.assertFalse(java.lang.Boolean(set()))
        self.assertTrue(java.lang.Boolean(tuple(['a'])))
        self.assertTrue(java.lang.Boolean(['a']))
        self.assertTrue(java.lang.Boolean({'a': 1}))
        self.assertTrue(java.lang.Boolean(set(['a', 'b'])))

        # Implicit does not automatically cast
        fixture = JClass('jpype.common.Fixture')()
        with self.assertRaises(TypeError):
            fixture.callBoxedBoolean(tuple())
        with self.assertRaises(TypeError):
            fixture.callBoxedBoolean(list())
        with self.assertRaises(TypeError):
            fixture.callBoxedBoolean(dict())
        with self.assertRaises(TypeError):
            fixture.callBoxedBoolean(set())

    def testByteBad(self):
        with self.assertRaises(TypeError):
            java.lang.Byte(tuple())

    def testCharacterBad(self):
        with self.assertRaises(TypeError):
            java.lang.Character(tuple())

    def testShortBad(self):
        with self.assertRaises(TypeError):
            java.lang.Short(tuple())

    def testIntegerBad(self):
        with self.assertRaises(TypeError):
            java.lang.Integer(tuple())

    def testLongBad(self):
        with self.assertRaises(TypeError):
            java.lang.Long(tuple())

    def testFloatBad(self):
        with self.assertRaises(TypeError):
            java.lang.Float(tuple())

    def testDoubleBad(self):
        with self.assertRaises(TypeError):
            java.lang.Double(tuple())

    def testBooleanBad2(self):
        with self.assertRaises(TypeError):
            java.lang.Boolean(tuple(), tuple())

    def testByteBad2(self):
        with self.assertRaises(TypeError):
            java.lang.Byte(tuple(), tuple())

    def testCharacterBad2(self):
        with self.assertRaises(TypeError):
            java.lang.Character(tuple(), tuple())

    def testShortBad2(self):
        with self.assertRaises(TypeError):
            java.lang.Short(tuple(), tuple())

    def testIntegerBad2(self):
        with self.assertRaises(TypeError):
            java.lang.Integer(tuple(), tuple())

    def testLongBad2(self):
        with self.assertRaises(TypeError):
            java.lang.Long(tuple(), tuple())

    def testFloatBad2(self):
        with self.assertRaises(TypeError):
            java.lang.Float(tuple(), tuple())

    def testDoubleBad2(self):
        with self.assertRaises(TypeError):
            java.lang.Double(tuple(), tuple())

    def compareTest(self, u, v):
        self.assertEqual(u, v)
        self.assertNotEqual(u, v - 1)
        self.assertTrue(u > v - 1)
        self.assertFalse(u > v + 1)
        self.assertTrue(u >= v)
        self.assertTrue(u <= v)
        self.assertFalse(u < v)
        self.assertFalse(u > v)
        self.assertTrue(u < v + 1)
        self.assertTrue(u > v - 1)

    def testByteBoxOps(self):
        u = JObject(81, JByte)
        self.assertIsInstance(u, jpype.java.lang.Byte)
        self.compareTest(u, 81)

    def testCharBoxOps(self):
        u = JObject('Q', JChar)
        self.assertIsInstance(u, jpype.java.lang.Character)
        self.compareTest(u, 81)

    def testShortBoxOps(self):
        u = JObject(81, JShort)
        self.assertIsInstance(u, jpype.java.lang.Short)
        self.compareTest(u, 81)

    def testIntBoxOps(self):
        u = JObject(81, JInt)
        self.assertIsInstance(u, jpype.java.lang.Integer)
        self.compareTest(u, 81)

    def testLongBoxOps(self):
        u = JObject(81, JLong)
        self.assertIsInstance(u, jpype.java.lang.Long)
        self.compareTest(u, 81)

    def testIntBoxOps(self):
        u = JObject(81, JFloat)
        self.assertIsInstance(u, jpype.java.lang.Float)
        self.compareTest(u, 81)

    def testLongBoxOps(self):
        u = JObject(81, JDouble)
        self.assertIsInstance(u, jpype.java.lang.Double)
        self.compareTest(u, 81)

    def testCharBox(self):
        u = passThrough(JChar('Q'))
        self.assertIsInstance(u, jpype.java.lang.Character)
        self.assertEqual(u, jpype.java.lang.Character('Q'))

    def testBooleanBox(self):
        u = passThrough(JBoolean(True))
        self.assertIsInstance(u, jpype.java.lang.Boolean)
        self.assertEqual(u, jpype.java.lang.Boolean(True))
        self.assertEqual(u, True)
        u = passThrough(JBoolean(False))
        self.assertIsInstance(u, jpype.java.lang.Boolean)
        self.assertEqual(u, jpype.java.lang.Boolean(False))
        self.assertEqual(u, False)

    def testByteBox(self):
        u = passThrough(JByte(5))
        self.assertIsInstance(u, java.lang.Byte)
        self.assertEqual(u, java.lang.Byte(5))

    def testShortBox(self):
        u = passThrough(JShort(5))
        self.assertIsInstance(u, java.lang.Short)
        self.assertEqual(u, java.lang.Short(5))

    def testIntBox(self):
        u = passThrough(JInt(5))
        self.assertIsInstance(u, java.lang.Integer)
        self.assertEqual(u, java.lang.Integer(5))

    def testLongBox(self):
        u = passThrough(JLong(5))
        self.assertIsInstance(u, java.lang.Long)
        self.assertEqual(u, java.lang.Long(5))

    def testFloatBox(self):
        u = passThrough(JFloat(5))
        self.assertIsInstance(u, java.lang.Float)
        self.assertEqual(u, java.lang.Float(5))

    def testDoubleBox(self):
        u = passThrough(JDouble(5))
        self.assertIsInstance(u, java.lang.Double)
        self.assertEqual(u, java.lang.Double(5))

    def testBooleanNull(self):
        n = JObject(None, JBoolean)
        self.assertIsInstance(n, java.lang.Boolean)
        self.assertEqual(n, None)
        self.assertNotEqual(n, True)
        self.assertNotEqual(n, False)
        with self.assertRaises(TypeError):
            int(n)
        with self.assertRaises(TypeError):
            float(n)
        self.assertEqual(str(n), str(None))
        self.assertEqual(repr(n), str(None))
        self.assertEqual(hash(n), hash(None))
        u = passThrough(n)
        self.assertEqual(u, None)

    def testCharNull(self):
        n = JObject(None, JChar)
        self.assertIsInstance(n, java.lang.Character)
        self.assertNotEqual(n, 0)
        with self.assertRaises(TypeError):
            int(n)
        with self.assertRaises(TypeError):
            float(n)
        self.assertEqual(str(n), str(None))
        self.assertEqual(repr(n), str(None))
        self.assertEqual(hash(n), hash(None))
        u = passThrough(n)
        self.assertEqual(u, None)

    def testByteNull(self):
        n = JObject(None, JByte)
        self.assertIsInstance(n, java.lang.Byte)
        self.assertNotEqual(n, 0)
        with self.assertRaises(TypeError):
            int(n)
        with self.assertRaises(TypeError):
            float(n)
        self.assertEqual(str(n), str(None))
        self.assertEqual(repr(n), str(None))
        self.assertEqual(hash(n), hash(None))
        u = passThrough(n)
        self.assertEqual(u, None)

    def testShortNull(self):
        n = JObject(None, JShort)
        self.assertIsInstance(n, java.lang.Short)
        self.assertNotEqual(n, 0)
        with self.assertRaises(TypeError):
            int(n)
        with self.assertRaises(TypeError):
            float(n)
        self.assertEqual(str(n), str(None))
        self.assertEqual(repr(n), str(None))
        self.assertEqual(hash(n), hash(None))
        u = passThrough(n)
        self.assertEqual(u, None)

    def testIntNull(self):
        n = JObject(None, JInt)
        self.assertIsInstance(n, java.lang.Integer)
        self.assertNotEqual(n, 0)
        with self.assertRaises(TypeError):
            int(n)
        with self.assertRaises(TypeError):
            float(n)
        self.assertEqual(str(n), str(None))
        self.assertEqual(repr(n), str(None))
        self.assertEqual(hash(n), hash(None))
        u = passThrough(n)
        self.assertEqual(u, None)

    def testLongNull(self):
        n = JObject(None, JLong)
        self.assertIsInstance(n, java.lang.Long)
        self.assertNotEqual(n, 0)
        with self.assertRaises(TypeError):
            int(n)
        with self.assertRaises(TypeError):
            float(n)
        self.assertEqual(str(n), str(None))
        self.assertEqual(repr(n), str(None))
        self.assertEqual(hash(n), hash(None))
        u = passThrough(n)
        self.assertEqual(u, None)

    def testFloatNull(self):
        n = JObject(None, JFloat)
        self.assertIsInstance(n, java.lang.Float)
        self.assertNotEqual(n, 0)
        self.assertNotEqual(n, 0.0)
        with self.assertRaises(TypeError):
            int(n)
        with self.assertRaises(TypeError):
            float(n)
        self.assertEqual(str(n), str(None))
        self.assertEqual(repr(n), str(None))
        self.assertEqual(hash(n), hash(None))
        u = passThrough(n)
        self.assertEqual(u, None)

    def testDoubleNull(self):
        n = JObject(None, JDouble)
        self.assertIsInstance(n, java.lang.Double)
        self.assertNotEqual(n, 0)
        self.assertNotEqual(n, 0.0)
        with self.assertRaises(TypeError):
            int(n)
        with self.assertRaises(TypeError):
            float(n)
        self.assertEqual(str(n), str(None))
        self.assertEqual(repr(n), str(None))
        self.assertEqual(hash(n), hash(None))
        u = passThrough(n)
        self.assertEqual(u, None)

    def testAsNumber(self):
        self.assertIsInstance(java.lang.Byte(1), java.lang.Number)
        self.assertIsInstance(java.lang.Short(1), java.lang.Number)
        self.assertIsInstance(java.lang.Integer(1), java.lang.Number)
        self.assertIsInstance(java.lang.Long(1), java.lang.Number)
        self.assertIsInstance(java.lang.Float(1), java.lang.Number)
        self.assertIsInstance(java.lang.Double(1), java.lang.Number)
