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
from jpype import *
from . import common

try:
    import unittest2 as unittest
except ImportError:
    import unittest

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
        self.assertSequenceEqual(bytes, [1, 2 ,3 ,4])
        self.assertEqual(start, 12)
        self.assertEqual(length, 13)

    def testProxyWithMultipleInterface(self):
        itf1 = self.package.TestInterface1
        itf2 = self.package.TestInterface2
        c = C()
        proxy = JProxy([itf1, itf2], inst=c)

        result = self._triggers.testProxy(proxy)

        expected = ['Test Method1 = 43', 'Test Method2 = 42']
        self.assertSequenceEqual(result, expected)

    def testProxyWithMultipleInterfaceInherited(self):
        itf2 = self.package.TestInterface2
        itf3 = self.package.TestInterface3
        c = C()
        proxy = JProxy([itf2,itf3], inst=c)

        result = self._triggers().testCallbackWithParameters(proxy)

        bytes, start, length = result
        self.assertSequenceEqual(bytes, [1, 2 ,3 ,4])
        self.assertEqual(start, 12)
        self.assertEqual(length, 13)
