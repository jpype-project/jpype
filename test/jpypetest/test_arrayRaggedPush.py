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
Correctness tests for pushing a nested (rectangular or genuinely ragged)
Python list into a multi-dimensional primitive Java array -- what the
API guarantees the caller regardless of which internal conversion
handles it. Covers rectangular and genuinely ragged (jagged) input at
multiple depths/types (all eight primitive leaf types -- Z/B/C/S as well
as I/J/F/D -- are ragged-native eligible), the type-widening rules (bool
implicitly widens to int, an incompatible element type raises
TypeError) at both depth >= 2 and flat (1D) depth, the 4-byte length-
marker padding the narrower Z/B/C/S leaf types require and I/J/F/D
don't, and overload disambiguation.
"""

import jpype
from jpype import JArray, JInt, JLong, JFloat, JDouble, JShort, JByte, JChar, JBoolean
import common


def to_nested_list(arr):
    if hasattr(arr, '__len__') and not isinstance(arr, (bytes, str)):
        try:
            return [to_nested_list(x) for x in arr]
        except TypeError:
            return list(arr)
    return arr


class ArrayRaggedPushTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.DeepBench = jpype.JClass('jpype.benchmark.DeepBench')

    # ---- rectangular round trip, construction path ----

    def testRectangularInt2D(self):
        data = [[1, 2, 3], [4, 5, 6]]
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRectangularInt3D(self):
        data = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]
        ja = JArray(JInt, 3)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRectangularLong2D(self):
        data = [[1, 2, 3], [2 ** 40, -(2 ** 40), 0]]
        ja = JArray(JLong, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRectangularFloat2D(self):
        data = [[1.5, 2.5], [3.5, 4.5]]
        ja = JArray(JFloat, 2)(data)
        self.assertElementsAlmostEqual(
            [x for row in to_nested_list(ja) for x in row],
            [x for row in data for x in row], places=5)

    def testRectangularDouble3D(self):
        data = [[[1.1, 2.2], [3.3, 4.4]], [[5.5, 6.6], [7.7, 8.8]]]
        ja = JArray(JDouble, 3)(data)
        got = to_nested_list(ja)
        for a, b in zip(
                [x for p in got for r in p for x in r],
                [x for p in data for r in p for x in r]):
            self.assertAlmostEqual(a, b, places=9)

    # ---- rectangular round trip, method-argument-dispatch path (not the
    # JArray(...) constructor -- exercises JPArrayClass::findJavaConversionImpl
    # via a declared array-typed parameter instead) ----

    def testRectangularInt2DAsArgument(self):
        # Sum alone can't catch a transposed/misindexed push (same total
        # either way) -- non-square, position-distinguishable data,
        # round-tripped elementwise through identity2DIntArray.
        data = [[1, 2, 3], [4, 5, 6]]
        self.assertEqual(to_nested_list(self.DeepBench.identity2DIntArray(data)), data)
        self.assertEqual(self.DeepBench.sum2DIntArray(data), sum(x for row in data for x in row))

    def testRectangularInt3DAsArgument(self):
        data = [[[1, 2], [3, 4], [5, 6]], [[7, 8], [9, 10], [11, 12]]]
        self.assertEqual(to_nested_list(self.DeepBench.identity3DIntArray(data)), data)
        expected = sum(x for p in data for r in p for x in r)
        self.assertEqual(self.DeepBench.sum3DIntArray(data), expected)

    # ---- genuinely ragged (jagged) input, fully supported (not just
    # rectangular arrays with a jagged fallback) ----

    def testRaggedInt3DWorkedExample(self):
        # Sibling sub-lists at both the outer and middle level differ in
        # length, and the leaf lengths vary too -- a compact worked
        # example covering ragged branching at every level at once.
        data = [[[1, 2], [3]], [[4, 5, 6]]]
        ja = JArray(JInt, 3)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedUnevenLeafLengths(self):
        data = [[1, 2, 3], [4], [5, 6]]
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedUnevenMidLevelLengths(self):
        data = [[[1, 2], [3, 4], [5, 6]], [[7, 8]]]
        ja = JArray(JInt, 3)(data)
        self.assertEqual(to_nested_list(ja), data)

    # ---- matchRaggedNode/encodeRaggedNode's per-node container-kind
    # dispatch (list vs. tuple vs. generic sequence) -- the tests above
    # only ever exercise the list branch, at both the leaf-container level
    # (remainingDepth == 1) and the recursive mid-level (remainingDepth > 1)
    # ----

    def testRaggedNodeKindTuple(self):
        data = ((1, 2, 3), (4, 5))
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), [[1, 2, 3], [4, 5]])

    def testRaggedNodeKindTupleDeep(self):
        data = (((1, 2), (3,)), ((4, 5, 6),))
        ja = JArray(JInt, 3)(data)
        self.assertEqual(to_nested_list(ja), [[[1, 2], [3]], [[4, 5, 6]]])

    def testRaggedNodeKindGeneric(self):
        GS = common.GenericSequence
        data = GS([GS([1, 2, 3]), GS([4, 5])])
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), [[1, 2, 3], [4, 5]])

    def testRaggedNodeKindGenericDeep(self):
        GS = common.GenericSequence
        data = GS([GS([GS([1, 2]), GS([3])]), GS([GS([4, 5, 6])])])
        ja = JArray(JInt, 3)(data)
        self.assertEqual(to_nested_list(ja), [[[1, 2], [3]], [[4, 5, 6]]])

    def testRaggedMidLevelNonSequenceFallsBackAndRaises(self):
        # A node that's neither list/tuple/sequence at all -- matchRaggedNode's
        # generic-kind non-sequence rejection -- declines the ragged-native
        # fast path and falls back to JPConversionSequence, which raises.
        with self.assertRaises(TypeError):
            JArray(JInt, 2)([[1, 2], 42])

    def testRaggedMidLevelStringFallsBackAndRaises(self):
        # A string is technically a PySequence but explicitly excluded
        # (JPPyString::check) so it isn't walked character-by-character.
        with self.assertRaises(TypeError):
            JArray(JInt, 2)([[1, 2], "ab"])

    def testRaggedLeafFailureInsideTupleFallsBackAndRaises(self):
        # A bad leaf value inside a TUPLE-kind node specifically (not the
        # already-covered LIST-kind failure) -- matchRaggedNode's
        # leaf-level TUPLE loop's own return-false.
        with self.assertRaises(TypeError):
            JArray(JInt, 2)([[1, 2], (3, "x")])

    def testRaggedLeafFailureInsideGenericFallsBackAndRaises(self):
        GS = common.GenericSequence
        with self.assertRaises(TypeError):
            JArray(JInt, 2)([[1, 2], GS([3, "x"])])

    def testRaggedRecursiveFailureInsideTupleFallsBackAndRaises(self):
        # A bad grandchild propagating a failure back up through the
        # recursive (remainingDepth > 1) TUPLE branch specifically.
        with self.assertRaises(TypeError):
            JArray(JInt, 3)([[[1, 2], [3, 4]], ([5, 6], [7, "x"])])

    def testRaggedRecursiveFailureInsideGenericFallsBackAndRaises(self):
        GS = common.GenericSequence
        with self.assertRaises(TypeError):
            JArray(JInt, 3)([[[1, 2], [3, 4]], GS([[5, 6], [7, "x"]])])

    def testRaggedGenericSizeRaisesFallsBackAndRaises(self):
        # matchRaggedNode's generic-kind size() call itself raising (a
        # broken __len__) must decline gracefully, not propagate an
        # unrelated internal exception.
        class BrokenLen:
            def __len__(self):
                raise RuntimeError("boom")

            def __getitem__(self, i):
                raise IndexError

        with self.assertRaises((TypeError, RuntimeError)):
            JArray(JInt, 2)([[1, 2], BrokenLen()])

    def testRaggedEmptySublistAtStart(self):
        data = [[], [1, 2], [3]]
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedEmptySublistInMiddle(self):
        data = [[1, 2], [], [3]]
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedEmptySublistAtEnd(self):
        data = [[1, 2], [3], []]
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedEmptySublistAtDeeperLevel(self):
        data = [[[1, 2], []], [[3]]]
        ja = JArray(JInt, 3)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedEmptyOuterList(self):
        ja = JArray(JInt, 2)([])
        self.assertEqual(to_nested_list(ja), [])

    def testRaggedSingleElementNextToLarge(self):
        data = [[1], list(range(50))]
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedLong3D(self):
        data = [[[1, 2 ** 40], [3]], [[-(2 ** 40), 5, 6]]]
        ja = JArray(JLong, 3)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedFloat2D(self):
        data = [[1.5, 2.5, 3.5], [4.5]]
        ja = JArray(JFloat, 2)(data)
        got = to_nested_list(ja)
        for gr, dr in zip(got, data):
            self.assertElementsAlmostEqual(gr, dr, places=5)

    def testRaggedDouble2D(self):
        data = [[1.1], [2.2, 3.3, 4.4]]
        ja = JArray(JDouble, 2)(data)
        got = to_nested_list(ja)
        for gr, dr in zip(got, data):
            self.assertElementsAlmostEqual(gr, dr, places=9)

    def testRaggedAsArgument(self):
        data = [[1, 2], [3]]
        self.assertEqual(to_nested_list(self.DeepBench.identity2DIntArray(data)), data)
        expected = sum(x for row in data for x in row)
        self.assertEqual(self.DeepBench.sum2DIntArray(data), expected)

    def testRaggedFloatAsArgument(self):
        # Only the method-argument-dispatch path (JPConversionRaggedSequence
        # via findJavaConversion) reaches encodeRaggedLeaf's 'F' case --
        # the JArray(JFloat, 2)(data) constructor path (testRaggedFloat2D
        # above) never calls it, same as the int scope-boundary comment
        # above documents.
        data = [[1.5, 2.5, 3.5], [4.5]]
        expected = sum(x for row in data for x in row)
        self.assertAlmostEqual(self.DeepBench.sum2DFloatArray(data), expected, places=5)

    # ---- matchRaggedNode failure branches, method-argument-dispatch path
    # ----
    # The JArray(...) constructor path never actually calls
    # JPConversionRaggedSequence for these failure cases (a mixed/invalid
    # leaf causes JPArrayClassNestedRagged::findJavaConversionImpl's
    # raggedSequenceConversion->matches() to decline and fall through to
    # plain sequenceConversion before matchRaggedNode's own TUPLE/GENERIC
    # leaf-check or LIST-recursive-failure lines ever run -- confirmed via
    # an isolated gcov reset-and-diff). Only the declared-array-parameter
    # (method-call) dispatch actually exercises those specific lines, so
    # these three targets are written against DeepBench.sum2DIntArray
    # rather than the constructor.

    def testTupleLeafFailureAsArgumentFallsBackAndRaises(self):
        # matchRaggedNode's leaf-level TUPLE loop's own return-false.
        with self.assertRaises(TypeError):
            self.DeepBench.sum2DIntArray([[1, 2], (3, "x")])

    def testGenericLeafFailureAsArgumentFallsBackAndRaises(self):
        # matchRaggedNode's leaf-level GENERIC loop's own return-false.
        GS = common.GenericSequence
        with self.assertRaises(TypeError):
            self.DeepBench.sum2DIntArray([[1, 2], GS([3, "x"])])

    def testGenericSizeRaisesAsArgumentFallsBackAndRaises(self):
        # matchRaggedNode's generic-kind size() call itself raising (a
        # broken __len__) must decline gracefully, not propagate an
        # unrelated internal exception.
        class BrokenLen:
            def __len__(self):
                raise RuntimeError("boom")

            def __getitem__(self, i):
                raise IndexError

        with self.assertRaises((TypeError, RuntimeError)):
            self.DeepBench.sum2DIntArray([[1, 2], BrokenLen()])

    # ---- Z/B/C/S leaf types: ragged-native support, extended to these
    # four 1-/2-byte-wide types alongside I/J/F/D above (isRaggedEligible,
    # jp_classhints.cpp, now covers every primitive type code). Each leaf
    # run of these narrower types is zero-padded up to a 4-byte boundary
    # once the run ends (raggedAlign4 in jp_classhints.cpp) so every
    # length marker at every level -- including one immediately following
    # an odd-length run -- stays 4-byte aligned; Support.readRaggedLeaf
    # (Java side) skips that same padding by position. The tests below
    # are deliberately built around odd leaf-run lengths (3, 1, 5, ...) so
    # a padding-byte-count mismatch on either side would corrupt the very
    # next sibling's length marker instead of silently succeeding.

    def testRaggedByte2D(self):
        data = [[1, 2, 3], [4], [5, 6, 7, 8, 9], []]
        ja = JArray(JByte, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedBoolean2D(self):
        data = [[True, False, True], [False], [True, True, False, False, True]]
        ja = JArray(JBoolean, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedChar2D(self):
        data = [['a', 'b', 'c'], ['d'], ['e', 'f', 'g', 'h', 'i']]
        ja = JArray(JChar, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedShort2D(self):
        data = [[1, 2, 3], [4], [5, 6, 7, 8, 9]]
        ja = JArray(JShort, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedByte3D(self):
        # Padding after an odd-length leaf run must still land the
        # *next sibling's own length marker correctly, one level up from
        # the leaf -- not just the immediately-following leaf marker the
        # 2D test above already covers.
        data = [[[1, 2, 3], [4]], [[5]], [[6, 7, 8, 9, 10], [11, 12]]]
        ja = JArray(JByte, 3)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedBoolean3D(self):
        data = [[[True, False, True], [False]], [[True]],
                [[False, False, True, True, False], [True, False]]]
        ja = JArray(JBoolean, 3)(data)
        self.assertEqual(to_nested_list(ja), data)

    # ---- Z/B/C/S, method-argument-dispatch path (not the JArray(...)
    # constructor) -- same routing distinction I/J/F/D already draw above
    # ----

    def testRaggedByteAsArgument(self):
        data = [[1, 2, 3], [4], [5, 6, 7, 8, 9]]
        self.assertEqual(to_nested_list(self.DeepBench.identity2DByteArray(data)), data)

    def testRaggedBooleanAsArgument(self):
        data = [[True, False, True], [False], [True, True, False, False, True]]
        self.assertEqual(to_nested_list(self.DeepBench.identity2DBooleanArray(data)), data)

    def testRaggedCharAsArgument(self):
        data = [['a', 'b', 'c'], ['d'], ['e', 'f', 'g', 'h', 'i']]
        self.assertEqual(to_nested_list(self.DeepBench.identity2DCharArray(data)), data)

    def testRaggedShortAsArgument(self):
        data = [[1, 2, 3], [4], [5, 6, 7, 8, 9]]
        expected = sum(x for row in data for x in row)
        self.assertEqual(self.DeepBench.sum2DShortArray(data), expected)

    # ---- container-kind dispatch (tuple/generic), spot-checked on the
    # narrower types too -- the 2D/3D tests above only ever exercise the
    # list branch ----

    def testRaggedNodeKindTupleByte(self):
        data = ((1, 2, 3), (4, 5))
        ja = JArray(JByte, 2)(data)
        self.assertEqual(to_nested_list(ja), [[1, 2, 3], [4, 5]])

    def testRaggedNodeKindGenericBoolean(self):
        GS = common.GenericSequence
        data = GS([GS([True, False, True]), GS([False])])
        ja = JArray(JBoolean, 2)(data)
        self.assertEqual(to_nested_list(ja), [[True, False, True], [False]])

    # ---- mixed-type fallback, Z/B/C/S: a non-conforming element
    # disqualifies the ragged-native match and falls through to the
    # general per-element path, mirroring the I bool-widening tests above
    # ----

    def testMixedByteThenBoolFallsBackAndSucceeds(self):
        # bool fails isRaggedLeafElement's PyLong_CheckExact for 'B', just
        # like it does for 'I' -- falls back to the general per-element
        # path, which does accept it (bool -> byte is a valid implicit
        # conversion there).
        data = [[1, 2], [True, 4]]
        ja = JArray(JByte, 2)(data)
        self.assertEqual(to_nested_list(ja), [[1, 2], [1, 4]])

    def testMixedShortThenBoolFallsBackAndSucceeds(self):
        data = [[1, 2], [True, 4]]
        ja = JArray(JShort, 2)(data)
        self.assertEqual(to_nested_list(ja), [[1, 2], [1, 4]])

    def testMixedBooleanThenIntFallsBackAndSucceeds(self):
        # A plain int fails isRaggedLeafElement's PyBool_Check for 'Z' --
        # falls back to the general per-element path, which accepts any
        # truthy/falsy value, not just an exact bool.
        data = [[True, False], [1, 0]]
        ja = JArray(JBoolean, 2)(data)
        self.assertEqual(to_nested_list(ja), [[True, False], [True, False]])

    def testMixedCharThenIntFallsBackAndSucceeds(self):
        # A plain int fails isRaggedLeafElement's exact-length-1-string
        # check for 'C' -- falls back to the general per-element path,
        # which accepts an index as the char's UTF-16 code point.
        data = [['a', 'b'], [ord('c'), 'd']]
        ja = JArray(JChar, 2)(data)
        self.assertEqual(to_nested_list(ja), [['a', 'b'], ['c', 'd']])

    # ---- out-of-range leaf values: matchRaggedNode's isRaggedLeafElement
    # only checks Python type, not Java range, so an in-range-type,
    # out-of-range-value leaf is accepted by the match and must still be
    # rejected at encode time (encodeRaggedLeaf's assertRange call) ----

    def testRaggedByteOutOfRangeRaises(self):
        with self.assertRaises(OverflowError):
            JArray(JByte, 2)([[1, 2], [300]])

    def testRaggedShortOutOfRangeRaises(self):
        with self.assertRaises(OverflowError):
            JArray(JShort, 2)([[1, 2], [100000]])

    # ---- mixed-type fallback: a non-conforming element (bool, or a value
    # of the wrong exact type) partway through disqualifies the whole
    # match for this conversion and falls through to the general path,
    # which must still succeed (or fail) exactly as it does today ----

    def testMixedIntThenBoolFallsBackAndSucceeds(self):
        # bool is a subclass of int but fails PyLong_CheckExact -- must
        # not be silently misencoded as an int; falls back to the general
        # per-element path, which does accept it (bool -> int is a valid
        # implicit conversion there).
        data = [[1, 2], [True, 4]]
        ja = JArray(JInt, 2)(data)
        got = to_nested_list(ja)
        self.assertEqual(got, [[1, 2], [1, 4]])

    def testMixedIntThenFloatRaises(self):
        with self.assertRaises(TypeError):
            JArray(JInt, 2)([[1, 2], [3, 4.5]])

    def testMixedIntThenStringRaises(self):
        with self.assertRaises(TypeError):
            JArray(JInt, 2)([[1, 2], [3, "x"]])

    # ---- overload disambiguation: two candidates differing only in array
    # leaf element type -- confirms matches() correctly qualifies *both*
    # candidates for a ragged plain-int list (no buffer built for either,
    # since that only happens in convert() for the winner). int[][] and
    # long[][] are unrelated Java types (no widening relationship between
    # array types the way there is for the scalar primitives), so two
    # equal-quality (_implicit) matches is genuinely ambiguous -- same
    # behavior JPMethodDispatch::findOverload already gives for any other
    # pair of equally-qualified, unrelated candidates, not something this
    # phase changes. The interesting assertion here is what it is *not*:
    # not a spurious pick, not a crash, not a silently-wrong result.

    def testOverloadResolutionReportsAmbiguity(self):
        data = [[1, 2], [3]]
        with self.assertRaises(TypeError):
            self.DeepBench.overloadArrayType(data)

    # ---- flat (1D) list push: the same type-widening contract users get
    # at depth >= 2 above (bool implicitly widens to int; a genuinely
    # incompatible element type raises TypeError) must also hold one
    # dimension down, regardless of which conversion happens to implement
    # it internally. Not already covered elsewhere for a flat list --
    # unlike empty/single-element flat-list construction, already covered
    # verbatim in test_arrayPullPush.py/test_arrayToList.py. ----

    def testFlatMixedIntThenBoolFallsBackAndSucceeds(self):
        # bool is a subclass of int but fails PyLong_CheckExact -- the
        # fast per-list-of-exact-ints path in JPIntType::setArrayRange
        # breaks out on it, falling to the general per-element path,
        # which does accept it (bool -> int is a valid implicit
        # conversion there).
        data = [1, 2, True, 4]
        ja = JArray(JInt, 1)(data)
        self.assertEqual(to_nested_list(ja), [1, 2, 1, 4])

    def testFlatMixedIntThenFloatRaises(self):
        with self.assertRaises(TypeError):
            JArray(JInt, 1)([1, 2, 4.5])

    def testFlatMixedIntThenStringRaises(self):
        with self.assertRaises(TypeError):
            JArray(JInt, 1)([1, 2, "x"])
