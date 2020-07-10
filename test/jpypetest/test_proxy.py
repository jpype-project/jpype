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
import contextlib
import sys

from jpype import *
import common
import subrun


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

    def testNotImplemented(self):
        itf1 = self.package.TestInterface1
        proxy = JObject(JProxy(itf1, dict={}), itf1)
        with self.assertRaises(JClass("java.lang.NoSuchMethodError")):
            proxy.testMethod1()

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
                return type(self) == type(other)

        b = Bomb()
        t = ProxyTriggers()
        self.assertTrue(t.testEquals(b))

    def testProxyFail(self):
        with self.assertRaises(TypeError):
            JProxy(inst=object(), dict={}, intf="java.io.Serializable")

    def testValid(self):
        @JImplements("java.util.Comparator")
        class ValidComparator(object):
            @JOverride
            def compare(self, _o1, _o2):
                return 0

            @JOverride
            def equals(self, _obj):
                return True
        arr = JArray(JString)(["a", "b"])
        java.util.Arrays.sort(arr, ValidComparator())

    def testRaisePython(self):
        @JImplements("java.util.Comparator")
        class RaiseException(object):
            def __init__(self, exc):
                self.exc = exc

            @JOverride
            def compare(self, _o1, _o2):
                raise self.exc("nobody expects the Python exception!")

            @JOverride
            def equals(self, _obj):
                return True
        arr = JArray(JString)(["a", "b"])
        with self.assertRaises(TypeError):
            java.util.Arrays.sort(arr, RaiseException(TypeError))
        with self.assertRaises(ValueError):
            java.util.Arrays.sort(arr, RaiseException(ValueError))
        with self.assertRaises(SyntaxError):
            java.util.Arrays.sort(arr, RaiseException(SyntaxError))
        with self.assertRaises(java.lang.RuntimeException):
            java.util.Arrays.sort(
                arr, RaiseException(java.lang.RuntimeException))

    def testBad(self):
        @JImplements("java.util.Comparator")
        class TooManyParams(object):
            @JOverride
            def compare(self, _o1, _o2, _o3):
                return 0

            @JOverride
            def equals(self, _obj):
                return True

        @JImplements("java.util.Comparator")
        class TooFewParams(object):
            @JOverride
            def compare(self, _o1):
                return 0

            @JOverride
            def equals(self, _obj):
                return True

        arr = JArray(JString)(["a", "b"])
        with self.assertRaises(TypeError):
            java.util.Arrays.sort(arr, TooManyParams())
        with self.assertRaises(TypeError):
            java.util.Arrays.sort(arr, TooFewParams())

    def testUnwrap(self):
        fixture = JClass("jpype.common.Fixture")()
        @JImplements("java.io.Serializable")
        class Q(object):
            pass

        q = Q()
        self.assertEqual(JProxy.unwrap(q), q)
        q2 = fixture.callObject(q)
        self.assertEqual(JProxy.unwrap(q2), q)

        class R(object):
            pass
        r = R()
        s = JProxy("java.io.Serializable", inst=r)
        self.assertEqual(JProxy.unwrap(s), r)
        s2 = fixture.callObject(s)
        self.assertEqual(JProxy.unwrap(s2), r)

    def testConvert(self):
        fixture = JClass("jpype.common.Fixture")()

        class R(object):
            pass
        r = R()
        # Verify with not asked to auto convert it doesn't
        s = JProxy("java.io.Serializable", inst=r)
        s2 = fixture.callObject(s)
        self.assertEqual(s2, s)
        self.assertNotEqual(s2, r)

        # Verify that when asked to auto convert it does
        s = JProxy("java.io.Serializable", inst=r, convert=True)
        s2 = fixture.callObject(s)
        self.assertNotEqual(s2, s)
        self.assertEqual(s2, r)

    def testMethods(self):
        fixture = JClass("jpype.common.Fixture")()
        @JImplements("java.io.Serializable")
        class R(object):
            pass
        r = R()
        s = JObject(r)
        # Check if Java will be able to work with this object safely
        self.assertTrue(s.equals(s))
        self.assertIsInstance(s.getClass(), java.lang.Class)
        self.assertIsInstance(s.toString(), java.lang.String)
        self.assertIsInstance(s.hashCode(), int)

    def testMethods2(self):
        fixture = JClass("jpype.common.Fixture")()

        class R(object):
            pass
        r = JProxy("java.io.Serializable", inst=R())
        s = JObject(r)
        # Check if Java will be able to work with this object safely
        self.assertTrue(s.equals(s))
        self.assertIsInstance(s.getClass(), java.lang.Class)
        self.assertIsInstance(s.toString(), java.lang.String)
        self.assertIsInstance(s.hashCode(), int)

    def testDeferredCheckingFailure(self):
        @JImplements("java.lang.Runnable", deferred=True)
        class MyImpl(object):
            pass

        with self.assertRaisesRegex(NotImplementedError, "requires method"):
            MyImpl()

    def testDeferredCheckingValid(self):
        @JImplements("java.lang.Runnable", deferred=True)
        class MyImpl(object):
            @JOverride
            def run(self):
                pass

        assert isinstance(MyImpl(), MyImpl)

    def testWeak(self):
        hc = java.lang.System.identityHashCode
        @JImplements("java.io.Serializable")
        class MyObj(object):
            pass

        def f():
            obj = MyObj()
            jobj = JObject(obj)
            i = hc(obj)
            return i, obj
        i0, obj = f()
        i1 = hc(obj)
        # These should be the same if unless the reference was broken
        self.assertEqual(i0, i1)

    def testFunctional(self):
        def SupplierFunc():
            return 561
        js = JObject(SupplierFunc, "java.util.function.Supplier")
        self.assertEqual(js.get(), 561)

    def testFunctionalLambda(self):
        js = JObject(lambda x: 2 * x, "java.util.function.DoubleUnaryOperator")
        self.assertEqual(js.applyAsDouble(1), 2.0)


@subrun.TestCase(individual=True)
class TestProxyDefinitionWithoutJVM(common.JPypeTestCase):
    def setUp(self):
        # Explicitly *don't* call the parent setUp as this would start the JVM.
        pass

    @contextlib.contextmanager
    def assertRaisesJVMNotRunning(self):
        with self.assertRaisesRegex(
                RuntimeError, "Java Virtual Machine is not running"):
            yield

    def testDeferredCheckingNeedsJVM(self):
        @JImplements("java.lang.Runnable", deferred=True)
        class MyImpl(object):
            pass

        with self.assertRaisesJVMNotRunning():
            MyImpl()

    def testNotDeferredChecking(self):
        with self.assertRaisesJVMNotRunning():
            @JImplements("java.lang.Runnable", deferred=False)
            class MyImpl(object):
                pass

    def testDeferredDefault(self):
        with self.assertRaisesJVMNotRunning():
            @JImplements("java.lang.Runnable")
            class MyImpl(object):
                pass

    def testValidDeferredJVMNotRunning(self):
        @JImplements("java.lang.Runnable", deferred=True)
        class MyImpl(object):
            @JOverride
            def run(self):
                pass

        with self.assertRaisesJVMNotRunning():
            MyImpl()

    def testValidDeferredJVMRunning(self):
        @JImplements("java.lang.Runnable", deferred=True)
        class MyImpl(object):
            @JOverride
            def run(self):
                pass

        startJVM()
        assert isinstance(MyImpl(), MyImpl)
