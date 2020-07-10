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
from jpype import JException, java, JProxy, JClass
from jpype.types import *
import traceback
import common


def throwIOException():
    raise java.io.IOException("Test throw")


def throwByJavaException():
    JClass('jpype.exc.ExceptionTest').throwIOException()


class ExceptionTestCase(common.JPypeTestCase):
    def testExceptionThrown(self):
        ext = JClass('jpype.exc.ExceptionTest')
        try:
            ext.throwRuntime()
            self.fail()
        except JException as ex:
            self.assertIs(type(ex), java.lang.RuntimeException)
            self.assertEqual('Foo', ex.message())
            trace = ex.stacktrace()
            self.assertTrue(str(trace).startswith(
                'java.lang.RuntimeException: Foo'))

    def testExceptionByJavaClass(self):
        ext = JClass('jpype.exc.ExceptionTest')
        try:
            ext.throwRuntime()
            self.fail()
        except java.lang.RuntimeException as ex:
            self.assertIs(type(ex), java.lang.RuntimeException)
            self.assertEqual('Foo', ex.message())
            trace = ex.stacktrace()
            self.assertTrue(str(trace).startswith(
                'java.lang.RuntimeException: Foo'))

    def testThrowException(self):
        exthrow = JClass('jpype.exc.ExceptionThrower')
        extest = JClass('jpype.exc.ExceptionTest')
        d = {"throwIOException": throwIOException, }
        p = JProxy(exthrow, dict=d)
        self.assertTrue(extest.delegateThrow(p))

    def testThrowException3(self):
        exthrow = JClass('jpype.exc.ExceptionThrower')
        extest = JClass('jpype.exc.ExceptionTest')
        d = {"throwIOException": throwByJavaException, }
        p = JProxy(exthrow, dict=d)

        self.assertTrue(extest.delegateThrow(p))

#    This test is problematic as __name__ is a class property not an object property
#    def testExceptionPYEXCName(self):
#        e = self.jpype.exc.ChildTestException()
#        name = "jpype.exc.ChildTestException"
#        self.assertEqual(name, e.__name__)

    def testExceptionInstanceof(self):
        e = self.jpype.exc.ChildTestException()
        self.assertIsInstance(e, self.jpype.exc.ParentTestException)

    def testExceptionPYEXCInstanceof(self):
        e = self.jpype.exc.ChildTestException
        self.assertTrue(issubclass(e, self.jpype.exc.ParentTestException))

    def testThrowChildExceptionFromCatchJExceptionParentClass(self):
        try:
            self.jpype.exc.ExceptionTest.throwChildTestException()
            self.fail()
        except self.jpype.exc.ParentTestException as ex:
            self.assertIsInstance(ex, self.jpype.exc.ChildTestException)

    def testCause(self):
        cls = jpype.JClass("jpype.exc.ExceptionTest")
        try:
            cls.throwChain()
        except Exception as ex:
            ex1 = ex

        self.assertEqual(str(ex1.__cause__), "Java Exception")
        frame = ex1.__cause__.__traceback__
        expected = [
            'jpype.exc.ExceptionTest.throwChain',
            'jpype.exc.ExceptionTest.method1',
            'jpype.exc.ExceptionTest.method2',
        ]
        i = 0
        while (frame):
            self.assertEqual(frame.tb_frame.f_code.co_name, expected[i])
            frame = frame.tb_next
            i += 1

    def testIndexError(self):
        with self.assertRaises(IndexError):
            raise java.lang.IndexOutOfBoundsException("From Java")

    def testValueError(self):
        js = JObject(None, JString)
        with self.assertRaises(ValueError):
            js.substring(0)

    def testExcCtor(self):
        WE = jpype.JClass("jpype.exc.WierdException")
        with self.assertRaises(WE):
            WE.testThrow()
        try:
            WE.testThrow()
        except Exception as ex:
            ex1 = ex
        self.assertEqual(ex1.args, ("Got it",))

    def testExcCauseChained1(self):
        import jpype.imports
        try:
            from org.jpype.fail import BadInitializer
        except Exception as ex:
            ex1 = ex
        self.assertIsInstance(ex1, ImportError)
        self.assertIsInstance(ex1.__cause__, JClass(
            "java.lang.ExceptionInInitializerError"))
        self.assertIsInstance(ex1.__cause__.__cause__, JClass(
            "java.lang.ArrayIndexOutOfBoundsException"))
        self.assertTrue(ex1.__cause__.__traceback__ is not None)
        self.assertTrue(ex1.__cause__.__cause__.__traceback__ is not None)

    def testExcCauseChained2(self):
        try:
            JClass('org.jpype.fail.BadInitializer2')
        except Exception as ex:
            ex1 = ex
        self.assertIsInstance(ex1, JClass(
            'java.lang.ExceptionInInitializerError'))
        self.assertIsInstance(ex1.__cause__.__cause__, JClass(
            "java.lang.ArrayIndexOutOfBoundsException"))
        self.assertTrue(ex1.__cause__.__traceback__ is not None)
        self.assertTrue(ex1.__cause__.__cause__.__traceback__ is not None)

    def testExpandStacktrace(self):
        Th = jpype.JClass('java.lang.Throwable')
        null = jpype.JObject(None, Th)
        # The next line should not fail
        Th._expandStacktrace(null)

    def testException(self):
        Th = jpype.JClass('java.lang.Throwable')('abc')
        self.assertEqual(str(Th), 'java.lang.Throwable: abc')
