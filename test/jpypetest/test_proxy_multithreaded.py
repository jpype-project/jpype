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
from jpype import *
import common
import time


class C:
    def testMethodInt(self):
        return 5

    def testMethodObject(self):
        return JPackage("jpype.proxy").ReturnObject(3)

    def testMethodString(self):
        return "Hallo"

    def testMethodList(self, noOfValues):
        responses = list()
        for i in range(0, noOfValues):
            responses.append(JPackage("jpype.proxy").ReturnObject(i))
        return java.util.Arrays.asList(responses)


class ThreadCallbackImpl:
    def __init__(self):
        self.values = []

    def notifyValue(self, val):
        self.values.append(val)


class ProxyMultiThreadedTestCase(common.JPypeTestCase):

    def setUp(self):
        super(ProxyMultiThreadedTestCase, self).setUp()
        self.package = JPackage("jpype.proxy")
        self.executor = None

    def tearDown(self):

        if self.executor is not None:
            self.executor.shutdown()

    def testProxyWithSingleThreadExecution(self):
        """Test multiple proxy calls with a single threaded executor.

        The Java part will only call one proxy method (longer running methods)
        at a time.

        The methods should run correctly.
        """

        proxy = JProxy(self.package.TestInterface4, inst=C())

        self.executor = self.package.ProxyExecutor(1)
        self.executor.registerProxy(proxy, 10)

        # register proxy executions
        # threads

        self.executor.runExecutor()
        result = self.executor.waitForExecutedTasks()
        expected = self.executor.getExpectedTasks()
        self.assertEqual(result, expected,
                         "Executed Tasks should be the same.")

    def testProxyWithMultipleThreadExecutionSingleCallbackInstance(self):
        """Test multiple proxy calls with different threads at the same time.

        The Java part will call the same callback instance from different
        threads at a time.

        Jpype library will throw an exception and the proxy method won't be
        called:
            java.lang.RuntimeException: Python exception thrown:
                AttributeError: 'NoneType' object has no attribute 'getName'
                at jpype.JPypeInvocationHandler.hostInvoke(Native Method)
            at jpype.JPypeInvocationHandler.invoke(
                JPypeInvocationHandler.java:10)
            ...
        """
        proxy = JProxy(self.package.TestInterface4, inst=C())

        self.executor = self.package.ProxyExecutor(10)
        self.executor.registerProxy(proxy, 100)

        # register proxy executions
        # threads
        self.executor.runExecutor()
        result = self.executor.waitForExecutedTasks()

        expected = self.executor.getExpectedTasks()
        self.assertEqual(result, expected,
                         "Executed Tasks should be the same.")

    def testProxyWithMultipleThreadExecutionMultiCallbackInstances(self):
        """Test multiple proxy calls with different threads at the same time.

        The Java part will call the proxy method from different callback
        instances and from different threads at a time.

        Jpype library will throw an exception and the proxy method won't be
        called:
            java.lang.RuntimeException: Python exception thrown:
                AttributeError: 'NoneType' object has no attribute 'getName'
                at jpype.JPypeInvocationHandler.hostInvoke(Native Method)
            at jpype.JPypeInvocationHandler.invoke(
                JPypeInvocationHandler.java:10)
            ...
        """

        self.executor = self.package.ProxyExecutor(10)

        for i in range(0, 5):
            proxy = JProxy(self.package.TestInterface4, inst=C())
            self.executor.registerProxy(proxy, 15)

        # register proxy executions
        # threads

        self.executor.runExecutor()
        result = self.executor.waitForExecutedTasks()
        expected = self.executor.getExpectedTasks()
        self.assertEqual(result, expected,
                         "Executed Tasks should be the same.")
