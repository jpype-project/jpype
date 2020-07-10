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
import common
import sys


def pythonNewerThan(major, minor):
    return sys.version_info[0] > major or (sys.version_info[0] == major and sys.version_info[1] > minor)


class CloseableTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testCloseable(self):
        CloseableTest = jpype.JClass("jpype.closeable.CloseableTest")
        CloseableTest.reset()
        self.assertFalse(CloseableTest.closed)
        with CloseableTest() as myFile:
            myFile.print_("hello 1")
        self.assertEqual(CloseableTest.printed, "hello 1")
        self.assertTrue(CloseableTest.closed)

    @common.unittest.skipUnless(pythonNewerThan(3, 0), "requires python 3")
    def testCloseableFail(self):
        CloseableTest = jpype.JClass("jpype.closeable.CloseableTest")
        CloseableTest.reset()
        CloseableTest.willfail = True
        self.assertFalse(CloseableTest.closed)
        ex1 = None
        try:
            with CloseableTest() as myFile:
                myFile.print_("hello 1")
                try:
                    raise Exception('foo')
                except Exception as ex:
                    pass
        except Exception as ex:
            ex1 = ex
        self.assertTrue(ex1)
        self.assertEqual(CloseableTest.printed, "hello 1")
        self.assertTrue(CloseableTest.closed)

    def testCloseablePyExcept(self):
        CloseableTest = jpype.JClass("jpype.closeable.CloseableTest")
        CloseableTest.reset()
        self.assertFalse(CloseableTest.closed)
        try:
            with CloseableTest() as myFile:
                myFile.print_("hello 2")
                raise TypeError("Python exception")
                myFile.print_("there")
        except Exception as ex:
            self.assertIsInstance(ex, TypeError)
        self.assertEqual(CloseableTest.printed, "hello 2")
        self.assertTrue(CloseableTest.closed)

    @common.unittest.skipUnless(pythonNewerThan(2, 6), "Earlier python does not support stacked exceptions.")
    def testCloseablePyExceptFail(self):
        CloseableTest = jpype.JClass("jpype.closeable.CloseableTest")
        CloseableTest.reset()
        CloseableTest.willfail = True
        self.assertFalse(CloseableTest.closed)
        try:
            with CloseableTest() as myFile:
                myFile.print_("hello 3")
                raise TypeError("Python exception")
                myFile.print_("there")
        except TypeError as ex:
            self.assertIsInstance(ex, TypeError)
        self.assertEqual(CloseableTest.printed, "hello 3")
        self.assertTrue(CloseableTest.closed)
        self.assertTrue(CloseableTest.failed)

    def testCloseableJExcept(self):
        CloseableTest = jpype.JClass("jpype.closeable.CloseableTest")
        CloseableTest.reset()
        self.assertFalse(CloseableTest.closed)
        try:
            with CloseableTest() as myFile:
                myFile.print_("hello 4")
                myFile.throwException()
                myFile.print_("there")
        except Exception as ex:
            self.assertIsInstance(ex, jpype.JException,
                                  'type is %s' % type(ex))
            self.assertEqual(ex.getMessage(), 'oh no!')
        self.assertEqual(CloseableTest.printed, "hello 4")
        self.assertTrue(CloseableTest.closed)

    @common.unittest.skipUnless(pythonNewerThan(2, 6), "Earlier python does not support stacked exceptions.")
    def testCloseableJExceptFail(self):
        CloseableTest = jpype.JClass("jpype.closeable.CloseableTest")
        CloseableTest.reset()
        CloseableTest.willfail = True
        self.assertFalse(CloseableTest.closed)
        try:
            with CloseableTest() as myFile:
                myFile.print_("hello 5")
                myFile.throwException()
                myFile.print_("there")
        except Exception as ex:
            self.assertIsInstance(ex, jpype.JException,
                                  'type is %s' % type(ex))
            self.assertEqual(ex.getMessage(), 'oh no!')  # fail if get "oh my?"
        self.assertEqual(CloseableTest.printed, "hello 5")
        self.assertTrue(CloseableTest.closed)
        self.assertTrue(CloseableTest.failed)

    def testCloseableAttr(self):
        cls = jpype.JClass("java.io.Closeable")
        self.assertTrue(hasattr(cls, '__enter__'))
        self.assertTrue(hasattr(cls, '__exit__'))

    def testAutoCloseableAttr(self):
        cls = jpype.JClass("java.lang.AutoCloseable")
        self.assertTrue(hasattr(cls, '__enter__'))
        self.assertTrue(hasattr(cls, '__exit__'))
