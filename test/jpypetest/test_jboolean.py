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

    def testBooleanFromInt(self):
        self.assertEqual(self.Test.callBoolean(int(123)), True)
        self.assertEqual(self.Test.callBoolean(int(0)), False)

    @common.requireNumpy
    def testBooleanFromNPInt(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int(123)), True)

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
    def testBooleanFromNPFloat(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callBoolean(np.float(2))

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

    @common.requireNumpy
    def testSetFromNPBoolArray(self):
        import numpy as np
        n = 100
        a = np.random.randint(0, 1, size=n, dtype=np.bool)
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
