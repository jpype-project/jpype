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
import common
import unittest2
from sys import hexversion as ver

def _testMethod() :
    return 32

def _testMethod2() :
    return "Fooo!"

class C :
    def testMethod(self) :
        return 42
        
    def testMethod2(self) :
        return "Bar"

    def write(self, bytes, start, length) :
        print 'aaaaa'
        print bytes.__class__, bytes[0]
        print start
        print length

class ProxyTestCase(common.JPypeTestCase) :

    def setUp(self):
        super(ProxyTestCase, self).setUp()
        self.package = JPackage("jpype.proxy")

    def testProxyWithDict(self) :
        d = {
            'testMethod' : _testMethod,
            'testMethod2' : _testMethod2,
        }
        itf2 = self.package.ITestInterface3
        Test3 = self.package.Test3
        proxy = JProxy(itf2, dict=d)
    
        Test3.testProxy(proxy)

    def testProxyWithInst(self) :
        itf2 = self.package.ITestInterface3
        Test3 = self.package.Test3

        c = C()
        proxy = JProxy(itf2, inst=c)
        Test3.testProxy(proxy)   

    def testProxyWithThread(self) :
        itf2 = self.package.ITestInterface3
        Test3 = self.package.Test3

        c = C()
        proxy = JProxy(itf2, inst=c)

        t3 = Test3()
        t3.testProxyWithThread(proxy)

    @unittest2.skipIf(ver > 0x020703, 'broken, see ISSUE #67')
    def testProxyWithArguments(self) :
        itf2 = self.package.ITestInterface2
        Test3 = self.package.Test3

        c = C()
        proxy = JProxy(itf2, inst=c)
        Test3().testCallbackWithParameters(proxy)
    
    @unittest2.skipIf(ver > 0x020703, 'broken, see ISSUE #67')
    def testProxyWithMultipleInterface(self) :
        itf2 = self.package.ITestInterface2
        itf3 = self.package.ITestInterface3
        Test3 = self.package.Test3

        c = C()
        proxy = JProxy([itf2,itf3], inst=c)
        Test3().testCallbackWithParameters(proxy)
