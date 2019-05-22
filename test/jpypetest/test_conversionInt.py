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


class ConversionIntTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Test = jpype.JClass("jpype.conversion.Test")

    def testIntFromInt(self):
        self.assertEquals(self.Test.callInt(int(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntFromNPInt(self):
        import numpy as np
        self.assertEquals(self.Test.callInt(np.int(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntFromNPInt8(self):
        import numpy as np
        self.assertEquals(self.Test.callInt(np.int8(123)), 123)
        self.assertEquals(self.Test.callInt(np.uint8(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntFromNPInt16(self):
        import numpy as np
        self.assertEquals(self.Test.callInt(np.int16(123)), 123)
        self.assertEquals(self.Test.callInt(np.uint16(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntFromNPInt32(self):
        import numpy as np
        self.assertEquals(self.Test.callInt(np.int32(123)), 123)
        self.assertEquals(self.Test.callInt(np.uint32(123)), 123)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntFromNPInt64(self):
        import numpy as np
        self.assertEquals(self.Test.callInt(np.int64(123)), 123)
        self.assertEquals(self.Test.callInt(np.uint64(123)), 123)

    def testIntFromFloat(self):
        with self.assertRaises(RuntimeError):
            self.Test.callInt(float(2))

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntFromNPFloat(self):
        import numpy as np
        with self.assertRaises(RuntimeError):
            self.Test.callInt(np.float(2))

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntFromNPFloat32(self):
        import numpy as np
        with self.assertRaises(RuntimeError):
            self.Test.callInt(np.float32(2))

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testIntFromNPFloat64(self):
        import numpy as np
        with self.assertRaises(RuntimeError):
            self.Test.callInt(np.float64(2))

    def testIntRange(self):
        with self.assertRaises(OverflowError):
            self.Test.callInt(long(1e10))
        with self.assertRaises(OverflowError):
            self.Test.callInt(long(-1e10))
