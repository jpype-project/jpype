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
import common
from jpype.types import *


class JChar2TestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testCharRange(self):
        self.assertEqual(ord(str(jpype.JChar(65))), 65)
        self.assertEqual(ord(str(jpype.JChar(512))), 512)

    def testCharFromStr(self):
        self.assertEqual(ord(jpype.JChar(chr(65))), 65)
        self.assertEqual(ord(jpype.JChar(chr(128))), 128)
        self.assertEqual(ord(jpype.JChar(chr(255))), 255)
        self.assertEqual(ord(jpype.JChar(chr(10255))), 10255)

    def testCharCastFloat(self):
        self.assertEqual(ord(jpype.JChar(65.0)), 65)

    @common.requireInstrumentation
    def testJPChar_new(self):
        _jpype.fault("PyJPChar_new")
        with self.assertRaisesRegex(SystemError, "fault"):
            JChar("a")
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            JChar("a")
        JChar("a")

    @common.requireInstrumentation
    def testJPChar_str(self):
        jc = JChar("a")
        _jpype.fault("PyJPChar_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jc)

    @common.requireInstrumentation
    def testJCharGetJavaConversion(self):
        _jpype.fault("JPCharType::findJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JChar._canConvertToJava(object())

    @common.requireInstrumentation
    def testJPJavaFrameCharArray(self):
        ja = JArray(JChar)(5)
        _jpype.fault("JPJavaFrame::NewCharArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JChar)(1)
        _jpype.fault("JPJavaFrame::SetCharArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 0
        _jpype.fault("JPJavaFrame::GetCharArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])
        _jpype.fault("JPJavaFrame::GetCharArrayElements")
        # Special case, only BufferError is allowed from getBuffer
        with self.assertRaises(BufferError):
            memoryview(ja[0:3])
        _jpype.fault("JPJavaFrame::ReleaseCharArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:3] = bytes([1, 2, 3])
        # Not sure why this one changed.
        _jpype.fault("JPJavaFrame::ReleaseCharArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            jpype.JObject(ja[::2], jpype.JObject)
        _jpype.fault("JPJavaFrame::ReleaseCharArrayElements")

        def f():
            # Special case no fault is allowed
            memoryview(ja[0:3])
        f()
        ja = JArray(JChar)(5)  # lgtm [py/similar-function]
        _jpype.fault("JPCharType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0, 0]

    def testFromObject(self):
        ja = JArray(JChar)(5)
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.common.Fixture")
        with self.assertRaises(TypeError):
            jf.static_char_field = object()
        with self.assertRaises(TypeError):
            jf().char_field = object()

    def testCharArrayAsString(self):
        t = JClass("jpype.array.TestArray")()
        v = t.getCharArray()
        self.assertEqual(str(v), 'avcd')

    def testArrayConversionChar(self):
        t = JClass("jpype.array.TestArray")()
        v = t.getCharArray()
        self.assertEqual(str(v[:]), 'avcd')

    def testArrayEqualsChar(self):
        contents = "abc"
        array = jpype.JArray(jpype.JChar)(contents)
        array2 = jpype.JArray(jpype.JChar)(contents)
        self.assertEqual(array, array)
        self.assertNotEqual(array, array2)
        self.assertEqual(array, "abc")

    def testArrayHash(self):
        ja = JArray(JByte)([1, 2, 3])
        self.assertIsInstance(hash(ja), int)

    def testNone(self):
        self.assertEqual(JChar._canConvertToJava(None), "none")


class JCharTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.nc = JChar('B')

    def testStr(self):
        self.assertEqual(type(str(self.nc)), str)
        self.assertEqual(str(self.nc), 'B')

    def testRepr(self):
        self.assertEqual(type(repr(self.nc)), str)
        self.assertEqual(repr(self.nc), "'B'")

    def testAbs(self):
        self.assertEqual(abs(self.nc), self.nc)

    def testOrd(self):
        self.assertEqual(ord(self.nc), 66)

    def testInt(self):
        self.assertEqual(int(self.nc), 66)

    def testFloat(self):
        self.assertEqual(float(self.nc), 66.0)

    def testLen(self):
        self.assertEqual(len(self.nc), 1)

    def testHash(self):
        self.assertEqual(hash(self.nc), hash('B'))

    def testAdd(self):
        self.assertEqual(self.nc + 1, 67)
        self.assertIsInstance(self.nc + 1, int)
        self.assertEqual(self.nc + 1.1, 67.1)
        self.assertIsInstance(self.nc + 1.1, float)

    def testSub(self):
        self.assertEqual(self.nc - 1, 65)
        self.assertIsInstance(self.nc - 1, int)
        self.assertEqual(self.nc - 1.1, 64.9)
        self.assertIsInstance(self.nc - 1.1, float)

    def testMult(self):
        self.assertEqual(self.nc * 2, 132)
        self.assertIsInstance(self.nc * 2, int)
        self.assertEqual(self.nc * 0.25, 16.5)
        self.assertIsInstance(self.nc * 2.0, float)

    def testRshift(self):
        self.assertEqual(self.nc >> 1, 33)
        self.assertIsInstance(self.nc >> 2, int)

    def testLshift(self):
        self.assertEqual(self.nc << 1, 132)
        self.assertIsInstance(self.nc << 2, int)

    def testAnd(self):
        self.assertEqual(self.nc & 244, 66 & 244)
        self.assertIsInstance(self.nc & 2, int)

    def testOr(self):
        self.assertEqual(self.nc | 40, 66 | 40)
        self.assertIsInstance(self.nc | 2, int)

    def testXor(self):
        self.assertEqual(self.nc ^ 1, 66 ^ 1)
        self.assertIsInstance(self.nc ^ 1, int)

    def testPass(self):
        fixture = jpype.JClass('jpype.common.Fixture')()
        self.assertEqual(type(fixture.callChar(self.nc)), JChar)
        self.assertEqual(type(fixture.callObject(self.nc)), jpype.java.lang.Character)

    def check(self, u, v0, v1, v2):
        self.assertEqual(v1, u)
        self.assertEqual(u, v1)
        self.assertNotEqual(v0, u)
        self.assertNotEqual(u, v0)
        self.assertNotEqual(v2, u)
        self.assertNotEqual(u, v2)
        self.assertTrue(u > v0)
        self.assertFalse(u > v2)
        self.assertTrue(u < v2)
        self.assertFalse(u < v0)
        self.assertTrue(v0 < u)
        self.assertFalse(v2 < u)
        self.assertTrue(v2 > u)
        self.assertFalse(v0 > u)
        self.assertTrue(u >= v1)
        self.assertFalse(u >= v2)
        self.assertTrue(v1 <= u)
        self.assertFalse(v2 <= u)

    def testCompareInt(self):
        self.check(self.nc, 65, 66, 67)

    def testCompareFloat(self):
        self.check(self.nc, 65.0, 66.0, 67.0)

    def testCompareJInt(self):
        self.check(self.nc, JInt(65), JInt(66), JInt(67))

    def testCompareJFloat(self):
        self.check(self.nc, JFloat(65.0), JFloat(66.0), JFloat(67.0))


class JCharBoxedTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.nc = jpype.java.lang.Character('B')

    def testStr(self):
        self.assertEqual(type(str(self.nc)), str)
        self.assertEqual(str(self.nc), 'B')

    def testRepr(self):
        self.assertEqual(type(repr(self.nc)), str)
        self.assertEqual(repr(self.nc), "'B'")

    def testOrd(self):
        self.assertEqual(ord(self.nc), 66)

    def testInt(self):
        self.assertEqual(int(self.nc), 66)

    def testFloat(self):
        self.assertEqual(float(self.nc), 66.0)

    def testLen(self):
        self.assertEqual(len(self.nc), 1)

    def testHash(self):
        self.assertEqual(hash(self.nc), hash('B'))

    def testAdd(self):
        self.assertEqual(self.nc + 1, 67)
        self.assertIsInstance(self.nc + 1, int)
        self.assertEqual(self.nc + 1.1, 67.1)
        self.assertIsInstance(self.nc + 1.1, float)

    def testSub(self):
        self.assertEqual(self.nc - 1, 65)
        self.assertIsInstance(self.nc - 1, int)
        self.assertEqual(self.nc - 1.1, 64.9)
        self.assertIsInstance(self.nc - 1.1, float)

    def testMult(self):
        self.assertEqual(self.nc * 2, 132)
        self.assertIsInstance(self.nc * 2, int)
        self.assertEqual(self.nc * 0.25, 16.5)
        self.assertIsInstance(self.nc * 2.0, float)

    def testRshift(self):
        self.assertEqual(self.nc >> 1, 33)
        self.assertIsInstance(self.nc >> 2, int)

    def testLshift(self):
        self.assertEqual(self.nc << 1, 132)
        self.assertIsInstance(self.nc << 2, int)

    def testAnd(self):
        self.assertEqual(self.nc & 244, 66 & 244)
        self.assertIsInstance(self.nc & 2, int)

    def testOr(self):
        self.assertEqual(self.nc | 40, 66 | 40)
        self.assertIsInstance(self.nc | 2, int)

    def testXor(self):
        self.assertEqual(self.nc ^ 1, 66 ^ 1)
        self.assertIsInstance(self.nc ^ 1, int)

    def testFloorDiv(self):
        self.assertEqual(self.nc // 3, 66 // 3)
        self.assertEqual(3 // self.nc, 3 // 66)

    def testDivMod(self):
        self.assertEqual(divmod(self.nc, 3), divmod(66, 3))
        self.assertEqual(divmod(3, self.nc), divmod(3, 66))

    def testInv(self):
        self.assertEqual(~self.nc, ~66)

    def testPos(self):
        self.assertEqual(+self.nc, +66)

    def testNeg(self):
        self.assertEqual(-self.nc, -66)

    def testBool(self):
        self.assertTrue(bool(self.nc))
        self.assertFalse(bool(JChar(0)))

    def testPass(self):
        fixture = jpype.JClass('jpype.common.Fixture')()
        self.assertEqual(type(fixture.callObject(self.nc)), type(self.nc))

    def check(self, u, v0, v1, v2):
        self.assertEqual(v1, u)
        self.assertEqual(u, v1)
        self.assertNotEqual(v0, u)
        self.assertNotEqual(u, v0)
        self.assertNotEqual(v2, u)
        self.assertNotEqual(u, v2)
        self.assertTrue(u > v0)
        self.assertFalse(u > v2)
        self.assertTrue(u < v2)
        self.assertFalse(u < v0)
        self.assertTrue(v0 < u)
        self.assertFalse(v2 < u)
        self.assertTrue(v2 > u)
        self.assertFalse(v0 > u)
        self.assertTrue(u >= v1)
        self.assertFalse(u >= v2)
        self.assertTrue(v1 <= u)
        self.assertFalse(v2 <= u)

    def testCompareInt(self):
        self.check(self.nc, 65, 66, 67)

    def testCompareFloat(self):
        self.check(self.nc, 65.0, 66.0, 67.0)

    def testCompareJInt(self):
        self.check(self.nc, JInt(65), JInt(66), JInt(67))

    def testCompareJFloat(self):
        self.check(self.nc, JFloat(65.0), JFloat(66.0), JFloat(67.0))


class JCharBoxedNullTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.nc = jpype.JObject(None, jpype.java.lang.Character)

    def testStr(self):
        self.assertEqual(type(str(self.nc)), str)
        self.assertEqual(str(self.nc), 'None')

    def testRepr(self):
        self.assertEqual(type(repr(self.nc)), str)
        self.assertEqual(repr(self.nc), 'None')

    def testInt(self):
        with self.assertRaises(TypeError):
            int(self.nc)

    def testFloat(self):
        with self.assertRaises(TypeError):
            float(self.nc)

    def testLen(self):
        with self.assertRaises(TypeError):
            len(self.nc)

    def testHash(self):
        self.assertEqual(hash(self.nc), hash(None))

    def testAdd(self):
        with self.assertRaises(TypeError):
            self.nc + 1
        with self.assertRaises(TypeError):
            1 + self.nc

    def testSub(self):
        with self.assertRaises(TypeError):
            self.nc - 1
        with self.assertRaises(TypeError):
            1 - self.nc

    def testMult(self):
        with self.assertRaises(TypeError):
            self.nc * 1
        with self.assertRaises(TypeError):
            1 * self.nc

    def testRshift(self):
        with self.assertRaises(TypeError):
            self.nc >> 1
        with self.assertRaises(TypeError):
            1 >> self.nc

    def testLshift(self):
        with self.assertRaises(TypeError):
            self.nc << 1
        with self.assertRaises(TypeError):
            1 << self.nc

    def testAnd(self):
        with self.assertRaises(TypeError):
            self.nc & 1
        with self.assertRaises(TypeError):
            1 & self.nc

    def testOr(self):
        with self.assertRaises(TypeError):
            self.nc | 1
        with self.assertRaises(TypeError):
            1 | self.nc

    def testXor(self):
        with self.assertRaises(TypeError):
            self.nc ^ 1
        with self.assertRaises(TypeError):
            1 ^ self.nc

    def testFloorDiv(self):
        with self.assertRaises(TypeError):
            self.nc // 1
        with self.assertRaises(TypeError):
            1 // self.nc

    def testDivMod(self):
        with self.assertRaises(TypeError):
            divmod(self.nc, 1)
        with self.assertRaises(TypeError):
            divmod(1, self.nc)

    def testInv(self):
        with self.assertRaises(TypeError):
            ~self.nc

    def testPos(self):
        with self.assertRaises(TypeError):
            +self.nc

    def testNeg(self):
        with self.assertRaises(TypeError):
            -self.nc

    def testBool(self):
        self.assertFalse(bool(self.nc))

    def testEq(self):
        self.assertTrue(self.nc == self.nc)
        self.assertFalse(self.nc != self.nc)

    def testPass(self):
        fixture = jpype.JClass('jpype.common.Fixture')()
        self.assertEqual(fixture.callObject(self.nc), None)
