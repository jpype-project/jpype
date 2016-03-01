#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
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
#*****************************************************************************
import jpype
from jpype import JString, java, JArray
import sys
import time
from . import common

class AttributeTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.__jp = self.jpype.attr

    def testWithBufferStrategy(self):
        j = jpype.JPackage("jpype").attr.ClassWithBuffer
        self.assertIsNone(j().bufferStrategy)

    def testCallOverloadedMethodWithCovariance(self):
        # This is a JDk5-specific problem.
        h = jpype.java.lang.StringBuffer()
        h.delete(0, 0)

    def testCallStaticString(self):
        h = self.__jp.Test1()
        v = h.testString(JString("abcd"), JString("abcde"))

        self.assertEqual(v[0], 'abcd')
        self.assertEqual(v[1], 'abcde')

    def testCallStaticUnicodeString(self):
        h = self.__jp.Test1()
        v = h.testString(JString(u"abcd"), JString(u"abcde"))

        self.assertEqual(v[0], 'abcd')
        self.assertEqual(v[1], 'abcde')

    def testCallString(self):
        v = self.__jp.Test1.testStaticString("a", "b")
        self.assertEqual(v[0], 'a')
        self.assertEqual(v[1], 'b')

    def testCallUnicodeString(self):
        v = self.__jp.Test1.testStaticString(u"a", u"b")
        self.assertEqual(v[0], 'a')
        self.assertEqual(v[1], 'b')

    def testCallStringWithNone(self):
        v = self.__jp.Test1.testStaticString("a", None)
        self.assertEqual(v[0], 'a')
        self.assertIsNone(v[1])

    def testWithHolder(self):
        holder = self.__jp.Holder()
        holder.f = "ffff"
        self.assertEqual(holder.f, 'ffff')
        result = self.__jp.Test1.testStaticHolder(holder)
        self.assertEqual(result, 'ffff')

    def testWithSubHolder(self):
        h2 = self.__jp.SubHolder()
        h2.f = "subholder"
        result = self.__jp.Test1.testStaticHolder(h2)
        self.assertEqual(result, 'subholder')

    def testCallWithArray(self):
        h2 = self.__jp.Test1()
        StringArray = JArray(JString)
        v = StringArray(["Foo", "bar"])
        t = self.__jp.Test1()
        result = t.testStringArray(v)
        self.assertSequenceEqual(["Foo", "bar"], result)

    def testGetStaticValue(self):
        self.assertEqual(str(self.__jp.Test1.objectValue), "234")

    def testGetStaticByInstance(self):
        h = self.__jp.Test1()
        self.assertEqual(str(h.objectValue), "234")

    def testGetNonStatic(self):
        h = self.__jp.Test1()
        self.assertEqual(h.stringValue, "Foo")

    def testSetStaticValue(self):
        self.__jp.Test1.objectValue = java.lang.Integer(43)
        self.assertEqual(str(self.__jp.Test1.objectValue), "43")
        self.__jp.Test1.reset()

    def testSetNonStaticValue(self):
        h = self.__jp.Test1()
        h.stringValue="bar"
        self.assertEqual(h.stringValue, "bar")

    def testReturnSubClass(self):
        h = self.__jp.Test1()
        v = h.getSubClass()
        self.assertIsInstance(v, self.__jp.SubHolder)

    def testCallWithClass(self):
        h = self.__jp.Test1()
        h.callWithClass(java.lang.Comparable)

    def testCallSuperclassMethod(self):
        h = self.__jp.Test2()
        h.test2Method()
        h.test1Method()

    def testCallWithLong(self):
        h = self.__jp.Test1()
        if sys.version > '3':
            l = int(123)
        else:
            l = long(123)

        h.setByte(l)
        self.assertEqual(l, h.mByteValue)
        h.setShort(l)
        self.assertEqual(l, h.mShortValue)
        h.setInt(l)
        self.assertEqual(l, h.mIntValue)
        h.setLong(l)
        self.assertEqual(l, h.mLongValue)

    def testCallWithBigLong(self):
        h = self.__jp.Test1()
        if sys.version > '3':
            l = int(4398046511103)
        else:
            l = long(4398046511103)
 
        self.assertRaises(TypeError, h.setByte, l)
        self.assertRaises(TypeError, h.setShort, l)
        self.assertRaises(TypeError, h.setInt, l)
        h.setLong(l)
        self.assertEqual(l, h.mLongValue)

    def testCallWithBigInt(self):
        h = self.__jp.Test1()
        if sys.version > '3' or sys.maxint > 2**31:
            l = int(4398046511103)
        else:
            l = long(4398046511103)
 
        self.assertRaises(TypeError, h.setByte, l)
        self.assertRaises(TypeError, h.setShort, l)
        self.assertRaises(TypeError, h.setInt, l)
        h.setLong(l)
        self.assertEqual(l, h.mLongValue)

    def testSetBoolean(self):
        h = self.__jp.Test1()
        self.assertEqual(False, h.mBooleanValue)
        h.setBoolean(True)
        self.assertEqual(True, h.mBooleanValue)
        h.setBoolean(False)
        self.assertEqual(False, h.mBooleanValue)
        # just testing the status quo, not sure about if this is nice
        h.setBoolean(42)
        self.assertEqual(True, h.mBooleanValue)
        h.setBoolean(0)
        self.assertEqual(False, h.mBooleanValue)
        if sys.version > '3':
            l = int(4398046511103)
        else:
            l = long(4398046511103)
        h.setBoolean(l)
        self.assertEqual(True, h.mBooleanValue)
        if sys.version > '3':
            l = int(0)
        else:
            l = long(0)
        h.setBoolean(l)
        self.assertEqual(False, h.mBooleanValue)

    def testCreateDate(self):
        d = jpype.java.util.Date(1448799485000)
        self.assertEqual(1448799485000, d.getTime())
        
    def testCharAttribute(self):
        h = self.__jp.Test1()
        h.charValue = u'b'

        self.assertEqual(h.charValue, 'b')
        if sys.version < '3':
            exp_repr = "u'b'"
        else:
            exp_repr = "'b'"
        self.assertEqual(repr(h.charValue), exp_repr)

    def testGetPrimitiveType(self):
        Integer = jpype.JClass("java.lang.Integer")
        intType = Integer.TYPE

    def testDifferentiateClassAndObject(self):
        h = self.__jp.Test1()

        self.assertEqual(h.callWithSomething(self.__jp.Test1), u"Class")
        result = h.callWithSomething(jpype.JObject(self.__jp.Test1,
                                                   jpype.java.lang.Object))
        self.assertEqual(result, u"Object")

    def testToString(self):
        h = self.__jp.Test1()
        self.assertEqual(str(h), 'aaa')

    def testSuperToString(self):
        h = self.__jp.Test2()
        self.assertEqual(str(h), 'aaa')

