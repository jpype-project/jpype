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
import unittest2 as unittest
import jpype
from jpype import JPackage, JArray, JByte, java
import common

VALUES = [12234,1234,234,1324,424,234,234,142,5,251,242,35,235,62,1235,46,245132,51, 2, 3, 4]

class ArrayTestCase(common.JPypeTestCase) :
    def __arrayEquals(self, a1, a2) :
        self.assertEqual(len(a1), len(a2))
        
        for i in range(len(a1)) :
            self.assertEqual(a1[i], a2[i])
    
    def testReadArray(self) :
        t = JPackage("jpype").array.TestArray()
        assert not isinstance(t, JPackage)
        
        self.__arrayEquals(VALUES, t.i)
        
        self.assertEqual(t.i[0], VALUES[0])
        self.__arrayEquals(VALUES[1:-2], t.i[1:-2])

    def testStangeBehavior(self) :
        ''' Test for stange crash reported in bug #1089302'''
        Test2 = jpype.JPackage('jpype.array').Test2
        test = Test2()
        test.test(test.getValue())

    def testWriteArray(self) :
        t = JPackage("jpype").array.TestArray()
        assert not isinstance(t, JPackage)

        t.i[0] = 32
        self.assertEqual(t.i[0], 32)
        
        t.i[1:3] = (33, 34)
        self.assertEqual(t.i[1], 33   )
        self.assertEqual(t.i[2], 34)
        
        self.__arrayEquals(t.i[:5], (32, 33, 34 ,1324, 424) )
        
    def testObjectArraySimple(self) :
        a = JArray(java.lang.String, 1)(2)
        a[1] = "Foo"
        self.assertEqual("Foo", a[1])
        
    def testByteArraySimple(self) :
        a = JArray(JByte)(2)
        a[1] = 2
        self.assertEqual(a[1], 2)
        
    def testIterateArray(self):
        t = JPackage("jpype").array.TestArray()
        self.assertFalse(isinstance(t, JPackage))

        for i in t.i :
            self.assertNotEqual(i, 0)

    def testGetSubclass(self) :
        t = JPackage("jpype").array.TestArray()
        v = t.getSubClassArray()
        
        self.assertTrue(isinstance(v[0], unicode))
        
    def testGetArrayAsObject(self) :
        t = JPackage("jpype").array.TestArray()
        v = t.getArrayAsObject()     

    def testCharArrayAsString(self) :
        t = JPackage("jpype").array.TestArray()
        v = t.charArray
        self.assertEqual(str(v), 'avcd')
        self.assertEqual(unicode(v), u'avcd')
        
    def testByteArrayAsString(self) :
        t = JPackage("jpype").array.TestArray()
        v = t.byteArray
        self.assertEqual(str(v), 'avcd')
        
    def testByteArrayIntoVector(self):
        ba = jpype.JArray(jpype.JByte)('123')
        v = jpype.java.util.Vector(1)
        v.add(ba)
        self.assertEqual(len(v), 1)
        self.assertNotEqual(v[0], None)
        
    def testJArrayConversionBool(self):
        expected = [True, False, False, True]
        jarr = jpype.JArray(jpype.JBoolean)(expected)
        
        self.assertEqual(expected, jarr[:])

    def testJArrayConversionChar(self):
        t = JPackage("jpype").array.TestArray()
        v = t.charArray
        self.assertEqual(str(v[:]), 'avcd')
        self.assertEqual(unicode(v[:]), u'avcd')
        
    def testJArrayConversionByte(self):
        expected = (0,1,2,3)
        ByteBuffer = jpype.java.nio.ByteBuffer
        bb = ByteBuffer.allocate(4)
        buf = bb.array()
        print "type: ", type(buf)
        for i in xrange(len(expected)):
            buf[i] = expected[i]
        
        self.assertEqual(expected[:], buf[:])

    @unittest.expectedFailure
    def testJArrayConversionShort(self):
        # TODO: think about impl short (JShort not available)
        jarr = jpype.JArray(jpype.JShort)(VALUES)
        result = jarr[0 : len(jarr)]
        self.assertEqual(VALUES, result)
        
        result = jarr[2:10]
        self.assertEqual(VALUES[2:10], result)
        
    def testJArrayConversionInt(self):
        jarr = jpype.JArray(jpype.JInt)(VALUES)
        result = jarr[0 : len(jarr)]
        self.assertEqual(VALUES, result)
        
        result = jarr[2:10]
        self.assertEqual(VALUES[2:10], result)
    
    def testJArrayConversionLong(self):
        jarr = jpype.JArray(jpype.JLong)(VALUES)
        result = jarr[0 : len(jarr)]
        self.assertEqual(VALUES, result)
        
        result = jarr[2:10]
        self.assertEqual(VALUES[2:10], result)
        
    def testJArrayConversionFloat(self):
        jarr = jpype.JArray(jpype.JFloat)(VALUES)
        result = jarr[0 : len(jarr)]
        self.assertEqual(VALUES, result)
        
        result = jarr[2:10]
        self.assertEqual(VALUES[2:10], result)
        
    def testJArrayConversionDouble(self):
        jarr = jpype.JArray(jpype.JDouble)(VALUES)
        result = jarr[0 : len(jarr)]
        self.assertEqual(VALUES, result)
        
        result = jarr[2:10]
        self.assertEqual(VALUES[2:10], result)
        
    def testConversionError(self):
        jarr = jpype.JArray(jpype.JInt, 1)(10)
        with self.assertRaises(RuntimeError):
            jarr[1:2] = [dict()]
        
        # -1 is returned by python, if conversion fails also, ensure this works
        jarr[1:2] = [-1]
        
    def testObjectArrayInitial(self):
        l1 = java.util.ArrayList()
        l1.add(0)
        l2 = java.util.ArrayList()
        l2.add(42)
        l3 = java.util.ArrayList()
        l3.add(13)
        jarr = jpype.JArray(java.util.ArrayList, 1)([l1, l2, l3])
        
        self.assertEqual(l1, jarr[0])
        self.assertEqual(l2, jarr[1])
        self.assertEqual(l3, jarr[2])
