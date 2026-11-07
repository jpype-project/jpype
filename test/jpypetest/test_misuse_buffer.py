# *****************************************************************************
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0
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
""" Buffers of unusual shape convert correctly or raise, and never crash. """
import ctypes
import jpype
from jpype.types import *
import common

try:
    import numpy as np
except ImportError:  # pragma: no cover
    np = None  # type: ignore[assignment]


def _empty(ctype, *shape):
    """ A zero-length buffer of the given shape, built without numpy.

    ctypes arrays export their nested shape, and unlike memoryview.cast they
    may contain a zero-length axis.
    """
    for n in reversed(shape):
        ctype = ctype * n
    return ctype()


class ZeroDimensionalTestCase(common.JPypeTestCase):

    def testMemoryview(self):
        view = memoryview(b"\x05\x00\x00\x00").cast("i", shape=[])
        self.assertEqual(view.ndim, 0)
        with self.assertRaises(TypeError):
            JArray.of(view)

    @common.unittest.skipIf(np is None, "numpy required")
    def testNumpy(self):
        with self.assertRaises(TypeError):
            JArray.of(np.array(5))
        with self.assertRaises(TypeError):
            JArray.of(np.array(5.0))


class EmptyShapeTestCase(common.JPypeTestCase):
    """ A zero-length axis converts to an empty array of the right shape. """

    def testEmptyRows(self):
        arr = JArray.of(_empty(ctypes.c_int, 0, 3), JInt)
        self.assertEqual(len(arr), 0)
        self.assertEqual(str(arr.getClass().getName()), "[[I")

    def testEmptyMiddleAxis(self):
        arr = JArray.of(_empty(ctypes.c_int, 2, 0, 3), JInt)
        self.assertEqual(len(arr), 2)
        self.assertEqual(len(arr[0]), 0)
        self.assertEqual(len(arr[1]), 0)
        self.assertEqual(str(arr.getClass().getName()), "[[[I")

    def testEmptyColumns(self):
        arr = JArray.of(_empty(ctypes.c_double, 3, 0), JDouble)
        self.assertEqual(len(arr), 3)
        self.assertEqual(len(arr[0]), 0)
        self.assertEqual(str(arr.getClass().getName()), "[[D")

    def testNonEmptyUnaffected(self):
        arr = JArray.of(memoryview(bytes(range(24))).cast("i", shape=[2, 3]))
        expected = memoryview(bytes(range(24))).cast("i", shape=[2, 3]).tolist()
        self.assertEqual([list(r) for r in arr], expected)

    @common.unittest.skipIf(np is None, "numpy required")
    def testNumpy(self):
        self.assertEqual(len(JArray.of(np.zeros((0, 3), dtype="i4"))), 0)
        arr = JArray.of(np.zeros((2, 0, 3), dtype="f8"))
        self.assertEqual(len(arr), 2)
        self.assertEqual(len(arr[0]), 0)
