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
try:
    import unittest2 as unittest
except ImportError:
    import unittest

if sys.version > '3':
    long = int


def haveNumpy():
    try:
        import numpy
        return True
    except ImportError:
        return False


class ConversionFloatTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.value = 1.0+1.0/65536
        self.Test = jpype.JClass("jpype.conversion.Test")

    def testFloatFromInt(self):
        self.assertEquals(self.Test.callFloat(int(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatFromNPInt(self):
        import numpy as np
        self.assertEquals(self.Test.callFloat(np.int(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatFromNPInt8(self):
        import numpy as np
        self.assertEquals(self.Test.callFloat(np.int8(123)), 123)
        self.assertEquals(self.Test.callFloat(np.uint8(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatFromNPInt16(self):
        import numpy as np
        self.assertEquals(self.Test.callFloat(np.int16(123)), 123)
        self.assertEquals(self.Test.callFloat(np.uint16(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatFromNPInt32(self):
        import numpy as np
        self.assertEquals(self.Test.callFloat(np.int32(123)), 123)
        self.assertEquals(self.Test.callFloat(np.uint32(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatFromNPInt64(self):
        import numpy as np
        self.assertEquals(self.Test.callFloat(np.int64(123)), 123)
        self.assertEquals(self.Test.callFloat(np.uint64(123)), 123)

    def testFloatFromFloat(self):
        self.assertEquals(self.Test.callFloat(float(self.value)), self.value)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatFromNPFloat(self):
        import numpy as np
        self.assertEquals(self.Test.callFloat(
            np.float(self.value)), self.value)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatFromNPFloat32(self):
        import numpy as np
        self.assertEquals(self.Test.callFloat(
            np.float32(self.value)), self.value)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testFloatFromNPFloat64(self):
        import numpy as np
        self.assertEquals(self.Test.callFloat(
            np.float64(self.value)), self.value)

    def testFloatRange(self):
        with self.assertRaises(OverflowError):
            self.Test.callFloat(float(1e40))
        with self.assertRaises(OverflowError):
            self.Test.callFloat(float(-1e40))
