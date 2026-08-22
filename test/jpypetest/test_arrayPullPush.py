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
Tests for JArray.pullTo()/JArray.pushFrom() -- bulk-copy a Java primitive
array's elements out to a caller-supplied writable buffer, and the mirror
operation, bulk-copying a caller-supplied readable buffer's elements into
an existing Java primitive array in place. pushFrom and the naming
(pullTo/pushFrom, matching the J2NI View.pull/push precedent) were added
alongside the byte-order/float16 bulk fast path.
"""

import sys
import unittest

import jpype
from jpype import JArray, JInt, JLong, JDouble, JString, JByte, JShort, JBoolean, JChar
import common

try:
    import numpy as np
    has_numpy = True
except ImportError:
    has_numpy = False


class ArrayPullToTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not has_numpy:
            self.skipTest("NumPy not available")

    def testContiguousFastPath(self):
        values = list(range(100))
        ja = JArray(JInt)(values)
        dest = np.empty(100, dtype=np.int32)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, values)

    def testDoubleContiguousFastPath(self):
        values = [i * 1.5 for i in range(50)]
        ja = JArray(JDouble)(values)
        dest = np.empty(50, dtype=np.float64)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, values)

    def testDestShapeNeedNotMatch(self):
        # Same total element count, different shape -- pullTo only
        # requires the flat element count and item size to line up.
        ja = JArray(JInt)(list(range(12)))
        dest = np.empty((3, 4), dtype=np.int32)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest.flatten(), list(range(12)))

    def testNonContiguousDest(self):
        # A strided (non-contiguous) destination forces the general
        # (GetPrimitiveArrayCritical + stride-walk) path instead of the
        # single-Get<Type>ArrayRegion fast path.
        values = list(range(20))
        ja = JArray(JInt)(values)
        backing = np.zeros(40, dtype=np.int32)
        dest = backing[::2]
        self.assertFalse(dest.flags['C_CONTIGUOUS'])
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, values)
        # Untouched interleaved elements stay zero.
        np.testing.assert_array_equal(backing[1::2], np.zeros(20))

    def testSteppedSource(self):
        # A sliced (stepped) Java array as the source also forces the
        # general path (m_Step != 1).
        values = list(range(20))
        ja = JArray(JInt)(values)
        sliced = ja[::2]
        dest = np.empty(10, dtype=np.int32)
        sliced.pullTo(dest)
        np.testing.assert_array_equal(dest, values[::2])

    def testSizeMismatchRaises(self):
        ja = JArray(JInt)(list(range(10)))
        dest = np.empty(5, dtype=np.int32)
        with self.assertRaises(ValueError):
            ja.pullTo(dest)

    def testItemSizeMismatchRaises(self):
        ja = JArray(JInt)(list(range(10)))
        dest = np.empty(10, dtype=np.float64)
        with self.assertRaises(TypeError):
            ja.pullTo(dest)

    def testNonPrimitiveArrayRaises(self):
        ja = JArray(JString)(["a", "b", "c"])
        dest = np.empty(3, dtype=np.int32)
        with self.assertRaises(TypeError):
            ja.pullTo(dest)

    def testEmptyArray(self):
        ja = JArray(JInt)([])
        dest = np.empty(0, dtype=np.int32)
        ja.pullTo(dest)

    def testReadOnlyDestRaises(self):
        ja = JArray(JInt)(list(range(10)))
        dest = np.empty(10, dtype=np.int32)
        dest.flags.writeable = False
        with self.assertRaises((TypeError, ValueError)):
            ja.pullTo(dest)

    def testOtherTypes(self):
        ivals = list(range(-5, 15))
        jai = JArray(JInt)(ivals)
        desti = np.empty(len(ivals), dtype=np.int32)
        jai.pullTo(desti)
        self.assertEqual(list(desti), ivals)

        bvals = [True, False, True, True, False]
        jab = JArray(JBoolean)(bvals)
        destb = np.empty(len(bvals), dtype=np.bool_)
        jab.pullTo(destb)
        self.assertEqual(list(destb), bvals)

        byvals = [1, 2, 3, -1, -128, 127]
        jaby = JArray(JByte)(byvals)
        destby = np.empty(len(byvals), dtype=np.int8)
        jaby.pullTo(destby)
        self.assertEqual(list(destby), byvals)


class ArrayPullToMultiDimTestCase(common.JPypeTestCase):
    """N-D pullTo (int[][], ..., int[][][][][]) -- see jp_array.cpp's
    pullToRectangular. Depths 2-4 exercise the direct
    Support.collectRectangular path; depth 5 exercises the recursive
    peel-the-outer-dimension extension one level past
    collectRectangular's own 4-dim JNI-call cap."""

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not has_numpy:
            self.skipTest("NumPy not available")

    def _makeJavaArray(self, shape):
        DeepBench = jpype.JClass('jpype.benchmark.DeepBench')
        maker = {
            2: DeepBench.make2DIntArray, 3: DeepBench.make3DIntArray,
            4: DeepBench.make4DIntArray, 5: DeepBench.make5DIntArray,
        }[len(shape)]
        return maker(shape[0])

    def testPull2D(self):
        ja = self._makeJavaArray((4, 4))
        expected = np.asarray(ja)
        dest = np.empty((4, 4), dtype=np.int32)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, expected)

    def testPull3D(self):
        ja = self._makeJavaArray((4, 4, 4))
        expected = np.asarray(ja)
        dest = np.empty((4, 4, 4), dtype=np.int32)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, expected)

    def testPull4D(self):
        ja = self._makeJavaArray((3, 3, 3, 3))
        expected = np.asarray(ja)
        dest = np.empty((3, 3, 3, 3), dtype=np.int32)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, expected)

    def testPull5DBeyondCollectRectangularCap(self):
        ja = self._makeJavaArray((3, 3, 3, 3, 3))
        expected = np.asarray(ja)
        dest = np.empty((3, 3, 3, 3, 3), dtype=np.int32)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, expected)

    def testPullNonContiguousDest2D(self):
        ja = self._makeJavaArray((4, 4))
        expected = np.asarray(ja)
        backing = np.zeros((4, 8), dtype=np.int32)
        dest = backing[:, ::2]
        self.assertFalse(dest.flags['C_CONTIGUOUS'])
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, expected)
        np.testing.assert_array_equal(backing[:, 1::2], np.zeros((4, 4)))

    def testPullNonContiguousDest5D(self):
        ja = self._makeJavaArray((2, 2, 2, 2, 2))
        expected = np.asarray(ja)
        backing = np.zeros((2, 2, 2, 2, 4), dtype=np.int32)
        dest = backing[..., ::2]
        self.assertFalse(dest.flags['C_CONTIGUOUS'])
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, expected)

    def testPullShapeMismatchRaises(self):
        ja = self._makeJavaArray((4, 4))
        dest = np.empty((3, 4), dtype=np.int32)
        with self.assertRaises(ValueError):
            ja.pullTo(dest)

    def testPullNdimMismatchRaises(self):
        ja = self._makeJavaArray((4, 4))
        dest = np.empty((4, 4, 1), dtype=np.int32)
        with self.assertRaises(ValueError):
            ja.pullTo(dest)

    def testPullRaggedRaises(self):
        JIntArray = JArray(JInt)
        JIntArray2D = JArray(JIntArray)
        ragged = JIntArray2D([JIntArray([1, 2, 3]), JIntArray([4, 5])])
        dest = np.empty((2, 3), dtype=np.int32)
        with self.assertRaises(TypeError):
            ragged.pullTo(dest)

    def testPullOuterShapeMismatchRaisesDepth5(self):
        # Same ndim (so pullTo's own view.ndim != depth check upstream
        # doesn't catch it), but the depth>4 recursive branch's own
        # n != view.shape[0] check (pullToRectangular, jp_array.cpp) --
        # unlike testPullShapeMismatchRaises above, which only exercises
        # this same kind of check at depth<=4 (validateRectangularShape).
        ja = self._makeJavaArray((3, 3, 3, 3, 3))
        dest = np.empty((2, 3, 3, 3, 3), dtype=np.int32)
        with self.assertRaises(ValueError):
            ja.pullTo(dest)

    def testPullItemSizeMismatchRaisesMultiDim(self):
        ja = self._makeJavaArray((4, 4))
        dest = np.empty((4, 4), dtype=np.int64)
        with self.assertRaises(TypeError):
            ja.pullTo(dest)

    @unittest.skipUnless(sys.version_info >= (3, 12),
            "PEP 688 __buffer__ needed to force a buffer export that "
            "declines PyBUF_STRIDES|PyBUF_FORMAT -- see "
            "test_arrayMultiDimBuffer.py's own copy of this technique "
            "for the full rationale.")
    def testPullBufferInvalidRaisesMultiDim(self):
        class NoStrides:
            def __buffer__(self, flags):
                raise BufferError("declines strides on purpose")

            def __release_buffer__(self, view):
                pass

        ja = self._makeJavaArray((4, 4))
        with self.assertRaises(BufferError):
            ja.pullTo(NoStrides())


class ArrayPushFromTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not has_numpy:
            self.skipTest("NumPy not available")

    def testContiguousFastPath(self):
        # Matching dtype, native byte order -- RAW_NATIVE, single
        # SetIntArrayRegion call.
        ja = JArray(JInt)(10)
        src = np.arange(10, dtype=np.int32)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testDoubleContiguousFastPath(self):
        ja = JArray(JDouble)(50)
        src = np.arange(50, dtype=np.float64) * 1.5
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testFloatContiguousFastPath(self):
        # JPFloatType::setElements -- only JInt/JDouble had a pushFrom
        # RAW_NATIVE-fast-path test; float never did.
        ja = JArray(jpype.JFloat)(20)
        src = (np.arange(20, dtype=np.float32) - 10) * 1.5
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testByteContiguousFastPath(self):
        # JPByteType::setElements -- only JInt/JDouble had a pushFrom
        # RAW_NATIVE-fast-path test; byte/short/boolean/char never did.
        ja = JArray(JByte)(10)
        src = np.arange(10, dtype=np.int8)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testShortContiguousFastPath(self):
        ja = JArray(JShort)(10)
        src = np.arange(10, dtype=np.int16)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testLongContiguousFastPath(self):
        # JPLongType::setElements -- testUint64ItemSize8Aliasing below
        # exercises the itemsize==8 'L'->'Q' aliasing (RAW_SWAPPED), but
        # nothing previously used a genuinely matching int64 source, so
        # RAW_NATIVE never reached setElements for JLong.
        ja = JArray(JLong)(10)
        src = np.arange(10, dtype=np.int64)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testBooleanContiguousFastPath(self):
        ja = JArray(JBoolean)(4)
        src = np.array([True, False, True, True], dtype=np.bool_)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testCharContiguousFastPath(self):
        ja = JArray(JChar)(5)
        src = np.arange(65, 70, dtype=np.uint16)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testSrcShapeNeedNotMatch(self):
        ja = JArray(JInt)(12)
        src = np.arange(12, dtype=np.int32).reshape(3, 4)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src.flatten())

    def testDtypeMismatchFallsBackAndConverts(self):
        # float64 -> int (truncating) is genuine dtype coercion: RAW_NONE,
        # must go through the general per-element converter path.
        ja = JArray(JInt)(8)
        src = np.arange(8, dtype=np.float64) + 0.9
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src.astype(np.int32))

    def testUint64ItemSize8Aliasing(self):
        # itemsize==8 with buffer format 'L' (numpy's uint64 code on a
        # platform where `unsigned long` is 8 bytes) exercises
        # classifyRawTransfer's 'L'->'Q' aliasing, matching getConverter's
        # own itemsize==8 'l'/'L' -> 'q'/'Q' aliasing.
        ja = JArray(JLong)(10)
        src = np.arange(10, dtype=np.uint64)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src.astype(np.int64))

    def testByteSwappedMatchingDtype(self):
        # Same numeric kind/width as the target, but non-native byte
        # order -- RAW_SWAPPED.
        native = np.arange(16, dtype=np.int32) - 5
        swapped = native.astype(native.dtype.newbyteorder())
        self.assertNotEqual(swapped.dtype.byteorder, '=')
        ja = JArray(JInt)(16)
        ja.pushFrom(swapped)
        np.testing.assert_array_equal(np.asarray(ja), native)

    def testFloat16ToFloat(self):
        ja = JArray(jpype.JFloat)(20)
        src = (np.arange(20, dtype=np.float32) - 10).astype(np.float16)
        ja.pushFrom(src)
        np.testing.assert_allclose(np.asarray(ja), src.astype(np.float32))

    def testFloat16ToDouble(self):
        ja = JArray(JDouble)(20)
        src = (np.arange(20, dtype=np.float32) - 10).astype(np.float16)
        ja.pushFrom(src)
        np.testing.assert_allclose(np.asarray(ja), src.astype(np.float64))

    def testFloat16ToInt(self):
        ja = JArray(JInt)(10)
        src = np.arange(10, dtype=np.float16) * 3
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src.astype(np.int32))

    def testNonContiguousSrc(self):
        # A strided (non-contiguous) source forces the general
        # (GetPrimitiveArrayCritical + stride-walk) path.
        backing = np.arange(40, dtype=np.int32)
        src = backing[::2]
        self.assertFalse(src.flags['C_CONTIGUOUS'])
        ja = JArray(JInt)(20)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testSteppedDest(self):
        # A sliced (stepped) Java array as the destination forces the
        # general path (m_Step != 1).
        ja = JArray(JInt)(list(range(20)))
        sliced = ja[::2]
        src = np.arange(10, dtype=np.int32) * 100
        sliced.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja)[::2], src)
        np.testing.assert_array_equal(np.asarray(ja)[1::2], np.arange(1, 20, 2))

    def testSizeMismatchRaises(self):
        ja = JArray(JInt)(10)
        src = np.arange(5, dtype=np.int32)
        with self.assertRaises(ValueError):
            ja.pushFrom(src)

    def testNonPrimitiveArrayRaises(self):
        ja = JArray(JString)(3)
        src = np.arange(3, dtype=np.int32)
        with self.assertRaises(TypeError):
            ja.pushFrom(src)

    def testEmptyArray(self):
        ja = JArray(JInt)([])
        src = np.empty(0, dtype=np.int32)
        ja.pushFrom(src)

    @unittest.skipUnless(sys.version_info >= (3, 12),
            "PEP 688 __buffer__ needed to force a buffer export that "
            "declines PyBUF_STRIDES|PyBUF_FORMAT -- see "
            "test_arrayMultiDimBuffer.py's own copy of this technique "
            "for the full rationale.")
    def testBufferInvalidRaises(self):
        # Flat (1D) JPArray::pushFrom's own buffer.valid() check -- the
        # ArrayPushFromMultiDimTestCase copy of this test exercises the
        # separate multi-dim buffer.valid() check instead.
        class NoStrides:
            def __buffer__(self, flags):
                raise BufferError("declines strides on purpose")

            def __release_buffer__(self, view):
                pass

        ja = JArray(JInt)(3)
        with self.assertRaises(BufferError):
            ja.pushFrom(NoStrides())

    def testLargeParallelPath(self):
        # Crosses Support.PARALLEL_THRESHOLD_ELEMENTS were it to apply --
        # it doesn't for the flat push/pull path (no per-row concept), but
        # this exercises the fast contiguous path at scale regardless.
        n = 2_000_000
        ja = JArray(JInt)(n)
        src = np.arange(n, dtype=np.int32)
        ja.pushFrom(src)
        dest = np.empty(n, dtype=np.int32)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, src)


