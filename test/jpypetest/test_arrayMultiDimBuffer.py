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
Correctness tests for pushing/pulling a numpy (buffer-protocol) array
into/out of a multi-dimensional primitive Java array: matching-dtype
bulk transfer, the dtype-mismatch fallback (a real per-element
conversion, e.g. float64 -> int32), non-contiguous sources (transposed
or strided views, which must not silently reinterpret the wrong bytes),
byte-swapped/float16 sources, and large arrays that cross the internal
parallel-vs-serial threshold on both sides. test_buffer.py already
covers per-type pull (np.asarray) and construction-time push at 1D-3D
for every primitive type; this file targets what's not already
exercised there: argument-conversion push (a declared array-typed
*method parameter*, not the JArray(...) constructor path) at depths up
to 5, and the cases above.

ArrayManualCtorBufferTestCase below is the one exception to that split:
it targets the manual JArray(JType, dims)(buffer)/JType[:, :, ...]
(buffer) *constructor* path specifically, at N-D depths. It lives here
rather than in test_array.py because it exercises the same buffer-fast-
path machinery (tryFastMultiArrayBuffer) as the argument-conversion
tests above, just through PyJPArray_init instead of PyJPModule_convertBuffer
-- the two are more naturally grouped by "buffer fast path" than by
"argument vs. constructor".
"""

import sys
import unittest

import jpype
from jpype import JArray, JInt, JDouble
import common

try:
    import numpy as np
    has_numpy = True
except ImportError:
    has_numpy = False


class ArrayMultiDimBufferTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not has_numpy:
            self.skipTest("NumPy not available")
        self.DeepBench = jpype.JClass('jpype.benchmark.DeepBench')

    # ---- push: matching dtype (fast path) ----

    def testPush2D(self):
        # Sum alone can't catch a transposed/misindexed reshape (same
        # total either way) -- use non-square, non-symmetric-under-
        # transpose shapes and round-trip elementwise through
        # identity*DIntArray (the argument-dispatch counterpart of
        # testPushValuesLandCorrectly's constructor-path check) rather
        # than just comparing sums.
        arr = np.arange(12, dtype=np.int32).reshape(3, 4)
        back = np.asarray(self.DeepBench.identity2DIntArray(arr))
        np.testing.assert_array_equal(back, arr)
        self.assertEqual(self.DeepBench.sum2DIntArray(arr), int(arr.sum()))

    def testPush3D(self):
        arr = np.arange(60, dtype=np.int32).reshape(3, 4, 5)
        back = np.asarray(self.DeepBench.identity3DIntArray(arr))
        np.testing.assert_array_equal(back, arr)
        self.assertEqual(self.DeepBench.sum3DIntArray(arr), int(arr.sum()))

    def testPush4D(self):
        arr = np.arange(2 * 3 * 4 * 5, dtype=np.int32).reshape(2, 3, 4, 5)
        back = np.asarray(self.DeepBench.identity4DIntArray(arr))
        np.testing.assert_array_equal(back, arr)
        self.assertEqual(self.DeepBench.sum4DIntArray(arr), int(arr.sum()))

    def testPush5D(self):
        arr = np.arange(2 * 3 * 2 * 3 * 2, dtype=np.int32).reshape(2, 3, 2, 3, 2)
        back = np.asarray(self.DeepBench.identity5DIntArray(arr))
        np.testing.assert_array_equal(back, arr)
        self.assertEqual(self.DeepBench.sum5DIntArray(arr), int(arr.sum()))

    def testPushValuesLandCorrectly(self):
        # Sum alone can't catch a transposed/misindexed reshape (same
        # total either way) -- round-trip through identityIntArray's 2D
        # sibling isn't available, so verify via a value that depends on
        # position: construct, pull back, compare elementwise.
        arr = np.arange(60, dtype=np.int32).reshape(3, 4, 5)
        ja = JArray(JInt, 3)(arr)
        back = np.asarray(ja)
        np.testing.assert_array_equal(back, arr)

    # ---- push: dtype mismatch (general converter fallback) ----

    def testPushDtypeMismatchFloat64ToInt(self):
        arr = (np.arange(16, dtype=np.float64) + 0.9).reshape(4, 4)
        expected = sum(int(x) for x in arr.flatten())
        self.assertEqual(self.DeepBench.sum2DIntArray(arr), expected)

    def testPushDtypeMismatchInt64ToInt(self):
        arr = np.arange(16, dtype=np.int64).reshape(4, 4)
        self.assertEqual(self.DeepBench.sum2DIntArray(arr), int(arr.sum()))

    def testPushDtypeMismatchInt64ToByte(self):
        # byte/boolean/char have no fixed-cost raw-reinterpret/byte-swap
        # relationship to int64 (classifyRawTransfer returns RAW_NONE), so
        # this exercises JPByteType::newMultiArrayObject -- the general
        # per-leaf-critical-section fallback tryFastMultiArrayBuffer declines
        # into -- which byte/boolean/char never reach via any other test
        # (no sum2D*Array method exists for them; identity2D*Array round-
        # trips the same conversion path instead).
        arr = np.arange(16, dtype=np.int64).reshape(4, 4) - 8
        back = np.asarray(self.DeepBench.identity2DByteArray(arr))
        np.testing.assert_array_equal(back, arr)

    def testPushDtypeMismatchInt64ToBoolean(self):
        arr = (np.arange(16, dtype=np.int64) % 2).reshape(4, 4)
        back = np.asarray(self.DeepBench.identity2DBooleanArray(arr))
        np.testing.assert_array_equal(back, arr.astype(bool))

    def testPushDtypeMismatchInt64ToChar(self):
        arr = (np.arange(16, dtype=np.int64) + ord('a')).reshape(4, 4)
        back = np.asarray(self.DeepBench.identity2DCharArray(arr))
        np.testing.assert_array_equal(back, arr.astype(np.uint16))

    # int's own JPIntType::newMultiArrayObject is exercised by
    # testPushDtypeMismatch{Float64,Int64}ToInt above, but short/long/
    # float/double never got the same treatment despite having their own
    # sum2D*Array method to exercise it through -- each of these sources is
    # chosen so classifyRawTransfer can't take a raw-reinterpret/byte-swap
    # shortcut (mismatched kind or size vs. the target), forcing the actual
    # newMultiArrayObject fallback rather than tryFastMultiArrayBuffer.

    def testPushDtypeMismatchInt64ToShort(self):
        arr = np.arange(16, dtype=np.int64).reshape(4, 4) - 8
        self.assertEqual(self.DeepBench.sum2DShortArray(arr), int(arr.sum()))

    def testPushDtypeMismatchFloat64ToLong(self):
        arr = (np.arange(16, dtype=np.float64) + 0.9).reshape(4, 4)
        expected = sum(int(x) for x in arr.flatten())
        self.assertEqual(self.DeepBench.sum2DLongArray(arr), expected)

    def testPushDtypeMismatchFloat64ToFloat(self):
        arr = (np.arange(16, dtype=np.float64) + 0.5).reshape(4, 4)
        self.assertAlmostEqual(self.DeepBench.sum2DFloatArray(arr), float(arr.sum()), places=3)

    def testPushDtypeMismatchInt64ToDouble(self):
        arr = np.arange(16, dtype=np.int64).reshape(4, 4) - 8
        self.assertEqual(self.DeepBench.sum2DDoubleArray(arr), float(arr.sum()))

    @unittest.skipUnless(sys.version_info >= (3, 12),
            "PEP 688 __buffer__ needed to force a buffer export that "
            "declines PyBUF_STRIDES -- see ArrayManualCtorBufferTestCase's "
            "own copy of this technique below for the full rationale.")
    def testMatchDeclinesWhenBufferExportFails(self):
        # JPConversionMultiArrayBuffer::matches (jp_classhints.cpp) --
        # PyObject_CheckBuffer passes but opening the buffer with
        # PyBUF_STRIDES|PyBUF_FORMAT fails: must decline the match (fall
        # through to sequenceConversion) rather than raise from matches()
        # itself.
        class NoStrides:
            def __buffer__(self, flags):
                raise BufferError("declines strides on purpose")

            def __release_buffer__(self, view):
                pass

        with self.assertRaises(TypeError):
            self.DeepBench.sum2DIntArray(NoStrides())

    def testPushNonContiguous(self):
        # A transposed view is not C-contiguous -- must not silently
        # reinterpret the wrong bytes; falls back correctly either way.
        base = np.arange(16, dtype=np.int32).reshape(4, 4)
        arr = base.T
        self.assertFalse(arr.flags['C_CONTIGUOUS'])
        self.assertEqual(self.DeepBench.sum2DIntArray(arr), int(arr.sum()))

    # ---- push: a non-contiguous source (transposed, or a strided slice)
    # is a fully valid buffer-protocol object and must push correctly,
    # via a bulk path rather than falling all the way back to a fully
    # general per-row/per-element walk. Sum alone can't catch a
    # misindexed reshape (same total either way, per
    # testPushValuesLandCorrectly's own comment above), so these
    # round-trip elementwise instead. ----

    def testPushNonContiguousValuesLandCorrectly2D(self):
        base = np.arange(16, dtype=np.int32).reshape(4, 4)
        arr = base.T
        self.assertFalse(arr.flags['C_CONTIGUOUS'])
        ja = JArray(JInt, 2)(arr)
        back = np.asarray(ja)
        np.testing.assert_array_equal(back, arr)

    def testPushNonContiguousValuesLandCorrectly3D(self):
        base = np.arange(60, dtype=np.int32).reshape(3, 4, 5)
        arr = np.transpose(base, (2, 0, 1))
        self.assertFalse(arr.flags['C_CONTIGUOUS'])
        ja = JArray(JInt, 3)(arr)
        back = np.asarray(ja)
        np.testing.assert_array_equal(back, arr)

    def testPushNonContiguousStridedSlice2D(self):
        base = np.arange(100, dtype=np.int32).reshape(10, 10)
        arr = base[::2, ::2]
        self.assertFalse(arr.flags['C_CONTIGUOUS'])
        ja = JArray(JInt, 2)(arr)
        back = np.asarray(ja)
        np.testing.assert_array_equal(back, arr)

    def testPushNonContiguous1DColumn(self):
        # 1D case, one dimension down from the ND tests above: a numpy
        # column slice has a non-unit stride, so this exercises the same
        # non-contiguous-source-must-still-match-the-buffer-path
        # requirement for a flat array argument.
        base = np.arange(20, dtype=np.int32).reshape(4, 5)
        col = base[:, 2]
        self.assertFalse(col.flags['C_CONTIGUOUS'])
        result = self.DeepBench.identityIntArray(col)
        np.testing.assert_array_equal(np.asarray(result), col)

    # ---- push: byte-swapped / float16 (RAW_SWAPPED, RAW_HALF_* bulk fast
    # paths in JPConversionMultiArrayBuffer -- classifyRawTransfer routes
    # these through Support.fillFromBuffer's mode-aware dispatch instead of
    # the general per-leaf-critical-section newMultiArrayObject fallback) ----

    def testPushByteSwapped2D(self):
        native = (np.arange(12, dtype=np.int32) - 5).reshape(3, 4)
        swapped = native.astype(native.dtype.newbyteorder())
        self.assertNotEqual(swapped.dtype.byteorder, '=')
        back = np.asarray(self.DeepBench.identity2DIntArray(swapped))
        np.testing.assert_array_equal(back, native)
        self.assertEqual(self.DeepBench.sum2DIntArray(swapped), int(native.sum()))

    def testPushByteSwapped3D(self):
        native = (np.arange(60, dtype=np.int32) - 5).reshape(3, 4, 5)
        swapped = native.astype(native.dtype.newbyteorder())
        back = np.asarray(self.DeepBench.identity3DIntArray(swapped))
        np.testing.assert_array_equal(back, native)
        self.assertEqual(self.DeepBench.sum3DIntArray(swapped), int(native.sum()))

    def testPushFloat16To2DInt(self):
        arr = (np.arange(12, dtype=np.float32) - 6).reshape(3, 4).astype(np.float16)
        expected = arr.astype(np.int32)
        back = np.asarray(self.DeepBench.identity2DIntArray(arr))
        np.testing.assert_array_equal(back, expected)
        self.assertEqual(self.DeepBench.sum2DIntArray(arr), int(expected.sum()))

    def testPushFloat16To3DInt(self):
        arr = (np.arange(60, dtype=np.float32) - 30).reshape(3, 4, 5).astype(np.float16)
        expected = arr.astype(np.int32)
        back = np.asarray(self.DeepBench.identity3DIntArray(arr))
        np.testing.assert_array_equal(back, expected)
        self.assertEqual(self.DeepBench.sum3DIntArray(arr), int(expected.sum()))

    # ---- pull: matching dtype (fast path), depths beyond test_buffer.py's ----

    def testPull2D(self):
        ja = self.DeepBench.make2DIntArray(5)
        arr = np.asarray(ja)
        # Cross-check against toList() -- an independently-tested,
        # differently-implemented element-wise conversion path (see
        # test_arrayToList.py) -- rather than a hardcoded fill formula,
        # since DeepBench.make2DIntArray's own fill (see DeepBench.java) is
        # an implementation detail free to change independently of this test.
        expected = np.array(ja.toList(), dtype=np.int32)
        np.testing.assert_array_equal(arr, expected)

    def testPull4D(self):
        ja = self.DeepBench.make4DIntArray(4)
        arr = np.asarray(ja)
        self.assertEqual(arr.shape, (4, 4, 4, 4))
        expected = np.array(ja.toList(), dtype=np.int32)
        np.testing.assert_array_equal(arr, expected)

    # ---- large scale: crosses the internal parallel-vs-serial threshold ----
    # (Support.PARALLEL_THRESHOLD_ELEMENTS == 1_000_000; these sizes are
    # comfortably past it on at least one axis so the parallel path is
    # exercised, not just serial.)

    def testPushLargeParallelPath(self):
        arr = np.arange(200 * 200 * 30, dtype=np.int32).reshape(200, 200, 30)
        self.assertEqual(self.DeepBench.sum3DIntArray(arr), int(arr.sum()))

    def testPullLargeParallelPath(self):
        n = 110  # 110**3 ~= 1.33M elements
        ja = self.DeepBench.make3DIntArray(n)
        arr = np.asarray(ja)
        # Cross-check against toList() -- see testPull2D's comment on why
        # this doesn't hardcode DeepBench.make3DIntArray's fill formula.
        expected = np.array(ja.toList(), dtype=np.int32)
        np.testing.assert_array_equal(arr, expected)

    # ---- double, to confirm the fast path isn't int-only ----

    def testPushPullDoubleRoundTrip(self):
        arr = np.random.random((6, 7)).astype(np.float64)
        ja = JArray(JDouble, 2)(arr)
        back = np.asarray(ja)
        np.testing.assert_array_equal(back, arr)


class ArrayManualCtorBufferTestCase(common.JPypeTestCase):
    """The manual JArray(JType, dims)(buffer) / JType[:,:,...](buffer)
    construction spelling -- as distinct from JArray.of(buffer), which is
    a dedicated buffer-only factory. PyJPArray_init previously never
    checked the buffer protocol at all for this path: a numpy array also
    satisfies PySequence_Check, so construction always fell into the
    generic newArray+setRange per-row path for N-D targets (no bulk-copy
    shortcut, unlike the primitive/1D case, where setArrayRange's
    primitive overrides already try the buffer fast path internally).
    See jp_array.h/PyJPArray_init's buffer-protocol fast-path block."""

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not has_numpy:
            self.skipTest("NumPy not available")

    def testDepths2Through6(self):
        for dims in range(2, 7):
            shape = (4,) * dims
            arr = np.arange(np.prod(shape), dtype=np.int32).reshape(shape)
            ja = JArray(JInt, dims)(arr)
            np.testing.assert_array_equal(np.asarray(ja), arr)

    def testClassGetitemSyntax(self):
        arr = np.arange(64, dtype=np.int32).reshape(4, 4, 4)
        ja = JInt[:, :, :](arr)
        np.testing.assert_array_equal(np.asarray(ja), arr)

    def testNonContiguousSourceFallsBack(self):
        arr = np.arange(64, dtype=np.int32).reshape(4, 4, 4)
        transposed = np.asfortranarray(arr)
        self.assertFalse(transposed.flags['C_CONTIGUOUS'])
        ja = JArray(JInt, 3)(transposed)
        np.testing.assert_array_equal(np.asarray(ja), transposed)

    def testCrossDtypeFallsBackAndConverts(self):
        arr = np.arange(64, dtype=np.int32).reshape(4, 4, 4)
        src = arr.astype(np.int64)
        ja = JArray(JInt, 3)(src)
        np.testing.assert_array_equal(np.asarray(ja), arr)

    def testFloatDtype(self):
        arr = np.random.random((3, 3, 3)).astype(np.float64)
        ja = JArray(JDouble, 3)(arr)
        np.testing.assert_array_equal(np.asarray(ja), arr)

    def testNdimMismatchFallsBack(self):
        # A source whose buffer ndim doesn't match the target array's
        # depth (a 0-d numpy scalar into a 3-D target) must decline the
        # fast path and fall through to the generic PySequence_Check
        # branch's own (unrelated) error, not crash or silently
        # misinterpret the shape.
        scalar = np.array(5, dtype=np.int32)
        self.assertEqual(scalar.ndim, 0)
        with self.assertRaises(TypeError):
            JArray(JInt, 3)(scalar)

    @unittest.skipUnless(sys.version_info >= (3, 12),
            "PEP 688 __buffer__ needed to force a buffer export that "
            "declines PyBUF_STRIDES -- no portable way to construct one "
            "on older Python, and no real-world buffer-protocol object "
            "(numpy, array.array, ctypes, memoryview) is known to ever "
            "decline it once PyObject_CheckBuffer is true.")
    def testBufferExportDeclinesStridesFallsBack(self):
        class NoStrides:
            def __buffer__(self, flags):
                raise BufferError("declines strides on purpose")

            def __release_buffer__(self, view):
                pass

        with self.assertRaises(TypeError):
            JArray(JInt, 2)(NoStrides())
