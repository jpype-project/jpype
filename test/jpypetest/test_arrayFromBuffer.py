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
Test for JArray.of() / arrayFromBuffer functionality

This tests the ability to convert numpy arrays to Java arrays using
JArray.of() (which calls arrayFromBuffer internally). Tests include
the fallback element-by-element conversion path for non-primitive types
like String arrays (bug #953).
"""

import jpype
import common
import sys

try:
    import numpy as np
    has_numpy = True
except ImportError:
    has_numpy = False


class ArrayFromBufferTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not has_numpy:
            self.skipTest("NumPy not available")

    def testStringArray1D(self):
        """Test 1D string array conversion"""
        test_string_array = np.array(['string_0', 'string_1', 'string_2'])

        # Convert to Java String array
        ja = jpype.JArray.of(test_string_array, dtype=jpype.JString)

        self.assertEqual(len(ja), 3)
        self.assertEqual(ja[0], 'string_0')
        self.assertEqual(ja[1], 'string_1')
        self.assertEqual(ja[2], 'string_2')

    def testStringArray2D(self):
        """Test 2D string array conversion"""
        test_string_matrix = np.array([
            f'string_{i}' for i in range(12)]).reshape((3, 4))

        # Convert to Java String[][] array
        ja = jpype.JArray.of(test_string_matrix, dtype=jpype.JString)

        self.assertEqual(len(ja), 3)
        self.assertEqual(len(ja[0]), 4)
        self.assertEqual(ja[0][0], 'string_0')
        self.assertEqual(ja[0][3], 'string_3')
        self.assertEqual(ja[1][0], 'string_4')
        self.assertEqual(ja[2][3], 'string_11')

    def testStringArray3D(self):
        """Test 3D string array conversion"""
        test_string_3d = np.array([
            f'str_{i}' for i in range(24)]).reshape((2, 3, 4))

        # Convert to Java String[][][] array
        ja = jpype.JArray.of(test_string_3d, dtype=jpype.JString)

        self.assertEqual(len(ja), 2)
        self.assertEqual(len(ja[0]), 3)
        self.assertEqual(len(ja[0][0]), 4)
        self.assertEqual(ja[0][0][0], 'str_0')
        self.assertEqual(ja[1][2][3], 'str_23')

    def testStringArrayWithObjectDtype(self):
        """Test string array with object dtype"""
        test_string_array = np.array(['foo', 'bar', 'baz'], dtype=object)

        # Convert to Java String array
        ja = jpype.JArray.of(test_string_array, dtype=jpype.JString)

        self.assertEqual(len(ja), 3)
        self.assertEqual(ja[0], 'foo')
        self.assertEqual(ja[1], 'bar')
        self.assertEqual(ja[2], 'baz')

    def testStringArray2DWithObjectDtype(self):
        """Test 2D string array with object dtype"""
        test_string_matrix = np.array([
            ['a', 'b', 'c'],
            ['d', 'e', 'f']
        ], dtype=object)

        # Convert to Java String[][] array
        ja = jpype.JArray.of(test_string_matrix, dtype=jpype.JString)

        self.assertEqual(len(ja), 2)
        self.assertEqual(len(ja[0]), 3)
        self.assertEqual(ja[0][0], 'a')
        self.assertEqual(ja[1][2], 'f')

    def testEmptyStringArray(self):
        """Test empty string array"""
        test_string_array = np.array([], dtype='U10')

        # Convert to Java String array
        ja = jpype.JArray.of(test_string_array, dtype=jpype.JString)

        self.assertEqual(len(ja), 0)

    def testStringArrayWithNone(self):
        """Test object array containing None values"""
        test_array = np.array(['hello', None, 'world'], dtype=object)

        # Convert to Java String array - None should become null
        ja = jpype.JArray.of(test_array, dtype=jpype.JString)

        self.assertEqual(len(ja), 3)
        self.assertEqual(ja[0], 'hello')
        self.assertIsNone(ja[1])
        self.assertEqual(ja[2], 'world')

    def testJObjectArray(self):
        """Test conversion with JObject dtype"""
        test_array = np.array(['str1', 'str2', 'str3'], dtype=object)

        # This should also work with JObject as the dtype
        ja = jpype.JArray.of(test_array, dtype=jpype.JObject)

        self.assertEqual(len(ja), 3)
        # The strings should be converted to Java Strings
        self.assertTrue(isinstance(ja[0], jpype.JString))

    def testIntArrayStillWorks(self):
        """Verify that primitive array conversion still works (regression test)"""
        test_int_array = np.array([1, 2, 3, 4, 5])

        # This should still use the fast path
        ja = jpype.JArray.of(test_int_array)

        self.assertEqual(len(ja), 5)
        self.assertEqual(ja[0], 1)
        self.assertEqual(ja[4], 5)

    def testFloatArrayStillWorks(self):
        """Verify that float array conversion still works (regression test)"""
        test_float_array = np.ones((3, 3), dtype=float)

        # This should still use the fast path
        ja = jpype.JArray.of(test_float_array)

        self.assertEqual(len(ja), 3)
        self.assertEqual(len(ja[0]), 3)
        self.assertAlmostEqual(ja[1][1], 1.0)
