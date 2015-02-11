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
from jpype import JException, java, JavaException, JProxy, JPackage
import traceback
from . import common

def throwIOException() :
    raise java.io.IOException.PYEXC("Test throw")

def throwByJavaException() :
    JPackage('jpype').exc.ExceptionTest.throwIOException()

class ExceptionTestCase(common.JPypeTestCase) :
    def testExceptionThrown(self) :
        with self.assertRaises(JavaException) as cm:
            self.jpype.exc.ExceptionTest.throwRuntime()
        self.assertIs(cm.exception.javaClass(), java.lang.RuntimeException)
        self.assertEqual('Foo', cm.exception.message())
        trace = cm.exception.stacktrace()
        self.assertTrue(trace.startswith('java.lang.RuntimeException: Foo'))

    def testExceptionByJavaClass(self) :
        with self.assertRaises(JException(java.lang.RuntimeException)) as cm:
            self.jpype.exc.ExceptionTest.throwRuntime()
        self.assertIs(cm.exception.javaClass(), java.lang.RuntimeException)
        self.assertEqual('Foo', cm.exception.message())
        trace = cm.exception.stacktrace()
        self.assertTrue(trace.startswith('java.lang.RuntimeException: Foo'))

#       def testThrowException(self) :
#               d = {"throwIOException" : throwIOException, }
#               p = JProxy(self.jpype.exc.ExceptionThrower, dict=d)
#
#               assert self.jpype.exc.ExceptionTest.delegateThrow(p)

    def testThrowException3(self) :
        d = {"throwIOException" : throwByJavaException, }
        p = JProxy(self.jpype.exc.ExceptionThrower, dict=d)

        self.assertTrue(self.jpype.exc.ExceptionTest.delegateThrow(p))

    def testExceptionPYEXCName(self) :
        e = self.jpype.exc.ChildTestException()
        name = "jpype.exc.ChildTestExceptionPyRaisable"
        self.assertEqual(name, e.PYEXC.__name__)

    def testExceptionInstanceof(self) :
        e = self.jpype.exc.ChildTestException()
        self.assertIsInstance(e, self.jpype.exc.ParentTestException)

    def testExceptionPYEXCInstanceof(self) :
        e = self.jpype.exc.ChildTestException()
        self.assertTrue(issubclass(e.PYEXC,
                                   self.jpype.exc.ParentTestException.PYEXC))

    def testThrowChildExceptionFromCatchJExceptionParentClass(self) :
        expected_exc = JException(self.jpype.exc.ParentTestException)
        with self.assertRaises(expected_exc) as cm:
            self.jpype.exc.ExceptionTest.throwChildTestException()
        pyexc = self.jpype.exc.ChildTestException.PYEXC
        self.assertIsInstance(cm.exception, pyexc)
