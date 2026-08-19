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
import sys
import logging
import time
import common
try:
    import numpy as np
except ImportError:
    pass


class JBooleanTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Test = jpype.JClass("jpype.common.Fixture")()

    @common.requireInstrumentation
    def testJPBoolean_str(self):
        jb = JBoolean(True)
        _jpype.fault("PyJPBoolean_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jb)
        _jpype.fault("PyJPModule_getContext")
        str(jb)

    @common.requireInstrumentation
    def testJPBooleanType(self):
        ja = JArray(JBoolean)(5)  # lgtm [py/similar-function]
        _jpype.fault("JPBooleanType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0, 0]
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.common.Fixture")
        with self.assertRaises(TypeError):
            jf.static_bool_field = object()
        with self.assertRaises(TypeError):
            jf().bool_field = object()

    @common.requireInstrumentation
    def testJBooleanGetJavaConversion(self):
        _jpype.fault("JPBooleanType::findJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JBoolean._canConvertToJava(object())

    def testArrayClassNoMatchRaises(self):
        # JPArrayClassBoolean::findJavaConversionImpl -- object() matches
        # none of null/object/buffer/list/tuple/sequence/hints, walking
        # the whole chain down to the final _none fallthrough.
        self.assertEqual(JArray(JBoolean)._canConvertToJava(object()), "none")

    def testBooleanFromInt(self):
        self.assertEqual(self.Test.callBoolean(int(123)), True)
        self.assertEqual(self.Test.callBoolean(int(0)), False)

    @common.requireNumpy
    def testBooleanFromNPInt(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int_(123)), True)

    @common.requireNumpy
    def testBooleanFromNPInt8(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int8(123)), True)
        self.assertEqual(self.Test.callBoolean(np.uint8(123)), True)

    @common.requireNumpy
    def testBooleanFromNPInt16(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int16(123)), True)
        self.assertEqual(self.Test.callBoolean(np.uint16(123)), True)

    @common.requireNumpy
    def testBooleanFromNPInt32(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int32(123)), True)
        self.assertEqual(self.Test.callBoolean(np.uint32(123)), True)

    @common.requireNumpy
    def testBooleanFromNPInt64(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int64(123)), True)
        self.assertEqual(self.Test.callBoolean(np.uint64(123)), True)

    def testBooleanFromFloat(self):
        with self.assertRaises(TypeError):
            self.Test.callBoolean(float(2))

    @common.requireNumpy
    def testBooleanFromNPFloat16(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callBoolean(np.float16(2))

    @common.requireNumpy
    def testBooleanFromNPFloat32(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callBoolean(np.float32(2))

    @common.requireNumpy
    def testBooleanFromNPFloat64(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callBoolean(np.float64(2))

    def testBooleanFromNone(self):
        with self.assertRaises(TypeError):
            self.Test.callBoolean(None)

    def testJArrayConversionBool(self):
        expected = [True, False, False, True]
        jarr = jpype.JArray(jpype.JBoolean)(expected)
        self.assertEqual(expected, list(jarr[:]))

    def testArraySetRangeTuple(self):
        ja = JArray(JBoolean)(3)
        ja[0:3] = (True, False, 1)
        self.assertEqual(list(ja[0:3]), [True, False, True])

    def testArraySetRangeSequence(self):
        ja = JArray(JBoolean)(3)
        ja[0:3] = common.GenericSequence([True, False, 1])
        self.assertEqual(list(ja[0:3]), [True, False, True])

    def testArraySetRangeTupleBoolRaises(self):
        # PyObject_IsTrue's error path (a __bool__ that raises) in the
        # TUPLE loop's non-exact-bool fallback.
        class Bad:
            def __bool__(self):
                raise RuntimeError("boom")
        ja = JArray(JBoolean)(2)
        with self.assertRaises(RuntimeError):
            ja[0:2] = (True, Bad())

    def testArraySetRangeSequenceBoolRaises(self):
        class Bad:
            def __bool__(self):
                raise RuntimeError("boom")
        ja = JArray(JBoolean)(2)
        with self.assertRaises(RuntimeError):
            ja[0:2] = common.GenericSequence([True, Bad()])

    @common.requireNumpy
    def testArraySetRangeBufferFallback(self):
        # A negative-stride (reversed) source declines the bulk
        # tryFastBufferPush path, falling back to the per-element
        # getConverter()/Convert<T> path in setArrayRange.
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype=np.int8)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat16Subnormal(self):
        # jp_convert.cpp's Half<Convert<float>::toZ>::convert -- a
        # subnormal half-float (exp==0, frac!=0) is nonzero, so truncates
        # to true, same as any other nonzero magnitude would.
        import numpy as np
        bits = np.array([1, 0x0200, 0x03ff], dtype=np.uint16)
        a = bits.view(np.float16)
        ja = JArray(JBoolean)(3)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, True, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackInt16Source(self):
        # getConverter's int16_t source case (from[0] == 'h', non-swapped)
        # -> 'z' target.
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype=np.int16)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackInt16SourceSwapped(self):
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype='>i2')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint16Source(self):
        # getConverter's uint16_t source case (from[0] == 'H', non-swapped)
        # -> 'z' target.
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype=np.uint16)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint16SourceSwapped(self):
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype='>u2')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackInt32Source(self):
        # getConverter's int32_t source case (from[0] in 'i','l',
        # non-swapped) -> 'z' target.
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype=np.int32)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackInt32SourceSwapped(self):
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype='>i4')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint32Source(self):
        # getConverter's uint32_t source case (from[0] in 'I','L',
        # non-swapped) -> 'z' target.
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype=np.uint32)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint32SourceSwapped(self):
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype='>u4')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint64Source(self):
        # getConverter's uint64_t source case (from[0] == 'Q',
        # non-swapped) -> 'z' target.
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype=np.uint64)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackUint64SourceSwapped(self):
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype='>u8')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat32Source(self):
        # getConverter's float source case (from[0] == 'f', non-swapped)
        # -> 'z' target.
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype=np.float32)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat32SourceSwapped(self):
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype='>f4')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat64Source(self):
        # getConverter's double source case (from[0] == 'd', non-swapped)
        # -> 'z' target.
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype=np.float64)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat64SourceSwapped(self):
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype='>f8')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat16SourceSwapped(self):
        # getConverter's float16 source case (from[0] == 'e', swapped) ->
        # 'z' target -- Reverse<Half<Convert<float>::toZ>::convert>::call4.
        import numpy as np
        ja = JArray(JBoolean)(3)
        a = np.array([1, 0, 1], dtype='>f2')
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, False, True])

    def testArraySetRangeBufferFallbackIntpSource(self):
        # getConverter's Py_ssize_t source case (from[0] == 'n') -> 'z'
        # target. numpy never produces format 'n' (its intp buffer format
        # is 'l' on this platform, already covered elsewhere) -- use
        # memoryview.cast('n') to get a genuine 'n'-format buffer.
        ja = JArray(JBoolean)(3)
        mv = memoryview(bytearray(24)).cast('n')
        mv[0] = 1
        mv[1] = 0
        mv[2] = 1
        ja[0:3] = mv[::-1]
        self.assertEqual(list(ja), [True, False, True])

    def testArraySetRangeBufferFallbackUintpSource(self):
        # getConverter's size_t source case (from[0] == 'N') -> 'z'
        # target.
        ja = JArray(JBoolean)(3)
        mv = memoryview(bytearray(24)).cast('N')
        mv[0] = 1
        mv[1] = 0
        mv[2] = 1
        ja[0:3] = mv[::-1]
        self.assertEqual(list(ja), [True, False, True])

    @common.requireNumpy
    def testArraySetRangeBufferFallbackFloat16InfNan(self):
        # jp_convert.cpp's Half<Convert<float>::toZ>::convert -- the "to
        # infinity and beyond" branch (exp==31): all nonzero, so all true.
        import numpy as np
        bits = np.array([0x7C00, 0xFC00, 0x7E00], dtype=np.uint16)
        a = bits.view(np.float16)
        ja = JArray(JBoolean)(3)
        ja[0:3] = a[::-1]
        self.assertEqual(list(ja), [True, True, True])

    @common.requireNumpy
    def testSetFromNPBoolArray(self):
        import numpy as np
        n = 100
        a = np.random.randint(0, 1, size=n, dtype=np.bool_)
        jarr = jpype.JArray(jpype.JBoolean)(n)
        jarr[:] = a
        self.assertCountEqual(a, jarr)

    @common.requireNumpy
    def testArrayBufferDims(self):
        ja = JArray(JBoolean)(5)
        a = np.zeros((5, 2))
        with self.assertRaisesRegex(TypeError, "incorrect"):
            ja[:] = a

    def testArrayBadItem(self):
        class q(object):
            def __bool__(self):
                raise SystemError("nope")
        ja = JArray(JBoolean)(5)
        a = [1, 2, q(), 3, 4]
        with self.assertRaisesRegex(SystemError, "nope"):
            ja[:] = a

    def testArrayBadDims(self):
        class q(bytes):
            # Lie about our length
            def __len__(self):
                return 5
        a = q([1, 2, 3])
        ja = JArray(JBoolean)(5)
        with self.assertRaisesRegex(ValueError, "Slice"):
            ja[:] = [1, 2, 3]
        with self.assertRaisesRegex(ValueError, "mismatch"):
            ja[:] = a
