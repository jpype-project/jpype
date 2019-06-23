# *****************************************************************************
#   Copyright 2017 Karl Einar Nelson
#
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
# *****************************************************************************
import jpype
import sys
import logging
import time
import common

if sys.version > '3':
    long = int


def haveNumpy():
    try:
        import numpy
        return True
    except ImportError:
        return False


class ConversionDoubleTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.value = 1.0+1.0/65536
        self.Test = jpype.JClass("jpype.types.MethodsTest")()

    def testDoubleFromInt(self):
        self.assertEqual(self.Test.callDouble(int(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleFromNPInt(self):
        import numpy as np
        self.assertEqual(self.Test.callDouble(np.int(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleFromNPInt8(self):
        import numpy as np
        self.assertEqual(self.Test.callDouble(np.int8(123)), 123)
        self.assertEqual(self.Test.callDouble(np.uint8(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleFromNPInt16(self):
        import numpy as np
        self.assertEqual(self.Test.callDouble(np.int16(123)), 123)
        self.assertEqual(self.Test.callDouble(np.uint16(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleFromNPInt32(self):
        import numpy as np
        self.assertEqual(self.Test.callDouble(np.int32(123)), 123)
        self.assertEqual(self.Test.callDouble(np.uint32(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleFromNPInt64(self):
        import numpy as np
        self.assertEqual(self.Test.callDouble(np.int64(123)), 123)
        self.assertEqual(self.Test.callDouble(np.uint64(123)), 123)

    def testDoubleFromFloat(self):
        self.assertEqual(self.Test.callDouble(float(self.value)), self.value)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleFromNPFloat(self):
        import numpy as np
        self.assertEqual(self.Test.callDouble(
            np.float(self.value)), self.value)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleFromNPFloat32(self):
        import numpy as np
        self.assertEqual(self.Test.callDouble(
            np.float32(self.value)), self.value)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testDoubleFromNPFloat64(self):
        import numpy as np
        self.assertEqual(self.Test.callDouble(
            np.float64(self.value)), self.value)

    def testDoubleRange(self):
        self.assertEqual(self.Test.callDouble(float(1e340)), float(1e340))
        self.assertEqual(self.Test.callDouble(float(-1e340)), float(-1e340))

    def testDoubleNaN(self):
        import math
        self.assertTrue(math.isnan(self.Test.callDouble(float("nan"))))
