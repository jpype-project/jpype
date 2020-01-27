# *****************************************************************************
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
# *****************************************************************************
from jpype import *
import common
import sys


def _testMethod1():
    return 33


def _testMethod2():
    return 32


def _testMethod3():
    return "Foo"


class C:
    def testMethod1(self):
        return 43

    def testMethod2(self):
        return 42

    def testMethod3(self):
        return "Bar"

    def write(self, bytes, start, length):
        return bytes, start, length


class ThreadCallbackImpl:
    def __init__(self):
        self.values = []

    def notifyValue(self, val):
        self.values.append(val)


class ProxyTestCase(common.JPypeTestCase):

    def setUp(self):
        super(ProxyTestCase, self).setUp()
        self.package = JPackage("jpype.proxy")
        self._triggers = self.package.ProxyTriggers

    def testProxyDecl(self):
        d = {
            'testMethod1': _testMethod1,
            'testMethod2': _testMethod2,
        }
        itf1 = self.package.TestInterface1
        itf2 = self.package.TestInterface2
        proxy = JProxy(itf1, dict=d)
        proxy = JProxy([itf1], dict=d)
        proxy = JProxy([itf1, itf2], dict=d)
        proxy = JProxy("jpype.proxy.TestInterface1", dict=d)
        proxy = JProxy(["jpype.proxy.TestInterface1"], dict=d)

    def testProxyRoundTrip(self):

        @JImplements(java.lang.Runnable)
        class MyRun(object):
            @JOverride
            def run(self):
                pass

        al = JClass('java.util.ArrayList')()
        runner = MyRun()
        al.add(runner)
        self.assertIs(al.get(0), runner)

    def testProxyLeak(self):

        @JImplements(java.lang.Runnable)
        class MyRun(object):
            @JOverride
            def run(self):
                pass

        al = JClass('java.util.ArrayList')()
        runner = MyRun()
        al.add(runner)
        initial = sys.getrefcount(runner)
        for i in range(0, 10):
            self.assertIs(al.get(0), runner)
        final = sys.getrefcount(runner)
        self.assertEqual(initial, final)

    def testProxyDeclFail1(self):
        itf1 = self.package.TestInterface1
        # Missing required arguments
        with self.assertRaisesRegex(TypeError, "must be specified"):
            proxy = JProxy(itf1)

    def testProxyDeclFail2(self):
        itf1 = self.package.TestInterface1
        # Not a str, JClass, or str
        with self.assertRaisesRegex(TypeError, "is not a Java interface"):
            proxy = JProxy(int(1), dict={})

    def testProxyImplementsForm1(self):
        itf1 = self.package.TestInterface1
        itf2 = self.package.TestInterface2

        @JImplements(itf1)
        class MyImpl(object):
            @JOverride
            def testMethod1(self):
                pass

    def testProxyImplementsForm2(self):
        itf1 = self.package.TestInterface1
        itf2 = self.package.TestInterface2
        @JImplements(itf1, itf2)
        class MyImpl(object):
            @JOverride
            def testMethod1(self):
                pass

            @JOverride
            def testMethod2(self):
                pass

            @JOverride
            def write(self):
                pass

    def testProxyImplementsForm3(self):
        @JImplements("jpype.proxy.TestInterface1")
        class MyImpl(object):
            @JOverride
            def testMethod1(self):
                pass

    def testProxyImplementsForm4(self):
        @JImplements("jpype.proxy.TestInterface1", "jpype.proxy.TestInterface2")
        class MyImpl(object):
            @JOverride
            def testMethod1(self):
                pass

            @JOverride
            def testMethod2(self):
                pass

            @JOverride
            def write(self):
                pass

    def testProxyImplementsFail1(self):
        with self.assertRaisesRegex(TypeError, "least one Java interface"):
            # Empty interfaces
            @JImplements()
            class MyImpl(object):
                @JOverride
                def testMethod1(self):
                    pass

    def testProxyImplementsFail2(self):
        with self.assertRaisesRegex(TypeError, "is not a Java interface"):
            # Wrong type
            @JImplements(int(1))
            class MyImpl(object):
                @JOverride
                def testMethod1(self):
                    pass

    def testProxyImplementsFail3(self):
        with self.assertRaisesRegex(NotImplementedError, "requires method"):
            # Missing implementation
            @JImplements("jpype.proxy.TestInterface1")
            class MyImpl(object):
                def testMethod1(self):
                    pass

    def testProxyImplementsFail4(self):
        with self.assertRaisesRegex(TypeError, "is not a Java interface"):
            # extends
            @JImplements("java.lang.StringBuilder")
            class MyImpl(object):
                def testMethod1(self):
                    pass

    def testProxyWithDict(self):
        d = {
            'testMethod1': _testMethod1,
            'testMethod2': _testMethod2,
        }
        itf1 = self.package.TestInterface1
        itf2 = self.package.TestInterface2
        proxy = JProxy([itf1, itf2], dict=d)

        result = self._triggers.testProxy(proxy)

        expected = ['Test Method1 = 33', 'Test Method2 = 32']
        self.assertSequenceEqual(result, expected)

    def testProxyWithDictInherited(self):
        d = {
            'testMethod2': _testMethod2,
            'testMethod3': _testMethod3,
        }
        itf3 = self.package.TestInterface3
        proxy = JProxy(itf3, dict=d)

        result = self._triggers.testProxy(proxy)

        expected = ['Test Method2 = 32', 'Test Method3 = Foo']
        self.assertSequenceEqual(result, expected)

    def testProxyWithInst(self):
        itf3 = self.package.TestInterface3
        c = C()
        proxy = JProxy(itf3, inst=c)

        result = self._triggers.testProxy(proxy)

        expected = ['Test Method2 = 42', 'Test Method3 = Bar']
        self.assertSequenceEqual(result, expected)

    def testProxyWithThread(self):
        itf = self.package.TestThreadCallback
        tcb = ThreadCallbackImpl()
        proxy = JProxy(itf, inst=tcb)

        self._triggers().testProxyWithThread(proxy)
        self.assertEqual(tcb.values, ['Waiting for thread start',
                                      '1', '2', '3',
                                      'Thread finished'])

    def testProxyWithArguments(self):
        itf2 = self.package.TestInterface2
        c = C()
        proxy = JProxy(itf2, inst=c)

        result = self._triggers().testCallbackWithParameters(proxy)

        bytes, start, length = result
        self.assertSequenceEqual(bytes, [1, 2, 3, 4])
        self.assertEqual(start, 12)
        self.assertEqual(length, 13)

    def testProxyWithMultipleInterface(self):
        itf1 = self.package.TestInterface1
        itf2 = self.package.TestInterface2
        c = C()
        proxy = JProxy([itf1, itf2], inst=c)

        try:
            result = self._triggers.testProxy(proxy)
        except Exception as ex:
            print(ex.stacktrace())
            raise ex

        expected = ['Test Method1 = 43', 'Test Method2 = 42']
        self.assertSequenceEqual(result, expected)

    def testProxyWithMultipleInterfaceInherited(self):
        itf2 = self.package.TestInterface2
        itf3 = self.package.TestInterface3
        c = C()
        proxy = JProxy([itf2, itf3], inst=c)

        result = self._triggers().testCallbackWithParameters(proxy)

        bytes, start, length = result
        self.assertSequenceEqual(bytes, [1, 2, 3, 4])
        self.assertEqual(start, 12)
        self.assertEqual(length, 13)

    def testProxyJObjectCast(self):
        jrun = JClass('java.lang.Runnable')
        jobj = JClass("java.lang.Object")

        @JImplements(jrun)
        class MyClass(object):
            @JOverride
            def run(self):
                pass
        self.assertIsInstance(JObject(MyClass(), jrun), jrun)
        self.assertIsInstance(JObject(MyClass()), jobj)

    def testProxyConvert(self):
        # This was tests that arguments and "self" both
        # convert to the same object
        TestInterface5 = JClass("jpype.proxy.TestInterface5")
        ProxyTriggers = JClass("jpype.proxy.ProxyTriggers")

        @JImplements(TestInterface5)
        class Bomb(object):
            @JOverride
            def equals(self, other):
                return type(self)==type(other)

        b = Bomb()
        t = ProxyTriggers()
        self.assertTrue(t.testEquals(b))
