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
import jpype
from jpype.types import *
import sys
import logging
import time
import common


def haveNumpy():
    try:
        import numpy
        return True
    except ImportError:
        return False


class ConversionLongTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Test = jpype.JClass("jpype.common.Fixture")()

    def testLongFromInt(self):
        self.assertEqual(self.Test.callLong(int(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongFromNPInt(self):
        import numpy as np
        self.assertEqual(self.Test.callLong(np.int(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongFromNPInt8(self):
        import numpy as np
        self.assertEqual(self.Test.callLong(np.int8(123)), 123)
        self.assertEqual(self.Test.callLong(np.uint8(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongFromNPInt16(self):
        import numpy as np
        self.assertEqual(self.Test.callLong(np.int16(123)), 123)
        self.assertEqual(self.Test.callLong(np.uint16(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongFromNPInt32(self):
        import numpy as np
        self.assertEqual(self.Test.callLong(np.int32(123)), 123)
        self.assertEqual(self.Test.callLong(np.uint32(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongFromNPInt64(self):
        import numpy as np
        self.assertEqual(self.Test.callLong(np.int64(123)), 123)
        self.assertEqual(self.Test.callLong(np.uint64(123)), 123)

    def testLongFromFloat(self):
        with self.assertRaises(TypeError):
            self.Test.callLong(float(2))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongFromNPFloat(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callLong(np.float(2))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongFromNPFloat32(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callLong(np.float32(2))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testLongFromNPFloat64(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callLong(np.float64(2))

    def testLongRange(self):
        with self.assertRaises(OverflowError):
            self.Test.callLong(int(1e30))
        with self.assertRaises(OverflowError):
            self.Test.callLong(int(-1e30))

    def testLongFromNone(self):
        with self.assertRaises(TypeError):
            self.Test.callLong(None)
