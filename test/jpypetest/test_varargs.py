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
import sys
import jpype
import common


def compareList(l1, l2):
    if len(l1) != len(l2):
        return False
    for i in range(0, len(l1)):
        if l1[i] != l2[i]:
            return False
    return True


class VarArgsTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.VarArgs = jpype.JClass('jpype.varargs.VarArgs')
        self.Object = jpype.JClass('java.lang.Object')
        self.ObjectA = jpype.JArray(self.Object)
        self.Integer = jpype.JClass('java.lang.Integer')
        self.String = jpype.JClass('java.lang.String')
        self.StringA = jpype.JArray(self.String)

    def testConflict(self):
        m = jpype.java.util.LinkedHashMap({"a": 1, "b": 2, "c": 3})
        test = self.VarArgs()
        self.assertEqual(test.conflict2([1, 2, 3]), 1)
        self.assertEqual(test.conflict2(m, m), 2)
        self.assertEqual(test.conflict3(['h', 'e']), 1)
        self.assertEqual(test.conflict3('h'), 2)
        self.assertEqual(test.conflict3('hello'), 2)
        self.assertEqual(test.conflict1(1), 1)
        self.assertEqual(test.conflict1([1, 2, 3]), 1)
        self.assertEqual(test.conflict1({"a": 1, "b": 2, "c": 3}), 2)
        self.assertEqual(test.conflict1(m), 2)

    def testVarArgsCtor(self):
        va0 = self.VarArgs('1')
        va1 = self.VarArgs('1', 'a')
        va2 = self.VarArgs('1', 'a', 'b')

        self.assertTrue(isinstance(va0.rest, self.ObjectA))
        self.assertTrue(isinstance(va1.rest, self.ObjectA))
        self.assertTrue(isinstance(va2.rest, self.ObjectA))
        self.assertEqual(len(va0.rest), 0)
        self.assertEqual(len(va1.rest), 1)
        self.assertEqual(len(va2.rest), 2)

    def testVarArgsMethod(self):
        va = self.VarArgs()
        a0 = va.method('a')
        a1 = va.method('a', 'b')
        a2 = va.method('a', 'b', 'c')

    def testVarArgsStatic(self):
        a0 = self.VarArgs.call()
        a1 = self.VarArgs.call(self.Object())
        a2 = self.VarArgs.call(self.Object(), self.Object())

        self.assertTrue(isinstance(a0, self.ObjectA))
        self.assertEqual(len(a0), 0)
        self.assertTrue(isinstance(a1, self.ObjectA))
        self.assertEqual(len(a1), 1)
        self.assertTrue(isinstance(a2, self.ObjectA))
        self.assertEqual(len(a2), 2)

        s2 = self.VarArgs.call('a', 'b')
        i2 = self.VarArgs.call(1, 2)
        m2 = self.VarArgs.call('a', 1, 1.0)

        self.assertTrue(isinstance(s2, self.ObjectA))
        self.assertEqual(len(s2), 2)
        self.assertTrue(isinstance(i2, self.ObjectA))
        self.assertEqual(len(i2), 2)
        self.assertTrue(isinstance(m2, self.ObjectA))
        self.assertEqual(len(m2), 3)

    def testVarArgsOverload(self):
        m0 = self.VarArgs.callOverload(self.Integer(1))
        m1 = self.VarArgs.callOverload('a')
        m2 = self.VarArgs.callOverload('a', '1')
        self.assertTrue(isinstance(m0, self.Integer))
        self.assertTrue(isinstance(m1, self.StringA))
        self.assertTrue(isinstance(m2, self.StringA))

    def testVarArgsStringTest(self):
        strArray = jpype.JArray(jpype.JString)
        self.assertTrue(compareList(
            self.VarArgs.callString('a', 'b'), ['a', 'b']))
        self.assertTrue(compareList(self.VarArgs.callString('a'), ['a']))
        self.assertTrue(compareList(self.VarArgs.callString(), []))

    def testVarArgsPlus0(self):
        self.assertEqual(self.VarArgs.callString0("a"), 0)
        self.assertEqual(self.VarArgs.callString0("a", "b"), 1)
        self.assertEqual(self.VarArgs.callString0("a", "b", "c"), 2)

    def testVarArgsPlus1(self):
        var = self.VarArgs()
        self.assertEqual(var.callString1("a"), 0)
        self.assertEqual(var.callString1("a", "b"), 1)
        self.assertEqual(var.callString1("a", "b", "c"), 2)
