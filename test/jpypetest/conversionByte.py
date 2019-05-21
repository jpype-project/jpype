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
from . import common
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


class ConversionByteTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Test = jpype.JClass("jpype.conversion.Test")

    def testByteFromInt(self):
        self.assertEquals(self.Test.callByte(int(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteFromNPInt(self):
        import numpy as np
        self.assertEquals(self.Test.callByte(np.int(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteFromNPInt8(self):
        import numpy as np
        self.assertEquals(self.Test.callByte(np.int8(123)), 123)
        self.assertEquals(self.Test.callByte(np.uint8(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteFromNPInt16(self):
        import numpy as np
        self.assertEquals(self.Test.callByte(np.int16(123)), 123)
        self.assertEquals(self.Test.callByte(np.uint16(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteFromNPInt32(self):
        import numpy as np
        self.assertEquals(self.Test.callByte(np.int32(123)), 123)
        self.assertEquals(self.Test.callByte(np.uint32(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteFromNPInt64(self):
        import numpy as np
        self.assertEquals(self.Test.callByte(np.int64(123)), 123)
        self.assertEquals(self.Test.callByte(np.uint64(123)), 123)

    def testByteFromFloat(self):
        with self.assertRaises(RuntimeError):
            self.Test.callByte(float(2))

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteFromNPFloat(self):
        import numpy as np
        with self.assertRaises(RuntimeError):
            self.Test.callByte(np.float(2))

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteFromNPFloat32(self):
        import numpy as np
        with self.assertRaises(RuntimeError):
            self.Test.callByte(np.float32(2))

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testByteFromNPFloat64(self):
        import numpy as np
        with self.assertRaises(RuntimeError):
            self.Test.callByte(np.float64(2))

    def testByteRange(self):
        with self.assertRaises(OverflowError):
            self.Test.callByte(long(1e10))
        with self.assertRaises(OverflowError):
            self.Test.callByte(long(-1e10))
