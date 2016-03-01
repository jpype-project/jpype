#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
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
#*****************************************************************************
from jpype import JPackage, java, JFloat, JByte, JShort, JInt, JLong
from . import common
import sys

class NumericTestCase(common.JPypeTestCase):
    def testMathAbs(self):
        self.assertEqual(java.lang.Math.abs(-10), 10)

    def testDoubleConversion(self):
        f = java.lang.Float.MAX_VALUE * 2
        jpype = JPackage("jpype")
        self.assertTrue(jpype.numeric.NumericTest.doubleIsTwiceMaxFloat(f))

    def testDoubleIsProperlyConverted(self):
        self.assertTrue(java.lang.Double.POSITIVE_INFINITY != 0.0)
        self.assertTrue(java.lang.Double.MAX_VALUE != 0.0)
        self.assertTrue(java.lang.Double.NaN != 0.0)
        self.assertTrue(java.lang.Double.NEGATIVE_INFINITY != 0.0)

    def testNegativeJFloatWrapper(self):
        f = JFloat(-1)

    def checkJWrapper(self, min_value, max_value, javawrapper, jwrapper, expected=TypeError):
        self.assertEqual(max_value, javawrapper(max_value).longValue())
        f = jwrapper(max_value)
        self.assertEqual(max_value, javawrapper(f).longValue())

        self.assertEqual(min_value, javawrapper(min_value).longValue())
        f = jwrapper(min_value)
        self.assertEqual(min_value, javawrapper(f).longValue())
        
        self.assertRaises(expected, javawrapper, max_value+1)
        self.assertRaises(expected, jwrapper, max_value+1)
        self.assertRaises(expected, javawrapper, min_value-1)
        self.assertRaises(expected, jwrapper, min_value-1)

        # test against int overflow
        if(max_value < 2**32):
            self.assertRaises(expected, javawrapper, 2**32)
            self.assertRaises(expected, jwrapper, 2**32)

    def testJCharWrapper(self):
        self.checkJWrapper(-2**7, 2**7-1, java.lang.Byte, JByte)

    def testJByteWrapper(self):
        self.checkJWrapper(-2**7, 2**7-1, java.lang.Byte, JByte)

    def testJShortWrapper(self):
        self.checkJWrapper(-2**15, 2**15-1, java.lang.Short, JShort)

    def testJIntWrapper(self):
        self.checkJWrapper(-2**31, 2**31-1, java.lang.Integer, JInt)

    def testJLongWrapper(self):
        self.checkJWrapper(-2**63, 2**63-1, java.lang.Long, JLong, OverflowError)
        
    def testJFloatWrapper(self):
        jwrapper = JFloat
        javawrapper = java.lang.Float
        jwrapper(float(2**127))
        javawrapper(float(2**127))
        self.assertRaises(TypeError, jwrapper, float(2**128))
        self.assertRaisesRegexp(RuntimeError, 'No matching overloads found', javawrapper, 5) # no conversion from int?

        # this difference might be undesirable, 
        # a double bigger than maxfloat passed to java.lang.Float turns into infinity 
        self.assertEquals(float('inf'), javawrapper(float(2**128)).doubleValue())
        self.assertEquals(float('-inf'), javawrapper(float(-2**128)).doubleValue())
                