#       def testStringToConversion(self):
#               try:
#                       jpype.ConversionConfig.string = False
#                       for i in range(1):
#                               h = self.__jp.Test1()
#
#                               start = time.time();
#                               for j in range(10):
#                                       ts = h.getBigString()
#                               stop = time.time()
#                               print ts.__class__, (stop-start), 'ms'
#
#                               # for comparison ... convert a string to JStrng and back
#                               s = "".join([" "]*1024*1024*5)
#                               start = time.time()
#                               js = JString(s)
#                               stop = time.time()
#                               print "converted to JString in", (stop-start), 'ms', len(s)
#
#                               start = time.time()
#                               cs = str(JString(s))
#                               print cs
#                               print "converted to String in", (stop-start), 'ms', len(cs), cs
#               finally:
#                       jpype.ConversionConfig.string = True

    def testComplexMethodOvlerloading(self):
        c = self.__jp.TestOverloadC()
        self.assertEqual(c.foo(1), "foo(int) in C: 1")
        self.assertEqual(c.foo(), "foo() in A")

    def testPassedObjectGetsCleanedUp(self):
        h = self.__jp.Test1()
        block_size = 1024 * 1024 * 10
        def allocate_then_free():
            byte_buffer = jpype.JClass('java.nio.ByteBuffer')
            inst = byte_buffer.allocate(1024 * 1024 * 100)
             # passing the object back to java seems to stop it being collected
            result = h.callWithSomething(inst)
        rt = jpype.java.lang.Runtime.getRuntime()
        free = rt.freeMemory()
        for x in range(0, 10 * free // block_size):
            allocate_then_free()
        

    def testSyntheticMethod(self):
        h = jpype.JClass('jpype.attr.SyntheticMethods$GenericImpl')()
        h.foo(jpype.java.util.ArrayList())
        