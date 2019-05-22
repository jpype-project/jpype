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


class ConversionBooleanTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Test = jpype.JClass("jpype.conversion.Test")

    def testBooleanFromInt(self):
        self.assertEqual(self.Test.callBoolean(int(123)), True)
        self.assertEqual(self.Test.callBoolean(int(0)), False)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanFromNPInt(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int(123)), True)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanFromNPInt8(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int8(123)), True)
        self.assertEqual(self.Test.callBoolean(np.uint8(123)), True)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanFromNPInt16(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int16(123)), True)
        self.assertEqual(self.Test.callBoolean(np.uint16(123)), True)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanFromNPInt32(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int32(123)), True)
        self.assertEqual(self.Test.callBoolean(np.uint32(123)), True)

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanFromNPInt64(self):
        import numpy as np
        self.assertEqual(self.Test.callBoolean(np.int64(123)), True)
        self.assertEqual(self.Test.callBoolean(np.uint64(123)), True)

    def testBooleanFromFloat(self):
        with self.assertRaises(RuntimeError):
            self.Test.callBoolean(float(2))

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanFromNPFloat(self):
        import numpy as np
        with self.assertRaises(RuntimeError):
            self.Test.callBoolean(np.float(2))

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanFromNPFloat32(self):
        import numpy as np
        with self.assertRaises(RuntimeError):
            self.Test.callBoolean(np.float32(2))

    @unittest.skipUnless(haveNumpy(), "numpy not available")
    def testBooleanFromNPFloat64(self):
        import numpy as np
        with self.assertRaises(RuntimeError):
            self.Test.callBoolean(np.float64(2))
