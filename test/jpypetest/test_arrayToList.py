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

"""
Test for JArray.toList() -- bulk-convert a Java array into a genuine
Python list. Closes the `array->list` pull gap: primitive arrays are
read in one JNI critical section instead of one JNI call per element via
list(arr)/_JavaArrayIter.
"""

import jpype
from jpype import (JArray, JBoolean, JByte, JChar, JShort, JInt, JLong,
                    JFloat, JDouble, JString, JObject)
import common


class ArrayToListTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testEachPrimitiveType(self):
        cases = [
            (JBoolean, [True, False, True, True]),
            (JByte, [1, -1, 127, -128, 0]),
            (JShort, [1, -1, 32767, -32768, 0]),
            (JInt, [1, -1, 2**31 - 1, -2**31, 0]),
            (JLong, [1, -1, 2**62, -2**62, 0]),
            (JFloat, [1.5, -2.5, 0.0]),
            (JDouble, [1.5, -2.5, 0.0, 3.14159]),
        ]
        for jtype, values in cases:
            with self.subTest(jtype=jtype):
                ja = JArray(jtype)(values)
                out = ja.toList()
                self.assertIsInstance(out, list)
                self.assertEqual(out, list(ja))

    def testChar(self):
        ja = JArray(JChar)("hello")
        self.assertEqual(ja.toList(), list("hello"))

    def testEmptyArray(self):
        self.assertEqual(JArray(JInt)([]).toList(), [])

    def testSingleElement(self):
        self.assertEqual(JArray(JInt)([42]).toList(), [42])

    def testMatchesListConstructor(self):
        values = list(range(200))
        ja = JArray(JInt)(values)
        self.assertEqual(ja.toList(), list(ja))
        self.assertEqual(ja.toList(), values)

    def testSteppedSlice(self):
        values = list(range(20))
        ja = JArray(JInt)(values)
        self.assertEqual(ja[::2].toList(), values[::2])
        self.assertEqual(ja[::3].toList(), values[::3])
        self.assertEqual(ja[::-1].toList(), values[::-1])
        self.assertEqual(ja[5:15:2].toList(), values[5:15:2])

    def testMultiDimRectangular(self):
        rows, cols = 4, 5
        mat = JArray(JInt, 2)(rows)
        expected = []
        for r in range(rows):
            row = [r * cols + c for c in range(cols)]
            mat[r] = JArray(JInt)(row)
            expected.append(row)
        self.assertEqual(mat.toList(), expected)

    def testMultiDimJagged(self):
        jag = JArray(JInt, 2)(3)
        jag[0] = JArray(JInt)([1, 2, 3])
        jag[1] = JArray(JInt)([4])
        jag[2] = JArray(JInt)([])
        self.assertEqual(jag.toList(), [[1, 2, 3], [4], []])

    def testThreeDim(self):
        arr = JArray(JInt, 3)(2)
        arr[0] = JArray(JInt, 2)([JArray(JInt)([1, 2]), JArray(JInt)([3, 4])])
        arr[1] = JArray(JInt, 2)([JArray(JInt)([5, 6, 7])])
        self.assertEqual(arr.toList(), [[[1, 2], [3, 4]], [[5, 6, 7]]])

    def testObjectArray(self):
        strs = JArray(JString)(["a", "b", "c"])
        self.assertEqual(strs.toList(), ["a", "b", "c"])

    def testObjectArrayWithNulls(self):
        strs = JArray(JString)(3)
        strs[0] = "x"
        self.assertEqual(strs.toList(), ["x", None, None])

    def testDefaultReturnsPlainPythonTypes(self):
        for jtype, pytype in [(JBoolean, bool), (JByte, int), (JShort, int),
                               (JInt, int), (JLong, int)]:
            with self.subTest(jtype=jtype):
                ja = JArray(jtype)([1, 0, 1])
                out = ja.toList()
                self.assertTrue(all(type(x) is pytype for x in out))

        for jtype in (JFloat, JDouble):
            with self.subTest(jtype=jtype):
                ja = JArray(jtype)([1.5, 2.5])
                out = ja.toList()
                self.assertTrue(all(type(x) is float for x in out))

        ja = JArray(JChar)("hi")
        self.assertTrue(all(type(x) is str for x in ja.toList()))

    def testDtypeJDouble(self):
        ja = JArray(JInt)([1, 2, 3])
        out = ja.toList(dtype=JDouble)
        self.assertTrue(all(isinstance(x, JDouble) for x in out))
        self.assertEqual(list(out), [1.0, 2.0, 3.0])

    def testDtypeJInt(self):
        ja = JArray(JDouble)([1.5, 2.7])
        out = ja.toList(dtype=JInt)
        self.assertTrue(all(isinstance(x, JInt) for x in out))
        self.assertEqual(list(out), [1, 2])

    def testDtypeInt(self):
        ja = JArray(JDouble)([1.5, 2.7])
        out = ja.toList(dtype=int)
        self.assertTrue(all(type(x) is int for x in out))
        self.assertEqual(out, [1, 2])

    def testDtypeFloat(self):
        ja = JArray(JInt)([1, 2, 3])
        out = ja.toList(dtype=float)
        self.assertTrue(all(type(x) is float for x in out))
        self.assertEqual(out, [1.0, 2.0, 3.0])

    def testDtypeIdentityStillWrapped(self):
        # dtype matching the array's own component type still forces
        # wrapped output (a no-op cast, but boxing is still requested).
        ja = JArray(JInt)([1, 2, 3])
        out = ja.toList(dtype=JInt)
        self.assertTrue(all(isinstance(x, JInt) for x in out))

    def testDtypeWithSlices(self):
        values = list(range(20))
        ja = JArray(JInt)(values)
        out = ja[::2].toList(dtype=JDouble)
        self.assertEqual(list(out), [float(x) for x in values[::2]])

    def testDtypeMultiDim(self):
        rows, cols = 2, 3
        mat = JArray(JInt, 2)(rows)
        for r in range(rows):
            mat[r] = JArray(JInt)([r * cols + c for c in range(cols)])
        out = mat.toList(dtype=float)
        expected = [[float(r * cols + c) for c in range(cols)] for r in range(rows)]
        self.assertEqual(out, expected)

    def testDtypeRejectsBoolean(self):
        ja = JArray(JBoolean)([True, False])
        with self.assertRaises(TypeError):
            ja.toList(dtype=int)

    def testDtypeRejectsInvalidType(self):
        ja = JArray(JInt)([1, 2, 3])
        with self.assertRaises(TypeError):
            ja.toList(dtype=str)
