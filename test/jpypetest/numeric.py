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
from jpype import JPackage, java, JFloat
from . import common

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
