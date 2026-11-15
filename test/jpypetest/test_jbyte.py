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
from jpype import java
import sys
import logging
import time
import common
try:
    import numpy as np
except ImportError:
    pass


class JByteTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.fixture = jpype.JClass("jpype.common.Fixture")()

    @common.requireInstrumentation
    def testConversionFault(self):
        _jpype.fault("JPByteType::findJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JByte._canConvertToJava(object())

    def testArrayClassHints(self):
        # JPArrayClassByte::getConversionInfo, reached via the array
        # class's _hints introspection property.
        hints = jpype.JClass(JByte[:])._hints
        self.assertEqual(list(hints.returns), [jpype.JClass(JByte[:])])

    @common.requireInstrumentation
    def testArrayFaults(self):
        ja = JArray(JByte)(5)
        _jpype.fault("JPByteType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0, 0]
        _jpype.fault("JPJavaFrame::NewByteArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JByte)(1)
        _jpype.fault("JPJavaFrame::SetByteArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 0
        _jpype.fault("JPJavaFrame::GetByteArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])
        _jpype.fault("JPJavaFrame::GetByteArrayElements")
        with self.assertRaises(BufferError):
            memoryview(ja[0:3])
        # ja[0:3] = bytes(...) and cloning a slice both go through
        # tryFastBufferPush's DirectByteBuffer handoff now (setArrayRange
        # tries it before falling back to the Get/ReleaseByteArrayElements
        # critical section), so the fault point to arm is
        # fillFlatIntoArray, not ReleaseByteArrayElements -- that release
        # call is never reached for a buffer-protocol source.
        _jpype.fault("JPJavaFrame::fillFlatIntoArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:3] = bytes([1, 2, 3])
        _jpype.fault("JPJavaFrame::fillFlatIntoArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            jpype.JObject(ja[::2], jpype.JObject)
        _jpype.fault("JPJavaFrame::ReleaseByteArrayElements")

        def f():
            # Special case no fault is allowed
            memoryview(ja[0:3])
        f()

    def testByteFromInt(self):
        self.assertEqual(self.fixture.callByte(int(123)), 123)

    @common.requireNumpy
    def testByteFromNPInt(self):
        import numpy as np
        self.assertEqual(self.fixture.callByte(np.int_(123)), 123)

    @common.requireNumpy
    def testByteFromNPInt8(self):
        import numpy as np
        self.assertEqual(self.fixture.callByte(np.int8(123)), 123)
        self.assertEqual(self.fixture.callByte(np.uint8(123)), 123)

    @common.requireNumpy
    def testByteFromNPInt16(self):
        import numpy as np
        self.assertEqual(self.fixture.callByte(np.int16(123)), 123)
        self.assertEqual(self.fixture.callByte(np.uint16(123)), 123)

    @common.requireNumpy
    def testByteFromNPInt32(self):
        import numpy as np
        self.assertEqual(self.fixture.callByte(np.int32(123)), 123)
        self.assertEqual(self.fixture.callByte(np.uint32(123)), 123)

    @common.requireNumpy
    def testByteFromNPInt64(self):
        import numpy as np
        self.assertEqual(self.fixture.callByte(np.int64(123)), 123)
        self.assertEqual(self.fixture.callByte(np.uint64(123)), 123)

    def testByteFromFloat(self):
        with self.assertRaises(TypeError):
            self.fixture.callByte(float(2))

    @common.requireNumpy
    def testByteFromNPFloat16(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.fixture.callByte(np.float16(2))

    @common.requireNumpy
    def testByteFromNPFloat32(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.fixture.callByte(np.float32(2))

    @common.requireNumpy
    def testByteFromNPFloat64(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.fixture.callByte(np.float64(2))

    def testByteRange(self):
        with self.assertRaises(OverflowError):
            self.fixture.callByte(int(1e10))
        with self.assertRaises(OverflowError):
            self.fixture.callByte(int(-1e10))

    def testExplicitRange(self):
        # These will not overflow as they are explicit casts
        self.assertEqual(JByte(2**8), 0)
        self.assertEqual(JByte(-2**8), 0)

    def testByteFromNone(self):
        with self.assertRaises(TypeError):
            self.fixture.callByte(None)

    def testByteArrayAsString(self):
        t = JClass("jpype.array.TestArray")()
        v = t.getByteArray()
        self.assertEqual(str(v), 'avcd')

    def testByteArrayIntoVector(self):
        ba = jpype.JArray(jpype.JByte)(b'123')
        v = jpype.java.util.Vector(1)
        v.add(ba)
        self.assertEqual(len(v), 1)
        self.assertNotEqual(v[0], None)

    def testByteArraySimple(self):
        a = JArray(JByte)(2)
        a[1] = 2
        self.assertEqual(a[1], 2)

    def testJArrayConversionByte(self):
        expected = (0, 1, 2, 3)
        ByteBuffer = jpype.java.nio.ByteBuffer
        bb = ByteBuffer.allocate(4)
        buf = bb.array()
        for i in range(len(expected)):
            buf[i] = expected[i]
        for i in range(len(expected)):
            self.assertEqual(expected[i], buf[i])

    def testFromObject(self):
        ja = JArray(JByte)(5)
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.common.Fixture")
        with self.assertRaises(TypeError):
            jf.static_byte_field = object()
        with self.assertRaises(TypeError):
            jf().byte_field = object()

    def testArrayHash(self):
        ja = JArray(JByte)([1, 2, 3])
        self.assertIsInstance(hash(ja), int)

    @common.requireNumpy
    def testArrayBufferDims(self):
        ja = JArray(JByte)(5)
        a = np.zeros((5, 2))
        with self.assertRaisesRegex(TypeError, "incorrect"):
            ja[:] = a

    def testArrayBadItem(self):
        class q(object):
            def __int__(self):
                raise SystemError("nope")

            def __index__(self):
                raise SystemError("nope")
        ja = JArray(JByte)(5)
        a = [1, -1, q(), 3, 4]
        with self.assertRaisesRegex(SystemError, "nope"):
            ja[:] = a

    def testArrayBadDims(self):
        class q(bytes):
            # Lie about our length
            def __len__(self):
                return 5
        a = q([1, 2, 3])
        ja = JArray(JByte)(5)
        with self.assertRaisesRegex(ValueError, "Slice"):
            ja[:] = [1, 2, 3]
        with self.assertRaisesRegex(ValueError, "mismatch"):
            ja[:] = a

    def testArraySetRange(self):
        ja = JArray(JByte)(3)
        ja[0:1] = [123]
        self.assertEqual(ja[0], 123)
        ja[0:1] = [-1]
        self.assertEqual(ja[0], -1)
        with self.assertRaises(TypeError):
            ja[0:1] = [1.000]
        with self.assertRaises(TypeError):
            ja[0:1] = [java.lang.Double(321)]
        with self.assertRaises(TypeError):
            ja[0:1] = [object()]

    def testArraySetRangeTuple(self):
        ja = JArray(JByte)(3)
        ja[0:2] = (100, -1)
        self.assertEqual(list(ja[0:2]), [100, -1])
        with self.assertRaises(TypeError):
            ja[0:1] = (1.000,)
        with self.assertRaises(TypeError):
            ja[0:1] = (object(),)

    def testArraySetRangeSequence(self):
        ja = JArray(JByte)(3)
        ja[0:2] = common.GenericSequence([100, -1])
        self.assertEqual(list(ja[0:2]), [100, -1])
        with self.assertRaises(TypeError):
            ja[0:1] = common.GenericSequence([1.000])
        with self.assertRaises(TypeError):
            ja[0:1] = common.GenericSequence([object()])

    def testArraySetRangeTupleNonExactIndex(self):
        # A bool is a valid __index__ object but not PyLong_CheckExact --
        # exercises the TUPLE loop's PyIndex_Check fallback conversion
        # (setArrayRange's non-exact-int sub-branch), not just its fast
        # PyLong_CheckExact path.
        ja = JArray(JByte)(2)
        ja[0:2] = (1, True)
        self.assertEqual(list(ja[0:2]), [1, 1])

    def testArraySetRangeListNonExactIndex(self):
        # Same as testArraySetRangeTupleNonExactIndex, but the LIST loop's
        # own PyIndex_Check fallback sub-branch.
        ja = JArray(JByte)(2)
        ja[0:2] = [1, True]
        self.assertEqual(list(ja[0:2]), [1, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallback(self):
        # A negative-stride (reversed) source declines the bulk
        # tryFastBufferPush path, falling back to the per-element
        # getConverter()/Convert<T> path in setArrayRange.
        ja = JArray(JByte)(3)
        a = np.array([10, 20, 30], dtype=np.int32)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackInt16Source(self):
        # getConverter's int16_t source case (from[0] == 'h', non-swapped)
        # -> 'b' target.
        ja = JArray(JByte)(3)
        a = np.array([1, 2, 3], dtype=np.int16)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackInt16SourceSwapped(self):
        # Same, but with an explicit non-native byte order so getConverter
        # takes the Reverse<Convert<int16_t>::toB>::call2 branch instead.
        ja = JArray(JByte)(3)
        a = np.array([1, 2, 3], dtype='>i2')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint16Source(self):
        # getConverter's uint16_t source case (from[0] == 'H', non-swapped)
        # -> 'b' target.
        ja = JArray(JByte)(3)
        a = np.array([1, 2, 3], dtype=np.uint16)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint16SourceSwapped(self):
        ja = JArray(JByte)(3)
        a = np.array([1, 2, 3], dtype='>u2')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackInt32SourceSwapped(self):
        # getConverter's int32_t source case (from[0] in 'i','l', swapped)
        # -> 'b' target. Non-swapped 'b' is already covered by
        # testArraySetRangeBufferFallback above (np.int32 source).
        ja = JArray(JByte)(3)
        a = np.array([10, 20, 30], dtype='>i4')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint32Source(self):
        # getConverter's uint32_t source case (from[0] in 'I','L',
        # non-swapped) -> 'b' target.
        ja = JArray(JByte)(3)
        a = np.array([10, 20, 30], dtype=np.uint32)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint32SourceSwapped(self):
        ja = JArray(JByte)(3)
        a = np.array([10, 20, 30], dtype='>u4')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint64Source(self):
        # getConverter's uint64_t source case (from[0] == 'Q',
        # non-swapped) -> 'b' target.
        ja = JArray(JByte)(3)
        a = np.array([10, 20, 30], dtype=np.uint64)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint64SourceSwapped(self):
        ja = JArray(JByte)(3)
        a = np.array([10, 20, 30], dtype='>u8')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat32Source(self):
        # getConverter's float source case (from[0] == 'f', non-swapped)
        # -> 'b' target.
        ja = JArray(JByte)(3)
        a = np.array([10, 20, 30], dtype=np.float32)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat32SourceSwapped(self):
        ja = JArray(JByte)(3)
        a = np.array([10, 20, 30], dtype='>f4')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat64Source(self):
        # getConverter's double source case (from[0] == 'd', non-swapped)
        # -> 'b' target.
        ja = JArray(JByte)(3)
        a = np.array([10, 20, 30], dtype=np.float64)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat64SourceSwapped(self):
        ja = JArray(JByte)(3)
        a = np.array([10, 20, 30], dtype='>f8')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat16SourceSwapped(self):
        # getConverter's float16 source case (from[0] == 'e', swapped) ->
        # 'b' target.
        ja = JArray(JByte)(3)
        a = np.array([1, 2, 3], dtype='>f2')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [3, 2, 1])

    def testArraySetRangeBufferFallbackIntpSource(self):
        # getConverter's Py_ssize_t source case (from[0] == 'n') -> 'b'
        # target.
        ja = JArray(JByte)(3)
        mv = memoryview(bytearray(24)).cast('n')
        mv[0], mv[1], mv[2] = 10, 20, 30
        ja[0:3] = mv[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    def testArraySetRangeBufferFallbackUintpSource(self):
        # getConverter's size_t source case (from[0] == 'N') -> 'b'
        # target.
        ja = JArray(JByte)(3)
        mv = memoryview(bytearray(24)).cast('N')
        mv[0], mv[1], mv[2] = 10, 20, 30
        ja[0:3] = mv[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackInt8Source(self):
        # getConverter's int8_t source case (from[0] in '?','c','b') ->
        # 'b' target -- only 'z' was hit by an int8 source anywhere else.
        ja = JArray(JByte)(3)
        a = np.array([10, 20, 30], dtype=np.int8)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [30, 20, 10])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat16Subnormal(self):
        # jp_convert.cpp's Half<Convert<float>::toB>::convert -- a
        # subnormal half-float (exp==0, frac!=0) truncated to byte is 0
        # regardless of which nonzero subnormal magnitude.
        bits = np.array([1, 0x0200, 0x03ff], dtype=np.uint16)
        a = bits.view(np.float16)
        ja = JArray(JByte)(3)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [0, 0, 0])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat16InfNan(self):
        # jp_convert.cpp's Half<Convert<float>::toB>::convert -- the "to
        # infinity and beyond" branch (exp==31).
        bits = np.array([0x7C00, 0xFC00, 0x7E00], dtype=np.uint16)
        a = bits.view(np.float16)
        ja = JArray(JByte)(3)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [0, 0, 0])
