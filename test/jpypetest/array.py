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
import jpype
from jpype import JPackage, JArray, JByte, java
import unittest, common

VALUES = (12234,1234,234,1324,424,234,234,142,5,251,242,35,235,62,1235,46,245132,51, 2, 3, 4)

def suite() :
    return unittest.makeSuite(ArrayTestCase)

class ArrayTestCase(common.JPypeTestCase) :
    def __arrayEquals(self, a1, a2) :
        assert len(a1) == len(a2)
        
        for i in range(len(a1)) :
            assert a1[i] == a2[i]
    
    def testReadArray(self) :
        t = JPackage("jpype").array.TestArray()
        assert not isinstance(t, JPackage)
        
        self.__arrayEquals(VALUES, t.i)
        
        assert t.i[0] == VALUES[0]
        
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
        assert t.i[0] == 32
        
        t.i[1:3] = (33, 34)
        assert t.i[1] == 33   
        assert t.i[2] == 34
        
        self.__arrayEquals(t.i[:5], (32, 33, 34 ,1324, 424) )
        
    def testObjectArraySimple(self) :
        a = JArray(java.lang.String, 1)(2)
        a[1] = "Foo"
        assert "Foo" == a[1]
        
    def testByteArraySimple(self) :
        a = JArray(JByte)(2)
        a[1] = 2
        assert a[1] == 2
        
    def testIterateArray(self):
        t = JPackage("jpype").array.TestArray()
        assert not isinstance(t, JPackage)

        for i in t.i :
            assert i != 0 

    def testGetSubclass(self) :
        t = JPackage("jpype").array.TestArray()
        v = t.getSubClassArray()
        
        assert isinstance(v[0], unicode)
        
    def testGetArrayAsObject(self) :
        t = JPackage("jpype").array.TestArray()
        v = t.getArrayAsObject()        

    def testCharArrayAsString(self) :
        t = JPackage("jpype").array.TestArray()
        v = t.charArray
        assert str(v) == 'avcd'
        assert unicode(v) == u'avcd'
        
    def testByteArrayAsString(self) :
        t = JPackage("jpype").array.TestArray()
        v = t.byteArray
        assert str(v) == 'avcd'
        
    def testByteArrayIntoVector(self):
    	ba = jpype.JArray(jpype.JByte)('123')
    	v = jpype.java.util.Vector(1)
    	v.add(ba)
    	assert len(v) == 1
    	assert v[0] is not None

if __name__ == '__main__' :
    import os.path
    root = os.path.abspath(os.path.dirname(__file__))    
    common.CLASSPATH= "%s/../../build/test/java" % root
    
    runner = unittest.TextTestRunner()
    runner.run(suite())

