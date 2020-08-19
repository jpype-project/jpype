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


class JStringTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testAddself(self):
        a = jpype.JString("abc")
        a = a + "def"
        self.assertEqual(a, "abcdef")

    def testEq(self):
        a = jpype.JString("abc")
        b = jpype.JClass("java.lang.String")("abc")
        self.assertEqual(a, b)

    def testEqPy(self):
        a = jpype.JString("abc")
        self.assertEqual(a, "abc")

    def testNotEq(self):
        a = jpype.JString("abc")
        b = jpype.JClass("java.lang.String")("def")
        self.assertNotEqual(a, b)

    def testNotEqPy(self):
        a = jpype.JString("abc")
        self.assertNotEqual(a, "def")

    def testLen(self):
        a = jpype.JString("abc")
        self.assertEqual(len(a), 3)

    def testGetItem(self):
        a = jpype.JString("abc")
        self.assertEqual(a[0], 'a')
        self.assertEqual(a[1], 'b')
        self.assertEqual(a[2], 'c')
        self.assertEqual(a[-1], 'c')
        self.assertEqual(a[-2], 'b')
        self.assertEqual(a[-3], 'a')
        with self.assertRaises(IndexError):
            x = a[3]
        with self.assertRaises(IndexError):
            x = a[-4]

    def testLt(self):
        a = jpype.JString("abc")
        b = jpype.JString("def")
        self.assertTrue(a < b)
        self.assertFalse(b < a)
        self.assertFalse(b < "def")
        self.assertTrue(a < "def")

    def testLe(self):
        a = jpype.JString("abc")
        b = jpype.JString("def")
        self.assertTrue(a <= a)
        self.assertTrue(a <= b)
        self.assertFalse(b <= a)
        self.assertTrue(b <= "def")
        self.assertTrue(a <= "def")

    def testGt(self):
        a = jpype.JString("abc")
        b = jpype.JString("def")
        self.assertFalse(a < a)
        self.assertTrue(a < b)
        self.assertFalse(b < a)
        self.assertFalse(b < "def")
        self.assertTrue(a < "def")

    def testGe(self):
        a = jpype.JString("abc")
        b = jpype.JString("def")
        self.assertTrue(a >= a)
        self.assertFalse(a >= b)
        self.assertTrue(b >= a)
        self.assertTrue(b >= "def")
        self.assertFalse(a >= "def")

    def testContains(self):
        a = jpype.JString("abc")
        self.assertTrue("ab" in a)
        self.assertFalse("cd" in a)

    def testHash(self):
        a = jpype.JString("abc")
        self.assertEqual(hash(a), hash("abc"))

    def testRepr(self):
        a = jpype.JString("abc")
        self.assertEqual(repr(a), "'abc'")

    def testConversion(self):
        self.assertEqual(jpype.JString("AAAA"), "AAAA")
        self.assertEqual(jpype.JString(bytes([65, 65, 65, 65])), "AAAA")

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

    def testNullString(self):
        self.assertEqual(str(JObject(None, JString)), "null")

    def testNullCompare(self):
        jsn = JObject(None, JString)
        self.assertFalse("a" == jsn)
        self.assertTrue("a" != jsn)
        self.assertFalse(jsn == "a")
        self.assertTrue(jsn != "a")
        self.assertTrue(jsn == jsn)
        self.assertFalse(jsn != jsn)
        with self.assertRaises(ValueError):
            jsn < "a"
        with self.assertRaises(ValueError):
            jsn <= "a"
        with self.assertRaises(ValueError):
            jsn > "a"
        with self.assertRaises(ValueError):
            jsn >= "a"
        with self.assertRaises(ValueError):
            "a" < jsn
        with self.assertRaises(ValueError):
            "a" <= jsn
        with self.assertRaises(ValueError):
            "a" > jsn
        with self.assertRaises(ValueError):
            "a" >= jsn

    def testNullAdd(self):
        jsn = JObject(None, JString)
        with self.assertRaises(ValueError):
            jsn + "a"

    def testNullHash(self):
        jsn = JObject(None, JString)
        self.assertEqual(hash(jsn), hash(None))

    def testSlice(self):
        s = 'abcdefghijklmnop'
        s2 = JString(s)
        self.assertEqual(s[:], s2[:])
        self.assertEqual(s[1:5], s2[1:5])
        self.assertEqual(s[:5], s2[:5])
        self.assertEqual(s[3:], s2[3:])
        self.assertEqual(s[::-1], s2[::-1])
