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


class OverloadTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.__jp = self.jpype.overloads
        self._aclass = JClass('jpype.overloads.Test1$A')
        self._bclass = JClass('jpype.overloads.Test1$B')
        self._cclass = JClass('jpype.overloads.Test1$C')
        self._a = self._aclass()
        self._b = self._bclass()
        self._c = self._cclass()
        self._i1impl = JClass('jpype.overloads.Test1$I1Impl')()
        self._i2impl = JClass('jpype.overloads.Test1$I2Impl')()
        self._i3impl = JClass('jpype.overloads.Test1$I3Impl')()
        self._i4impl = JClass('jpype.overloads.Test1$I4Impl')()
        self._i5impl = JClass('jpype.overloads.Test1$I5Impl')()
        self._i6impl = JClass('jpype.overloads.Test1$I6Impl')()
        self._i7impl = JClass('jpype.overloads.Test1$I7Impl')()
        self._i8impl = JClass('jpype.overloads.Test1$I8Impl')()

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
            JObject(self._c, 'jpype.overloads.Test1$A')))
        self.assertEqual('B', test1.testMostSpecific(
            JObject(self._c, 'jpype.overloads.Test1$B')))
        # JObject wrapper forces exact matches
        #self.assertRaisesRegex(TypeError, 'No matching overloads found', test1.testMostSpecific, JObject(self._c, 'jpype.overloads.Test1$C'))

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
        intexpectation = 'int' if not sys.version_info[0] > 2 and sys.maxint == 2**31 - 1 else 'long'
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
            JObject(self._i4impl, 'jpype.overloads.Test1$I2')))
        self.assertEqual('I3', test1.testInterfaces1(
            JObject(self._i4impl, 'jpype.overloads.Test1$I3')))

    def testInterfaces2(self):
        test1 = self.__jp.Test1()
        self.assertEqual('I4', test1.testInterfaces2(self._i4impl))
        self.assertEqual('I2', test1.testInterfaces2(
            JObject(self._i4impl, 'jpype.overloads.Test1$I2')))
        self.assertEqual('I3', test1.testInterfaces2(
            JObject(self._i4impl, 'jpype.overloads.Test1$I3')))

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
            JClass('jpype.overloads.Test1$I4Impl')))
        self.assertEqual('Class', test1.testClassVsObject(
            JClass('jpype.overloads.Test1$I3')))

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
            testdefault = JClass('jpype.overloads.Test1$DefaultC')()
        except:
            pass
        else:
            self.assertEqual('B', testdefault.defaultMethod())
