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


class ConversionShortTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Test = jpype.JClass("jpype.common.Fixture")()

    def testShortFromInt(self):
        self.assertEqual(self.Test.callShort(int(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPInt(self):
        import numpy as np
        self.assertEqual(self.Test.callShort(np.int(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPInt8(self):
        import numpy as np
        self.assertEqual(self.Test.callShort(np.int8(123)), 123)
        self.assertEqual(self.Test.callShort(np.uint8(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPInt16(self):
        import numpy as np
        self.assertEqual(self.Test.callShort(np.int16(123)), 123)
        self.assertEqual(self.Test.callShort(np.uint16(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPInt32(self):
        import numpy as np
        self.assertEqual(self.Test.callShort(np.int32(123)), 123)
        self.assertEqual(self.Test.callShort(np.uint32(123)), 123)

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPInt64(self):
        import numpy as np
        self.assertEqual(self.Test.callShort(np.int64(123)), 123)
        self.assertEqual(self.Test.callShort(np.uint64(123)), 123)

    def testShortFromFloat(self):
        with self.assertRaises(TypeError):
            self.Test.callShort(float(2))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPFloat(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callShort(np.float(2))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPFloat32(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callShort(np.float32(2))

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testShortFromNPFloat64(self):
        import numpy as np
        with self.assertRaises(TypeError):
            self.Test.callShort(np.float64(2))

    def testShortRange(self):
        with self.assertRaises(OverflowError):
            self.Test.callShort(int(1e10))
        with self.assertRaises(OverflowError):
            self.Test.callShort(int(-1e10))

    def testShortFromNone(self):
        with self.assertRaises(TypeError):
            self.Test.callShort(None)
