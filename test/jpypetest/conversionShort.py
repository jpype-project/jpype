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


class ConversionShortTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Test = jpype.JClass("jpype.conversion.Test")

    def testShortFromInt(self):
        self.assertEquals(self.Test.callShort(int(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPInt(self):
        import numpy as np
        self.assertEquals(self.Test.callShort(np.int(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPInt8(self):
        import numpy as np
        self.assertEquals(self.Test.callShort(np.int8(123)), 123)
        self.assertEquals(self.Test.callShort(np.uint8(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPInt16(self):
        import numpy as np
        self.assertEquals(self.Test.callShort(np.int16(123)), 123)
        self.assertEquals(self.Test.callShort(np.uint16(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPInt32(self):
        import numpy as np
        self.assertEquals(self.Test.callShort(np.int32(123)), 123)
        self.assertEquals(self.Test.callShort(np.uint32(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPInt64(self):
        import numpy as np
        self.assertEquals(self.Test.callShort(np.int64(123)), 123)
        self.assertEquals(self.Test.callShort(np.uint64(123)), 123)

    def testShortFromFloat(self):
        with self.assertRaises(RuntimeError):
            self.Test.callShort(float(2))

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPFloat(self):
        import numpy as np
        with self.assertRaises(RuntimeError):
            self.Test.callShort(np.float(2))

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPFloat32(self):
        import numpy as np
        with self.assertRaises(RuntimeError):
            self.Test.callShort(np.float32(2))

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPFloat64(self):
        import numpy as np
        with self.assertRaises(RuntimeError):
            self.Test.callShort(np.float64(2))

    def testShortRange(self):
        with self.assertRaises(OverflowError):
            self.Test.callShort(long(1e10))
        with self.assertRaises(OverflowError):
            self.Test.callShort(long(-1e10))
