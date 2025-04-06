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
from jpype import JString, java, JArray, JClass, JByte, JShort, JInt, JLong, JFloat, JDouble, JChar, JBoolean, JObject
import sys
import time
import common

java = jpype.java


class MyClass:
    def fun1(self):
        pass

    def fun2(self, *a):
        pass

    def fun3(self, a=1):
        pass

    def fun4(self, a):
        pass


def fun1():
    pass


def fun2(*a):
    pass


def fun3(a=1):
    pass


def fun4(a):
    pass


if __name__ == '__main__':
    jpype.startJVM()
    from java.lang import Runnable
    mc = MyClass()
    cb = Runnable @ fun1
    cb = Runnable @ fun2
    cb = Runnable @ fun3
    #cb = Runnable @ fun4
    cb = Runnable @ mc.fun1
    cb = Runnable @ mc.fun2
    cb = Runnable @ mc.fun3
    cb = Runnable @ mc.fun4


class OverloadTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.__jp = self.jpype.test.overloads
        self._aclass = JClass('org.jpype.test.overloads.Test1$A')
        self._bclass = JClass('org.jpype.test.overloads.Test1$B')
        self._cclass = JClass('org.jpype.test.overloads.Test1$C')
        self._a = self._aclass()
        self._b = self._bclass()
        self._c = self._cclass()
        self._i1impl = JClass('org.jpype.test.overloads.Test1$I1Impl')()
        self._i2impl = JClass('org.jpype.test.overloads.Test1$I2Impl')()
        self._i3impl = JClass('org.jpype.test.overloads.Test1$I3Impl')()
        self._i4impl = JClass('org.jpype.test.overloads.Test1$I4Impl')()
        self._i5impl = JClass('org.jpype.test.overloads.Test1$I5Impl')()
        self._i6impl = JClass('org.jpype.test.overloads.Test1$I6Impl')()
        self._i7impl = JClass('org.jpype.test.overloads.Test1$I7Impl')()
        self._i8impl = JClass('org.jpype.test.overloads.Test1$I8Impl')()

    def testMostSpecificInstanceMethod(self):
        test1 = self.__jp.Test1()
        self.assertEqual('A', test1.testMostSpecific(self._a))
        self.assertEqual('B', test1.testMostSpecific(self._b))
        self.assertEqual('B', test1.testMostSpecific(self._c))
        self.assertEqual('B', test1.testMostSpecific(None))

    def testForceOverloadResolution(self):
        test1 = self.__jp.Test1()
        self.assertEqual('A', test1.testMostSpecific(
            JObject(self._c, self._aclass)))
        self.assertEqual('B', test1.testMostSpecific(
            JObject(self._c, self._bclass)))
        # JObject wrapper forces exact matches
        #self.assertRaisesRegex(TypeError, 'No matching overloads found', test1.testMostSpecific, JObject(self._c, self._cclass))
        self.assertEqual('A', test1.testMostSpecific(
            JObject(self._c, 'org.jpype.test.overloads.Test1$A')))
        self.assertEqual('B', test1.testMostSpecific(
            JObject(self._c, 'org.jpype.test.overloads.Test1$B')))
        # JObject wrapper forces exact matches
        #self.assertRaisesRegex(TypeError, 'No matching overloads found', test1.testMostSpecific, JObject(self._c, 'org.jpype.test.overloads.Test1$C'))

    def testVarArgsCall(self):
        test1 = self.__jp.Test1()
        self.assertEqual('A,B...', test1.testVarArgs(self._a, []))
        self.assertEqual('A,B...', test1.testVarArgs(self._a, None))
        self.assertEqual('A,B...', test1.testVarArgs(self._a, [self._b]))
        self.assertEqual('A,A...', test1.testVarArgs(self._a, [self._a]))
        self.assertEqual('A,B...', test1.testVarArgs(
            self._a, [self._b, self._b]))
        self.assertEqual('B,B...', test1.testVarArgs(
            self._b, [self._b, self._b]))
        self.assertEqual('B,B...', test1.testVarArgs(
            self._c, [self._c, self._b]))
        self.assertEqual('B,B...', test1.testVarArgs(
            self._c, JArray(self._cclass, 1)([self._c, self._c])))
        self.assertEqual('B,B...', test1.testVarArgs(self._c, None))
        self.assertEqual('B,B...', test1.testVarArgs(None, None))

    def testPrimitive(self):
        test1 = self.__jp.Test1()
        intexpectation = 'long'
        # FIXME it is not possible to determine if this is bool/char/byte currently
        #self.assertEqual(intexpectation, test1.testPrimitive(5))
        #self.assertEqual('long', test1.testPrimitive(2**31))
        self.assertEqual('byte', test1.testPrimitive(JByte(5)))
        self.assertEqual('Byte', test1.testPrimitive(java.lang.Byte(5)))
        self.assertEqual('short', test1.testPrimitive(JShort(5)))
        self.assertEqual('Short', test1.testPrimitive(java.lang.Short(5)))
        self.assertEqual('int', test1.testPrimitive(JInt(5)))
        self.assertEqual('Integer', test1.testPrimitive(java.lang.Integer(5)))
        self.assertEqual('long', test1.testPrimitive(JLong(5)))
        self.assertEqual('Long', test1.testPrimitive(java.lang.Long(5)))
        self.assertEqual('float', test1.testPrimitive(JFloat(5)))
        self.assertEqual('Float', test1.testPrimitive(java.lang.Float(5.0)))
        self.assertEqual('double', test1.testPrimitive(JDouble(5)))
        self.assertEqual('Double', test1.testPrimitive(java.lang.Double(5.0)))
        self.assertEqual('boolean', test1.testPrimitive(JBoolean(5)))
        self.assertEqual('Boolean', test1.testPrimitive(java.lang.Boolean(5)))
        self.assertEqual('char', test1.testPrimitive(JChar('5')))
        self.assertEqual('Character', test1.testPrimitive(
            java.lang.Character('5')))

    def testInstanceVsClassMethod(self):
        # this behaviour is different than the one in java, so maybe we should change it?
        test1 = self.__jp.Test1()
        self.assertEqual(
            'static B', self.__jp.Test1.testInstanceVsClass(self._c))
        self.assertEqual('instance A', test1.testInstanceVsClass(self._c))
        # here what would the above resolve to in java
        self.assertEqual(
            'static B', self.__jp.Test1.testJavaInstanceVsClass())

    def testInterfaces1(self):
        test1 = self.__jp.Test1()
        self.assertRaisesRegex(
            TypeError, 'Ambiguous overloads found', test1.testInterfaces1, self._i4impl)
        self.assertEqual('I2', test1.testInterfaces1(
            JObject(self._i4impl, 'org.jpype.test.overloads.Test1$I2')))
        self.assertEqual('I3', test1.testInterfaces1(
            JObject(self._i4impl, 'org.jpype.test.overloads.Test1$I3')))

    def testInterfaces2(self):
        test1 = self.__jp.Test1()
        self.assertEqual('I4', test1.testInterfaces2(self._i4impl))
        self.assertEqual('I2', test1.testInterfaces2(
            JObject(self._i4impl, 'org.jpype.test.overloads.Test1$I2')))
        self.assertEqual('I3', test1.testInterfaces2(
            JObject(self._i4impl, 'org.jpype.test.overloads.Test1$I3')))

    def testInterfaces3(self):
        test1 = self.__jp.Test1()
        self.assertRaisesRegex(
            TypeError, 'Ambiguous overloads found', test1.testInterfaces3, self._i8impl)
        self.assertEqual('I4', test1.testInterfaces3(self._i6impl))
        self.assertRaisesRegex(
            TypeError, 'No matching overloads found', test1.testInterfaces3, self._i3impl)

    def testInterfaces4(self):
        test1 = self.__jp.Test1()
        self.assertEqual('I8', test1.testInterfaces4(None))
        self.assertEqual('I1', test1.testInterfaces4(self._i1impl))
        self.assertEqual('I2', test1.testInterfaces4(self._i2impl))
        self.assertEqual('I3', test1.testInterfaces4(self._i3impl))
        self.assertEqual('I4', test1.testInterfaces4(self._i4impl))
        self.assertEqual('I5', test1.testInterfaces4(self._i5impl))
        self.assertEqual('I6', test1.testInterfaces4(self._i6impl))
        self.assertEqual('I7', test1.testInterfaces4(self._i7impl))
        self.assertEqual('I8', test1.testInterfaces4(self._i8impl))

    def testClassVsObject(self):
        test1 = self.__jp.Test1()
        self.assertEqual('Object', test1.testClassVsObject(self._i4impl))
        self.assertEqual('Object', test1.testClassVsObject(1))
        self.assertEqual('Class', test1.testClassVsObject(None))
        self.assertEqual('Class', test1.testClassVsObject(
            JClass('org.jpype.test.overloads.Test1$I4Impl')))
        self.assertEqual('Class', test1.testClassVsObject(
            JClass('org.jpype.test.overloads.Test1$I3')))

    def testStringArray(self):
        test1 = self.__jp.Test1()
        self.assertEqual('Object', test1.testStringArray(self._i4impl))
        self.assertEqual('Object', test1.testStringArray(1))
        self.assertRaisesRegex(
            TypeError, 'Ambiguous overloads found', test1.testStringArray, None)
        self.assertEqual('String', test1.testStringArray('somestring'))
        self.assertEqual('String[]', test1.testStringArray([]))
        self.assertEqual('String[]', test1.testStringArray(['a', 'b']))
        self.assertEqual('String[]', test1.testStringArray(
            JArray(java.lang.String, 1)(['a', 'b'])))
        self.assertEqual('Object', test1.testStringArray(
            JArray(JInt, 1)([1, 2])))

    def testListVSArray(self):
        test1 = self.__jp.Test1()
        self.assertEqual('String[]', test1.testListVSArray(['a', 'b']))
        self.assertEqual('List<String>', test1.testListVSArray(
            jpype.java.util.Arrays.asList(['a', 'b'])))

    def testDefaultMethods(self):
        try:
            testdefault = JClass('org.jpype.test.overloads.Test1$DefaultC')()
        except:
            pass
        else:
            self.assertEqual('B', testdefault.defaultMethod())

    def testFunctionalInterfacesWithDifferentSignatures(self):
        test2 = self.__jp.Test2()
        self.assertEqual('NoArgs', test2.testFunctionalInterfaces(lambda: 'NoArgs'))
        self.assertEqual('SingleArg', test2.testFunctionalInterfaces(lambda a: 'SingleArg'))
        self.assertEqual('TwoArg', test2.testFunctionalInterfaces(lambda a, b: 'TwoArg'))

    def testFunctionalInterfacesWithDefaults(self):
        def my_fun(x, y=None):
            return 'my_fun'

        def my_fun_vargs(x, *vargs):
            return 'my_fun_vargs'

        def my_fun_kwargs(x, **kwargs):
            return 'my_fun_kwargs'

        def my_fun_kw(*, keyword_arg=None):
            return 'my_fun_kw'

        class my_fun_class:
            def __call__(self):
                return 'my_fun_class'

        test2 = self.__jp.Test2()
        self.assertRaisesRegex(
            TypeError, 'Ambiguous overloads found', test2.testFunctionalInterfaces, my_fun)
        self.assertRaisesRegex(
            TypeError, 'Ambiguous overloads found', test2.testFunctionalInterfaces, my_fun_vargs)
        self.assertRaisesRegex(
            TypeError, 'Ambiguous overloads found', test2.testFunctionalInterfaces, my_fun_class)
        self.assertEqual('SingleArg', test2.testFunctionalInterfaces(my_fun_kwargs))
        self.assertEqual('NoArgs', test2.testFunctionalInterfaces(my_fun_kw))

    def testRunnable(self):
        Runnable = jpype.JClass("java.lang.Runnable")
        mc = MyClass()
        # These should work
        cb = Runnable @ fun1
        cb = Runnable @ fun2
        cb = Runnable @ fun3
        cb = Runnable @ mc.fun1
        cb = Runnable @ mc.fun2
        cb = Runnable @ mc.fun3
        # These should fail
        with self.assertRaises(TypeError):
            cb = Runnable @ fun4
        with self.assertRaises(TypeError):
            cb = Runnable @ mc.fun4

    def testDerivedStatic(self):
        Boolean = jpype.JClass("java.lang.Boolean")
        Object = jpype.JClass("java.lang.Object")
        DerivedTest = JClass("org.jpype.test.overloads.DerivedTest")
        Base = DerivedTest.Base()
        Derived = DerivedTest.Derived()
        self.assertEqual(DerivedTest.testStatic(True, Base), 1)
        self.assertEqual(DerivedTest.testStatic("that", Base), 2)
        self.assertEqual(DerivedTest.testStatic(True, Derived), 1)
        self.assertEqual(DerivedTest.testStatic("that", Derived), 2)
        self.assertEqual(DerivedTest.testStatic(jpype.JBoolean(True), Base), 1)
        self.assertEqual(DerivedTest.testStatic(jpype.JBoolean(True), Derived), 1)
        self.assertEqual(DerivedTest.testStatic(jpype.JObject(True, Boolean), Derived), 2)
        self.assertEqual(DerivedTest.testStatic(jpype.JObject(True, Object), Derived), 2)

    def testDerivedMember(self):
        Boolean = jpype.JClass("java.lang.Boolean")
        Object = jpype.JClass("java.lang.Object")
        DerivedTest = JClass("org.jpype.test.overloads.DerivedTest")
        Base = DerivedTest.Base()
        Derived = DerivedTest.Derived()
        obj = DerivedTest()
        self.assertEqual(obj.testMember(True, Base), 3)
        self.assertEqual(obj.testMember("that", Base), 4)
        self.assertEqual(obj.testMember(True, Derived), 3)
        self.assertEqual(obj.testMember("that", Derived), 4)
        self.assertEqual(obj.testMember(jpype.JBoolean(True), Base), 3)
        self.assertEqual(obj.testMember(jpype.JBoolean(True), Derived), 3)
        self.assertEqual(obj.testMember(jpype.JObject(True, Boolean), Derived), 4)
        self.assertEqual(obj.testMember(jpype.JObject(True, Object), Derived), 4)

    def testDerivedSub(self):
        Boolean = jpype.JClass("java.lang.Boolean")
        Object = jpype.JClass("java.lang.Object")
        DerivedTest = JClass("org.jpype.test.overloads.DerivedTest")
        Base = DerivedTest.Base()
        Derived = DerivedTest.Derived()
        obj = DerivedTest.Sub()
        self.assertEqual(obj.testMember(True, Base), 3)
        self.assertEqual(obj.testMember("that", Base), 4)
        self.assertEqual(obj.testMember(True, Derived), 3)
        self.assertEqual(obj.testMember("that", Derived), 4)
        self.assertEqual(obj.testMember(jpype.JBoolean(True), Base), 3)
        self.assertEqual(obj.testMember(jpype.JBoolean(True), Derived), 3)
        self.assertEqual(obj.testMember(jpype.JObject(True, Boolean), Derived), 4)
        self.assertEqual(obj.testMember(jpype.JObject(True, Object), Derived), 4)