class ArrayPushFromMultiDimTestCase(common.JPypeTestCase):
    """N-D pushFrom (int[][], ..., int[][][][][]) -- see jp_array.cpp's
    pushFromRectangular, the push-direction mirror of pullToRectangular
    (see ArrayPullToMultiDimTestCase). Same depth coverage rationale:
    depths 2-4 exercise the direct Support.collectRectangular +
    fillFromBufferIntoRectangular path, depth 5 exercises the recursive
    peel-the-outer-dimension extension one level past
    collectRectangular's own 4-dim JNI-call cap."""

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not has_numpy:
            self.skipTest("NumPy not available")

    def _makeJavaArray(self, shape):
        DeepBench = jpype.JClass('jpype.benchmark.DeepBench')
        maker = {
            2: DeepBench.make2DIntArray, 3: DeepBench.make3DIntArray,
            4: DeepBench.make4DIntArray, 5: DeepBench.make5DIntArray,
        }[len(shape)]
        return maker(shape[0])

    def testPush2D(self):
        ja = self._makeJavaArray((4, 4))
        src = np.arange(16, dtype=np.int32).reshape(4, 4)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testPush3D(self):
        ja = self._makeJavaArray((4, 4, 4))
        src = np.arange(64, dtype=np.int32).reshape(4, 4, 4)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testPush4D(self):
        ja = self._makeJavaArray((3, 3, 3, 3))
        src = np.arange(81, dtype=np.int32).reshape(3, 3, 3, 3)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testPush5DBeyondCollectRectangularCap(self):
        ja = self._makeJavaArray((3, 3, 3, 3, 3))
        src = np.arange(3 ** 5, dtype=np.int32).reshape((3,) * 5)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testPushNonContiguousSrc2D(self):
        ja = self._makeJavaArray((4, 4))
        backing = np.arange(32, dtype=np.int32).reshape(4, 8)
        src = backing[:, ::2]
        self.assertFalse(src.flags['C_CONTIGUOUS'])
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testPushNonContiguousSrc5D(self):
        ja = self._makeJavaArray((2, 2, 2, 2, 2))
        backing = np.arange(2 * 2 * 2 * 2 * 4, dtype=np.int32).reshape(2, 2, 2, 2, 4)
        src = backing[..., ::2]
        self.assertFalse(src.flags['C_CONTIGUOUS'])
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testPushPreservesArrayIdentity(self):
        # In-place contract: pushFrom must never allocate a new array --
        # each leaf row must still be the exact same Java array object
        # after the call, just with different contents.
        System = jpype.JClass('java.lang.System')
        ja = self._makeJavaArray((4, 4))
        identity_before = [System.identityHashCode(ja[i]) for i in range(4)]
        src = np.arange(16, dtype=np.int32).reshape(4, 4)
        ja.pushFrom(src)
        identity_after = [System.identityHashCode(ja[i]) for i in range(4)]
        self.assertEqual(identity_before, identity_after)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testPushShapeMismatchRaises(self):
        ja = self._makeJavaArray((4, 4))
        src = np.arange(12, dtype=np.int32).reshape(3, 4)
        with self.assertRaises(ValueError):
            ja.pushFrom(src)

    def testPushNdimMismatchRaises(self):
        ja = self._makeJavaArray((4, 4))
        src = np.arange(16, dtype=np.int32).reshape(4, 4, 1)
        with self.assertRaises(ValueError):
            ja.pushFrom(src)

    def testPushRaggedRaises(self):
        JIntArray = JArray(JInt)
        JIntArray2D = JArray(JIntArray)
        ragged = JIntArray2D([JIntArray([1, 2, 3]), JIntArray([4, 5])])
        src = np.arange(5, dtype=np.int32).reshape(1, 5)
        with self.assertRaises(TypeError):
            ragged.pushFrom(src)

    def testPushOuterShapeMismatchRaisesDepth5(self):
        # Push-direction counterpart of
        # testPullOuterShapeMismatchRaisesDepth5 -- pushFromRectangular's
        # own depth>4 recursive n != view.shape[0] check.
        ja = self._makeJavaArray((3, 3, 3, 3, 3))
        src = np.zeros((2, 3, 3, 3, 3), dtype=np.int32)
        with self.assertRaises(ValueError):
            ja.pushFrom(src)

    def testPushItemSizeMismatchRaisesMultiDim(self):
        ja = self._makeJavaArray((4, 4))
        src = np.zeros((4, 4), dtype=np.int64)
        with self.assertRaises(TypeError):
            ja.pushFrom(src)

    @unittest.skipUnless(sys.version_info >= (3, 12),
            "PEP 688 __buffer__ needed to force a buffer export that "
            "declines PyBUF_STRIDES|PyBUF_FORMAT -- see "
            "test_arrayMultiDimBuffer.py's own copy of this technique "
            "for the full rationale.")
    def testPushBufferInvalidRaisesMultiDim(self):
        class NoStrides:
            def __buffer__(self, flags):
                raise BufferError("declines strides on purpose")

            def __release_buffer__(self, view):
                pass

        ja = self._makeJavaArray((4, 4))
        with self.assertRaises(BufferError):
            ja.pushFrom(NoStrides())

    def testPushPullRoundTrip5D(self):
        ja = self._makeJavaArray((3, 3, 3, 3, 3))
        src = np.arange(3 ** 5, dtype=np.int32).reshape((3,) * 5)
        ja.pushFrom(src)
        dest = np.empty((3,) * 5, dtype=np.int32)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, src)

    def testBufferExportSnapshotSemantics(self):
        # An N-D array's buffer export (memoryview()/np.asarray()) is a
        # one-time read-only snapshot -- Java's array-of-arrays layout
        # isn't contiguous, so there's no way to hand out a true live
        # view. This is required buffer-protocol behavior (an exported
        # buffer's memory must stay valid for the life of that export),
        # not a caching bug: a *held-open* export stays frozen after a
        # later pushFrom, but releasing it and re-exporting always gives
        # fresh data.
        ja = self._makeJavaArray((4, 4))

        mv = memoryview(ja)
        snapshot_while_held = np.asarray(mv).copy()
        src = np.arange(16, dtype=np.int32).reshape(4, 4)
        ja.pushFrom(src)
        # Reading through the *same, still-open* export is frozen.
        np.testing.assert_array_equal(np.asarray(mv), snapshot_while_held)
        del mv

        # A fresh export, taken after the old one was released, reflects
        # the pushFrom.
        np.testing.assert_array_equal(np.asarray(ja), src)
