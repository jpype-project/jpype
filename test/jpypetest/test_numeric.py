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
from jpype import JPackage, java
from jpype.types import *
import common


class NumericTestCase(common.JPypeTestCase):
    def testMathAbs(self):
        self.assertEqual(java.lang.Math.abs(-10), 10)

    def testDoubleConversion(self):
        f = java.lang.Float.MAX_VALUE * 2
        self.assertTrue(JClass("jpype.numeric.NumericTest").doubleIsTwiceMaxFloat(f))

    def testDoubleIsProperlyConverted(self):
        self.assertTrue(java.lang.Double.POSITIVE_INFINITY != 0.0)
        self.assertTrue(java.lang.Double.MAX_VALUE != 0.0)
        self.assertTrue(java.lang.Double.NaN != 0.0)
        self.assertTrue(java.lang.Double.NEGATIVE_INFINITY != 0.0)

    def testCompareNullLong(self):
        null = JObject(None, java.lang.Integer)
        self.assertEqual(null, None)
        self.assertNotEqual(null, object())
        with self.assertRaisesRegex(TypeError, "null"):
            null > 0
        with self.assertRaisesRegex(TypeError, "null"):
            null < 0
        with self.assertRaisesRegex(TypeError, "null"):
            null >= 0
        with self.assertRaisesRegex(TypeError, "null"):
            null <= 0

    def testCompareNullFloat(self):
        null = JObject(None, java.lang.Double)
        self.assertEqual(null, None)
        self.assertNotEqual(null, object())
        with self.assertRaisesRegex(TypeError, "null"):
            null > 0
        with self.assertRaisesRegex(TypeError, "null"):
            null < 0
        with self.assertRaisesRegex(TypeError, "null"):
            null >= 0
        with self.assertRaisesRegex(TypeError, "null"):
            null <= 0

    def testHashNull(self):
        null = JObject(None, java.lang.Integer)
        self.assertEqual(hash(null), hash(None))
        null = JObject(None, java.lang.Float)
        self.assertEqual(hash(null), hash(None))

    @common.requireInstrumentation
    def testJPNumber_new(self):
        _jpype.fault("PyJPNumber_new")

        class MyNum(_jpype._JNumberLong, internal=True):
            pass
        with self.assertRaisesRegex(SystemError, "fault"):
            JInt(1)
        with self.assertRaises(TypeError):
            MyNum(1)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            JInt(1)
        JInt(1)

    @common.requireInstrumentation
    def testJPNumberLong_int(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_int")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            int(ji)
        int(ji)

    @common.requireInstrumentation
    def testJPNumberLong_float(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_float")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            float(ji)
        float(ji)

    @common.requireInstrumentation
    def testJPNumberLong_str(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(ji)
        str(ji)

    @common.requireInstrumentation
    def testJPNumberLong_repr(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_repr")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(ji)
        repr(ji)

    @common.requireInstrumentation
    def testJPNumberLong_compare(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_compare")
        with self.assertRaisesRegex(SystemError, "fault"):
            ji == 1
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            ji == 1
        ji == 1

    @common.requireInstrumentation
    def testJPNumberLong_hash(self):
        ji = JInt(1)
        _jpype.fault("PyJPNumberLong_hash")
        with self.assertRaises(SystemError):
            hash(ji)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaises(SystemError):
            hash(ji)
        hash(ji)
