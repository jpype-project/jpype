#*****************************************************************************
#   Copyright 2017 Karl Einar Nelson
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
try:
    import unittest2 as unittest
except ImportError:
    import unittest
import sys
import jpype
import jpype._jboxed
from . import common

#Python2/3 support
if sys.version > '3':
    long  =  int
    unicode = str

# Test code
class BoxedTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Boxed = jpype.JClass('jpype.boxed.Boxed')
        self.Object = jpype.JClass('java.lang.Object')
        self.Number = jpype.JClass('java.lang.Number')
        self.Comparable = jpype.JClass('java.lang.Comparable')
        self.Short = jpype.JClass('java.lang.Short')
        self.Integer = jpype.JClass('java.lang.Integer')
        self.Long = jpype.JClass('java.lang.Long')
        self.Float = jpype.JClass('java.lang.Float')
        self.Double = jpype.JClass('java.lang.Double')

    def testShort(self):
        c1=12345
        # Check passed from and passed to
        d1=self.Boxed.newShort(c1)
        d2=self.Short(c1)
        self.assertEqual(d1,c1)
        self.assertEqual(d2,c1)
        self.assertEqual(d1,d2)
        self.assertEqual(self.Boxed.callShort(c1), self.Boxed.callShort(d2))
        # Verify ops
        self.assertEqual(d1+2,d1+2)
        self.assertEqual(d1*2,d1*2)

    def testInteger(self):
        c1=12345
        # Check passed from and passed to
        d1=self.Boxed.newInteger(c1)
        d2=self.Integer(c1)
        self.assertEqual(d1,c1)
        self.assertEqual(d2,c1)
        self.assertEqual(d1,d2)
        self.assertEqual(self.Boxed.callInteger(c1), self.Boxed.callInteger(d2))
        # Verify ops
        self.assertEqual(d1+2,d1+2)
        self.assertEqual(d1*2,d1*2)

    def testLong(self):
        c1=12345
        # Check passed from and passed to
        d1=self.Boxed.newLong(c1)
        d2=self.Long(c1)
        self.assertEqual(d1,c1)
        self.assertEqual(d2,c1)
        self.assertEqual(d1,d2)
        self.assertEqual(self.Boxed.callLong(c1), self.Boxed.callLong(d2))
        # Verify ops
        self.assertEqual(d1+2,d1+2)
        self.assertEqual(d1*2,d1*2)

    def testDoubleFromFloat(self):
        self.Double(1.0)

    def testFloatFromInt(self):
        self.Float(1)

    def testDoubleFromInt(self):
        self.Double(1)

    def testBoxed2(self):
        self.Short(self.Integer(1))
        self.Integer(self.Integer(1))
        self.Long(self.Integer(1))
        self.Float(self.Integer(1))
        self.Float(self.Long(1))
        self.Double(self.Integer(1))
        self.Double(self.Long(1))
        self.Double(self.Float(1))

    def testFloat(self):
        c1=123124/256.0
        # Check passed from and passed to
        d1=self.Boxed.newFloat(c1)
        d2=self.Float(c1)
        self.assertEqual(d1,c1)
        self.assertEqual(d2,c1)
        self.assertEqual(d1,d2)
        self.assertEqual(self.Boxed.callFloat(c1), self.Boxed.callFloat(d2))
        # Verify ops
        self.assertEqual(d1+2,d1+2)
        self.assertEqual(d1*2,d1*2)
        self.assertTrue(d2<c1+1)
        self.assertTrue(d2>c1-1)

    def testDouble(self):
        c1=123124/256.0
        # Check passed from and passed to
        d1=self.Boxed.newDouble(c1)
        d2=self.Double(c1)
        self.assertEqual(d1,c1)
        self.assertEqual(d2,c1)
        self.assertEqual(d1,d2)
        self.assertEqual(self.Boxed.callDouble(c1), self.Boxed.callDouble(d2))
        # Verify ops
        self.assertEqual(d1+2,d1+2)
        self.assertEqual(d1*2,d1*2)
        self.assertTrue(d2<c1+1)
        self.assertTrue(d2>c1-1)

    def testShortResolve(self):
        self.assertEqual(self.Boxed.whichShort(1), 1)
        self.assertEqual(self.Boxed.whichShort(self.Short(1)), 2)

    def testIntegerResolve(self):
        self.assertEqual(self.Boxed.whichInteger(1), 1)
        self.assertEqual(self.Boxed.whichInteger(self.Integer(1)), 2)

    def testLongResolve(self):
        self.assertEqual(self.Boxed.whichLong(1), 1)
        self.assertEqual(self.Boxed.whichLong(self.Long(1)), 2)
 
    def testFloatResolve(self):
        self.assertEqual(self.Boxed.whichFloat(1.0), 1)
        self.assertEqual(self.Boxed.whichFloat(self.Float(1.0)), 2)

    def testDoubleResolve(self):
        self.assertEqual(self.Boxed.whichDouble(1.0), 1)
        self.assertEqual(self.Boxed.whichDouble(self.Double(1.0)), 2)

if __name__=='__main__':
    jpype.startJVM(jpype.getDefaultJVMPath())
    unittest.main()
